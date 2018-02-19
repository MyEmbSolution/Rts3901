var network_discovery = {
    validateBonjour: function() {
        if (!$("#bonjour_enable").bootstrapSwitch("state"))
            $("#bonjour_save").prop("disabled", false);
        else
            $("#bonjour_save").prop("disabled", !regex.hostname.test($("#bonjour_hostname").val()));
    },
    initBonjour: function() {
        var request = {
            "command": "getInfo",
            "data": ["bonjourInfo"]
        };
        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#bonjour_enable").bootstrapSwitch("state", o.data.bonjourInfo.enable);
                $("#bonjour_hostname").val(o.data.bonjourInfo.hostname).prop("disabled", !o.data.bonjourInfo.enable);
                network_discovery.validateBonjour();
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get Bonjour config failed.");
            }
        }, "json");
        $("#bonjour_enable").on("switchChange.bootstrapSwitch", function() {
            if ($(this).bootstrapSwitch("state")) {
                network_discovery.validateBonjour();
                $("#bonjour_hostname").prop("disabled", false);
            } else {
                $("#bonjour_save").prop("disabled", false);
                $("#bonjour_hostname").prop("disabled", true);
            }
        });
        $("#bonjour_hostname").on("keyup", function() {
            network_discovery.validateBonjour();
        });
        $("#bonjour_save").on("click", function() {
            var request = {
                "command": "setInfo",
                "data": {
                    "bonjourInfo": {
                        "enable": +$("#bonjour_enable").bootstrapSwitch("state"),
                        "hostname": $("#bonjour_hostname").val()
                    }
                }
            };
            $("#bonjour_save").prop("disabled", true);
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                $("#bonjour_save").prop("disabled", false);
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  Bonjour config saved.");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Config save failed.");
            }, "json");
        });
    },
    initUpnp: function() {
        var request = {
            "command": "getInfo",
            "data": ["upnpInfo"]
        };
        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#upnp_enable").bootstrapSwitch("state", o.data.upnpInfo.enable);
                $("#upnp_friendlyName").val(o.data.upnpInfo.friendlyName).prop("disabled", !o.data.upnpInfo.enable);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get UPnP config failed.");
            }
        }, "json");
        $("#upnp_enable").on("switchChange.bootstrapSwitch", function() {
            $("#upnp_friendlyName").prop("disabled", !$(this).bootstrapSwitch("state"));
        });
        $("#upnp_save").on("click", function() {
            var request = {
                "command": "setInfo",
                "data": {
                    "upnpInfo": {
                        "enable": +$("#upnp_enable").bootstrapSwitch("state"),
                        "friendlyName": $("#upnp_friendlyName").val()
                    }
                }
            };
            $("#upnp_save").prop("disabled", true);
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                $("#upnp_save").prop("disabled", false);
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  UPnP config saved.");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Config save failed.");
            }, "json");
        });
    }
};

$(function() {
    $('a[data-toggle="tab"]').on("click", function() {
        network_discovery["init" + this.href.split("#")[1].split("_")[1]]();
        $(this).tab("show");
    });
    $("#bonjour_enable,#upnp_enable").bootstrapSwitch("size", "mini");
    network_discovery.initUpnp();
});
