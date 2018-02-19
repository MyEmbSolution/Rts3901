var network_pppoe = {
    currentState: 0,
    initParams: function() {
        $("#pppoe_enable").bootstrapSwitch("size", "mini");
        var request = {
            "command": "getInfo",
            "data": ["pppoeInfo"]
        };
        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#pppoe_enable").bootstrapSwitch("state", o.data.pppoeInfo.enable);
                network_pppoe.currentState = o.data.pppoeInfo.enable;
                $("tbody input, tbody select").prop("disabled", !o.data.pppoeInfo.enable);
                if (o.data.pppoeInfo.enable) {
                    $("#pppoe_username").val(o.data.pppoeInfo.username);
                    $("#pppoe_password").val(o.data.pppoeInfo.password);
                    $("#pppoe_status").text(o.data.pppoeInfo.status ? "connected" : "connecting");
                    $("#pppoe_ipaddr").text(o.data.pppoeInfo.ipaddr);
                }
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get PPPoE config failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $("#reboot").on("click", function() {
            utils.reboot(30 * 1000);
        });
        $(".eye_ctrl").on("click", function() {
            $(this).toggleClass("image-eye-on image-eye-off");
            var type = $(this).hasClass("image-eye-on") ? "text" : "password";
            $(this).parent().find("input").attr("type", type);
        });
        $("#pppoe_enable").on("switchChange.bootstrapSwitch", function() {
            $("tbody input, tbody select").prop("disabled", !$(this).bootstrapSwitch("state"));
        });
        $("#pppoe_save").on("click", function() {
            if (+$("#pppoe_enable").bootstrapSwitch("state") === network_pppoe.currentState) {
                $("#doJob").trigger("click");
                $("#save").prop("disabled", true);
            } else {
                $("#waitModal").modal("show");
            }
        });
        $("#doJob").on("click", function() {
            var request = {
                "command": "setInfo",
                "data": {
                    "pppoeInfo": {
                        "enable": +$("#pppoe_enable").bootstrapSwitch("state"),
                        "username": $("#pppoe_username").val(),
                        "password": $("#pppoe_password").val()
                    }
                }
            };
            $("#pppoe_save").prop("disabled", true);
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                $("#pppoe_save").prop("disabled", false);
                if (o.status === "succeed") {
                    if (+$("#pppoe_enable").bootstrapSwitch("state") === network_pppoe.currentState) {
                        showAlert("success", "<strong>Success!</strong>  PPPoE config saved.");
                    } else {
                        utils.reboot(30 * 1000);
                    }
                } else if (o.status === "failed") {
                    $("#waitModal").modal("hide");
                    showAlert("danger", "<strong>Try Again!</strong>  Config save failed.");
                }
            }, "json");
        });
    }
};

$(function() {
    network_pppoe.initParams();
    network_pppoe.bindEvent();
});