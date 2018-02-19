var network_wireless = {
    currentAp: "",
    wifiApList: [],
    maxChannel: 13,
    info: {},
    validate: function(tab) {
        if (tab === "station" && $("#station_wpsEnable").bootstrapSwitch("state")) {
            var valid = true;
            if ($("[name=station_wpsMode]:eq(1)").prop("checked"))
                valid = regex.wpsPin.test($("#station_wpsServerPin").val());
            else if ($("[name=station_wpsMode]:eq(2)").prop("checked"))
                valid = regex.wpsPin.test($("#station_wpsClientPin").val());
            $("#station_save").prop("disabled", !valid);
        } else {
            var valid = true;
            if (!$("#" + tab + "_essid").val().length)
                valid = false;
            if (+$("#" + tab + "_auth").val() > 1)
                valid = valid && regex.wpaPasswd.test($("#" + tab + "_password").val());
            else if (+$("#" + tab + "_auth").val() == 1)
                valid = valid && regex.wepPasswd.test($("#" + tab + "_password").val());

            $("#" + tab + "_save").prop("disabled", !valid);
        }
    },
    enableStatusChanged: function(tab) {
        if (tab === "station") {
            if (network_wireless.info.staInfo.enable) {
                $("#station_enable").bootstrapSwitch("state", true);
                $("#station_scan").prop("disabled", false).trigger("click");
                $("#tab_station table input, #tab_station table select").prop("disabled", false);
                $("#station_wpsEnable").bootstrapSwitch("disabled", false);
                if (network_wireless.info.staInfo.wps_enable) {
                    if (network_wireless.info.staInfo.wps_pbc) {
                        $("[name=station_wpsMode]:eq(0)").prop("checked", true);
                    } else {
                        $("[name=station_wpsMode]:eq(1)").prop("checked", true);
                    }
                    $("#station_wpsServerPin").val(network_wireless.info.staInfo.wps_pin);
                } else {
                    $("#station_auth").val(network_wireless.info.staInfo.auth).trigger("change");
                    $.each(network_wireless.info.staInfo, function(id, value) {
                        if (id !== "enable")
                            $("#station_" + id).val(value);
                    });
                }
                network_wireless.validate("station");
            } else {
                $("#station_enable").bootstrapSwitch("state", false);
                $("#tab_station table input, #tab_station table select").prop("disabled", true);
                $("#station_scan,#station_save").prop("disabled", true);
                $("#station_wpsEnable").bootstrapSwitch("disabled", true);
            }
        } else if (tab === "softAp") {
            $("#softAp_auth").val(network_wireless.info.softapInfo.auth).trigger("change");
            $("#softAp_hide").bootstrapSwitch("state", network_wireless.info.softapInfo.hide);
            $.each(network_wireless.info.softapInfo, function(id, value) {
                if (id !== "hide")
                    $("#softAp_" + id).val(value);
            });
            network_wireless.validate("softAp");
        }
    },
    addAp: function(i, ap) {
        var lockIcon = "wifi_aplist_lock_off";
        var signalIcon = "wifi_aplist_signal_off_30";
        if (ap.flag.split("WPA").length === 1 && ap.flag.split("WEP").length === 1)
            lockIcon = "";
        if (ap.signal >= 40 && ap.signal < 80)
            signalIcon = "wifi_aplist_signal_off_60";
        else if (ap.signal >= 80)
            signalIcon = "wifi_aplist_signal_off_100";
        $("<tr>").addClass("wifi_aplist_tr_unselect_bg")
            .attr("id", "ap_list_" + i)
            .on({
                "click": function() {
                    $("#station_wpsEnable").bootstrapSwitch("state", false);
                    if (this.id !== network_wireless.currentAp) {
                        if (network_wireless.currentAp !== "") {
                            $("#" + network_wireless.currentAp).removeClass("wifi_aplist_tr_select_bg").addClass("wifi_aplist_tr_unselect_bg");
                            $("#" + network_wireless.currentAp).find("td").eq(0).css("color", "#5c5c5c");
                            if ($("#" + network_wireless.currentAp).find("td").eq(1).hasClass("wifi_aplist_lock_on"))
                                $("#" + network_wireless.currentAp).find("td").eq(1).removeClass("wifi_aplist_lock_on").addClass("wifi_aplist_lock_off");
                            var wifi_icon_class = $("#" + network_wireless.currentAp).find("td").eq(2).attr("class").replace("on", "off");
                            $("#" + network_wireless.currentAp).find("td").eq(2).attr("class", wifi_icon_class);
                        }
                        $(this).removeClass("wifi_aplist_tr_unselect_bg").addClass("wifi_aplist_tr_select_bg");
                        $(this).find("td").eq(0).css("color", "#fff");
                        if ($(this).find("td").eq(1).hasClass("wifi_aplist_lock_off"))
                            $(this).find("td").eq(1).removeClass("wifi_aplist_lock_off").addClass("wifi_aplist_lock_on");
                        var wifi_icon_class = $(this).find("td").eq(2).attr("class").replace("off", "on");
                        $(this).find("td").eq(2).attr("class", wifi_icon_class);
                        network_wireless.currentAp = this.id;
                    }
                    var index = $("#station_apList tbody tr").index(this);
                    $("#station_bssid").text(network_wireless.wifiApList[index].bssid);
                    $("#station_channel").text(network_wireless.wifiApList[index].channel);
                    $("#station_signal").text(network_wireless.wifiApList[index].signal);
                    $("#station_flag").text(network_wireless.wifiApList[index].flag);
                    $("#station_essid").val(network_wireless.wifiApList[index].essid);
                    var flag = network_wireless.wifiApList[index].flag;
                    if (flag.split("WPA").length > 1) {
                        if (flag.split("WPA").length === 3)
                            $("#station_auth").val(4);
                        else if (flag.split("WPA2").length > 1)
                            $("#station_auth").val(3);
                        else
                            $("#station_auth").val(2);
                        $("#station_auth").trigger("change");
                        if (flag.split("TKIP").length > 1 && flag.split("CCMP").length > 1)
                            $("#station_cipher").val(2);
                        else if (flag.split("TKIP").length > 1)
                            $("#station_cipher").val(1);
                        else if (flag.split("CCMP").length > 1)
                            $("#station_cipher").val(0);
                    } else if (flag.split("WEP").length > 1) {
                        $("#station_auth").val(1);
                        $("#station_auth").trigger("change");
                    } else {
                        $("#station_auth").val(0);
                        $("#station_auth").trigger("change");
                    }
                },
                "mouseenter": function() {
                    if (this.id !== network_wireless.currentAp) {
                        $(this).removeClass("wifi_aplist_tr_unselect_bg").addClass("wifi_aplist_tr_select_bg");
                        $(this).find("td").eq(0).css("color", "#fff");
                        if ($(this).find("td").eq(1).hasClass("wifi_aplist_lock_off"))
                            $(this).find("td").eq(1).removeClass("wifi_aplist_lock_off").addClass("wifi_aplist_lock_on");
                        var wifi_icon_class = $(this).find("td").eq(2).attr("class").replace("off", "on");
                        $(this).find("td").eq(2).attr("class", wifi_icon_class);
                    }
                },
                "mouseleave": function() {
                    if (this.id !== network_wireless.currentAp) {
                        $(this).removeClass("wifi_aplist_tr_select_bg").addClass("wifi_aplist_tr_unselect_bg");
                        $(this).find("td").eq(0).css("color", "#5c5c5c");
                        if ($(this).find("td").eq(1).hasClass("wifi_aplist_lock_on"))
                            $(this).find("td").eq(1).removeClass("wifi_aplist_lock_on").addClass("wifi_aplist_lock_off");
                        var wifi_icon_class = $(this).find("td").eq(2).attr("class").replace("on", "off");
                        $(this).find("td").eq(2).attr("class", wifi_icon_class);
                    }
                }
            })
            .append($("<td>").addClass("tab_sub_title").text(ap.essid))
            .append($("<td>").addClass(lockIcon))
            .append($("<td>").addClass(signalIcon))
            .appendTo($("#station_apList tbody"));
    },
    initStation: function() {
        network_wireless.enableStatusChanged("station");
        $("#station_enable").on("switchChange.bootstrapSwitch", function() {
            var request = {
                command: "setInfo",
                data: {
                    staInfo: {
                        enable: +$(this).bootstrapSwitch("state")
                    }
                }
            };
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    network_wireless.info.staInfo.enable = request.data.staInfo.enable;
                    network_wireless.enableStatusChanged("station");
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Set station enable status failed.");
                }
            }, "json");
        });
    },
    initSoftap: function() {
        network_wireless.enableStatusChanged("softAp");
        $("#softAp_enable").bootstrapSwitch("state", network_wireless.info.softapInfo.enable).trigger("switchChange.bootstrapSwitch");
    },
    initParams: function() {
        $.ajaxSetup("cache", false);
        for (var i = 1; i <= network_wireless.maxChannel; i++)
            $("#softAp_channel").append(new Option(i));
        $("div.tab-content input[type=checkbox]").bootstrapSwitch("size", "mini");
        var request = {
            command: "getInfo",
            data: ["staInfo", "softapInfo"]
        };
        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                network_wireless.info = o.data;
                network_wireless.initStation();
                network_wireless.initSoftap();
            } else {
                showAlert("danger", "<strong>Try Again!</strong>  Get station enable status failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $(".eye_ctrl").on("click", function() {
            $(this).toggleClass("image-eye-on image-eye-off");
            var type = $(this).hasClass("image-eye-on") ? "text" : "password";
            $(this).parent().find("input").attr("type", type);
        });
        $("[name=station_wpsMode]").on("change", function() {
            network_wireless.validate("station");
        });
        $(".tab-pane input").on("keyup", function() {
            network_wireless.validate(this.id.split("_")[0]);
        });
        $("#station_scan").on("click", function() {
            var request = {
                command: "getScan"
            };
            $("#station_scan").prop("disabled", true);
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                $("#station_scan").prop("disabled", false);
                if (o.status === "succeed") {
                    $("#station_apList tbody").empty();
                    network_wireless.wifiApList = o.data;
                    $.each(o.data, function(i, ap) {
                        network_wireless.addAp(i, ap);
                    });
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Get AP list failed.");
                }
            }, "json");
        });
        $("#station_auth,#softAp_auth").on("change", function() {
            var tab = this.id.split("_")[0];
            var mode = +$("#" + tab + "_auth").val();
            if (mode > 1) {
                $("#" + tab + "_wepTr").hide();
                $("#" + tab + "_wpaTr").show();
                $("#" + tab + "_passwordTr").show();
            } else if (mode == 1) {
                $("#" + tab + "_wepTr").show();
                $("#" + tab + "_wpaTr").hide();
                $("#" + tab + "_passwordTr").show();
            } else {
                $("#" + tab + "_wepTr").hide();
                $("#" + tab + "_wpaTr").hide();
                $("#" + tab + "_passwordTr").hide();
            }
            network_wireless.validate(tab);
        });
        $("#station_save").on("click", function() {
            var request = {
                command: "setInfo",
                data: {
                    staInfo: { "enable": 1}
                }
            };
            if ($("#station_wpsEnable").bootstrapSwitch("state")) {
                request.data.staInfo.wps_enable = 1;
                request.data.staInfo.wps_pbc = $("[name=station_wpsMode]:eq(0)").prop("checked") ? 1 : 0;
                request.data.staInfo.wps_pin = $("[name=station_wpsMode]:eq(1)").prop("checked") ? $("#station_wpsServerPin").val() : ($("[name=station_wpsMode]:eq(2)").prop("checked") ? $("#station_wpsClientPin").val() : "");
            } else {
                request.data.staInfo.essid = $("#station_essid").val();
                request.data.staInfo.auth = +$("#station_auth").val();
                if (request.data.staInfo.auth > 1) {
                    request.data.staInfo.cipher = +$("#station_cipher").val();
                    request.data.staInfo.password = $("#station_password").val();
                } else if (request.data.staInfo.auth === 1) {
                    request.data.staInfo.index = +$("#station_index").val();
                    request.data.staInfo.password = $("#station_password").val();
                }
            }
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  Connected to " + $("#station_essid").val() + ".");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Fail to connect " + $("#station_essid").val() + ".");
            }, "json");
        });
        $("#station_wpsGenerate").on("click", function() {
            var request = {
                command: "creatPin",
                mode: 0
            };
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed")
                    $("#station_wpsServerPin").val(o.pinCode);
            }, "json");
        });
        $("#station_wpsEnable").on("switchChange.bootstrapSwitch", function() {
            if ($(this).bootstrapSwitch("state")) {
                $(".station_wps").show();
                $(".station_noWps").hide();
                network_wireless.validate("station");
            } else {
                $(".station_wps").hide();
                $(".station_noWps").show();
                $("#station_auth").trigger("change");
            }
        });
        $("#softAp_enable").on("switchChange.bootstrapSwitch", function() {
            $("#tab_softAp table input, #tab_softAp table select").prop("disabled", !$(this).bootstrapSwitch("state"));
            $("#softAp_hide").bootstrapSwitch("disabled", !$(this).bootstrapSwitch("state"));
        });
        $("#softAp_save").on("click", function() {
            if (network_wireless.info.softapInfo.enable === +$("#softAp_enable").bootstrapSwitch("state")) {
                $("#doJob").trigger("click");
                $("#softAp_save").prop("disabled", true);
            } else {
                $("#waitModal").modal("show");
            }
        });
        $("#doJob").on("click", function() {
            var request = {
                command: "setInfo",
                data: {
                    softapInfo: {
                        enable: +$("#softAp_enable").bootstrapSwitch("state"),
                        essid: $("#softAp_essid").val(),
                        hide: +$("#softAp_hide").bootstrapSwitch("state"),
                        channel: +$("#softAp_channel").val(),
                        protocol: +$("#softAp_protocol").val(),
                        auth: +$("#softAp_auth").val()
                    }
                }
            };
            if (request.data.softapInfo.auth > 1) {
                request.data.softapInfo.cipher = +$("#softAp_cipher").val();
                request.data.softapInfo.password = $("#softAp_password").val();
            } else if (request.data.softapInfo.auth === 1) {
                request.data.softapInfo.index = +$("#softAp_index").val();
                request.data.softapInfo.password = $("#softAp_password").val();
            }
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    if (network_wireless.info.softapInfo.enable !== +$("#softAp_enable").bootstrapSwitch("state"))
                        utils.reboot(30 * 1000);
                    else
                        showAlert("success", "<strong>Success!</strong>  SoftAp config applied.");
                } else {
                    $("#waitModal").modal("hide");
                    showAlert("danger", "<strong>Try Again!</strong>  SoftAp config apply failed.");
                }
                if (network_wireless.info.softapInfo.enable === +$("#softAp_enable").bootstrapSwitch("state"))
                    $("#softAp_save").prop("disabled", false);
            }, "json");
        });
    }
};

$(function() {
    network_wireless.initParams();
    network_wireless.bindEvent();
});

