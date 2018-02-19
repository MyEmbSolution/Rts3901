var network_tcpip = {
    current_ethType: 0,
    current_brEnable: 0,
    current_ipaddr: "",
    ipv6_supported: false,
    setLanInfo: 1,
    wanFlash: {},
    initParams: function() {
        $.ajaxSetup("cache", false);
        $("#brEnable, #lan_dhcpd").bootstrapSwitch("size", "mini");

        var request = {
            command: "getInfo",
            data: ["brEnable", "ethType", "wanInfo", "wanFlash", "lanInfo", "ipv6Info"]
        };

        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#brEnable").bootstrapSwitch("state", o.data.brEnable).trigger("switchChange.bootstrapSwitch");
                network_tcpip.current_brEnable = o.data.brEnable;
                $("#ethType").val(o.data.ethType);
                network_tcpip.current_ethType = o.data.ethType;
                $.each(o.data.wanInfo, function(id, value) {
                    $("#wan_" + id).val(value);
                });
                network_tcpip.current_ipaddr = o.data.wanInfo.ipaddr;
                $("#wanSetting input").prop("disabled", o.data.wanInfo.dhcpc6);
                $.each(o.data.lanInfo, function(id, value) {
                    if (id === "dhcpd") {
                        $("#lan_dhcpd").bootstrapSwitch("state", value);
                        var $dhcp = $("#lanSetting tbody tr:gt(2)");
                        value ? $dhcp.show() : $dhcp.hide();
                    } else {
                        $("#lan_" + id).val(value);
                    }
                    if (id === "ipaddr" && value === "0.0.0.0") {
                        $("#lanSetting input").prop("disabled", true);
                        $("#lan_dhcpd").bootstrapSwitch("disabled", true);
                        network_tcpip.setLanInfo = 0;
                    }
                });
                network_tcpip.wanFlash = o.data.wanFlash;
                if (o.data.ipv6Info.support) {
                    network_tcpip.ipv6_supported = true;
                    $("#Ipv6Setting").show();
                    $("#Ipv6_dhcpc6").val(o.data.ipv6Info.dhcpc6);
                    if (o.data.ipv6Info.dhcpc6) {
                        $(".Ipv6_static").hide();
                    } else {
                        $("#Ipv6_ipaddr6").val(o.data.ipv6Info.ipaddr6[0].ipaddr6);
                        $("#Ipv6_prefix6").val(o.data.ipv6Info.ipaddr6[0].prefix6);
                        $("#Ipv6_gateway6").val(o.data.ipv6Info.gateway6);
                        $(".Ipv6_static").show();
                    }
                    $("#Ipv6Info_dhcpc6").val(o.data.ipv6Info.dhcpc6);
                    $.each(o.data.ipv6Info.ipaddr6, function(i, ip) {
                        $("#Ipv6Info_gateway6").parent().parent().before(
                            $("<tr>").append($("<th>").text("IP Address " + (i + 1)))
                                     .append($("<td>").append($("<input disabled>").val(ip.ipaddr6 + "/" + ip.prefix6))
                        ));
                    });
                    $("#Ipv6Info_gateway6").val(o.data.gateway6);
                }
            } else {
                showAlert("danger", "<strong>Try Again!</strong>  Get TCP/IP config failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $("#brEnable").on("switchChange.bootstrapSwitch", function() {
            if (!$(this).bootstrapSwitch("state")) {
                $("#wanSetting thead th").text("WAN Setting");
                $("#lanSetting").show();
            } else {
                $("#wanSetting thead th").text("LAN / WAN Setting");
                $("#lanSetting").hide()
            }
        });
        $("#wan_dhcpc").on("change", function() {
            $("#wanSetting input").prop("disabled", +$(this).val());
            if (!+$(this).val()) {
                $.each(network_tcpip.wanFlash, function(id, value) {
                    if (id !== "dhcpc")
                        $("#wan_" + id).val(value);
                });
            }
        });
        $("#Ipv6_dhcpc6").on("change", function() {
            +$(this).val() ? $(".Ipv6_static").hide() : $(".Ipv6_static").show();
        });
        $("#lan_dhcpd").on("switchChange.bootstrapSwitch", function() {
            var $dhcp = $("#lanSetting tbody tr:gt(2)");
            $(this).bootstrapSwitch("state") ? $dhcp.show() : $dhcp.hide();
        });
        $("input").on("keyup", function() {
            if (this.id === "lan_ipaddr"
                || this.id === "lan_netmask"
                || this.id === "lan_beginIP"
                || this.id === "lan_endIP") {
                if (!regex.ip.test($("#lan_ipaddr").val())
                    || !regex.netmask.test($("#lan_netmask").val())
                    || !regex.ip.test($("#lan_beginIP").val())
                    || !regex.ip.test($("#lan_endIP").val())) {
                    $("#save").prop("disabled", true);
                } else {
                    var ip = $("#lan_ipaddr").val();
                    var mask = $("#lan_netmask").val();
                    var begin = $("#lan_beginIP").val();
                    var end = $("#lan_endIP").val();
                    var valid = true;
                    for (var i = 0; i < 4; i++)
                        valid = valid && ((+ip.split(".")[i] & +mask.split(".")[i]) === (+begin.split(".")[i] & +mask.split(".")[i]))
                                      && ((+ip.split(".")[i] & +mask.split(".")[i]) === (+end.split(".")[i] & +mask.split(".")[i]));
                    valid = valid && (+begin.split(".")[3] <= +end.split(".")[3])
                    $("#save").prop("disabled", !valid);
                }
            } else {
                $("#save").prop("disabled", !regex[this.className].test($(this).val()));
            }
        });
        $("#save").on("click", function() {
            if (network_tcpip.current_ethType === +$("#ethType").val()
                && network_tcpip.current_brEnable === +$("#brEnable").bootstrapSwitch("state")) {
                $("#doJob").trigger("click");
                $("#save").prop("disabled", true);
            } else {
                $("#waitModal").modal("show");
            }
        });
        $("#Ipv6_showInfo").on("click", function() {
            $("#Ipv6Info").modal("show");
        });
        $("#doJob").on("click", function() {
            var request = {
                command: "setInfo",
                data: {
                    brEnable: +$("#brEnable").bootstrapSwitch("state"),
                    ethType: +$("#ethType").val(),
                    wanInfo: {
                        dhcpc: +$("#wan_dhcpc").val(),
                        ipaddr: $("#wan_ipaddr").val(),
                        netmask: $("#wan_netmask").val(),
                        gateway: $("#wan_gateway").val(),
                        dns1: $("#wan_dns1").val(),
                        dns2: $("#wan_dns2").val()
                    }
                }
            };
            if (!request.data.brEnable && network_tcpip.setLanInfo) {
                request.data.lanInfo = {
                    dhcpd: +$("#lan_dhcpd").bootstrapSwitch("state"),
                    ipaddr: $("#lan_ipaddr").val(),
                    netmask: $("#lan_netmask").val()
                };
                if (request.data.lanInfo.dhcpd) {
                    request.data.lanInfo.beginIP = $("#lan_beginIP").val();
                    request.data.lanInfo.endIP = $("#lan_endIP").val();
                }
            }
            if (network_tcpip.ipv6_supported) {
                request.data.ipv6Info = {};
                request.data.ipv6Info.dhcpc6 = +$("#Ipv6_dhcpc6").val();
                if (!request.data.ipv6Info.dhcpc6) {
                    request.data.ipv6Info.ipaddr6 = $("#Ipv6_ipaddr6").val();
                    request.data.ipv6Info.prefix6 = +$("#Ipv6_prefix6").val();
                    request.data.ipv6Info.gateway6 = $("#Ipv6_gateway6").val();
                }
            };
            $.ajax({
                method: "post",
                url: "/cgi-bin/network.cgi",
                data: JSON.stringify(request),
                dataType: "json",
                timeout: 500,
                cache: false,
                success: function(o) {
                    if (o.status === "succeed") {
                        if (network_tcpip.current_ethType !== +$("#ethType").val()
                            || network_tcpip.current_brEnable !== +$("#brEnable").bootstrapSwitch("state"))
                            utils.reboot(30 * 1000, +$("#wan_dhcpc").val() ? undefined : ("http://" + $("#wan_ipaddr").val()));
                        else
                            showAlert("success", "<strong>Success!</strong>  TCP/IP config saved");
                    } else {
                        $("#waitModal").modal("hide");
                        showAlert("danger", "<strong>Try Again!</strong>  Set TCP/IP config failed.");
                    }
                    $("#save").prop("disabled", false);
                },
                error: function(jqXHR, reason) {
                    if (reason === "timeout") {
                        if (!(+$("#wan_dhcpc").val()) && network_tcpip.current_ipaddr !== $("#wan_ipaddr").val()) {
                            setTimeout(function() {
                                location.replace("http://" + $("#wan_ipaddr").val());
                            }, 2000);
                            showAlert("success", "<strong>Success!</strong>  Browser will redirect to the new ip!");
                        } else if (+$("#wan_dhcpc").val()) {
                            showAlert("success", "<strong>Success!</strong>  Please visit website using the new ip!");
                        }
                    }
                }
            });
        });
    }
};

$(function() {
    network_tcpip.initParams();
    network_tcpip.bindEvent();
});
