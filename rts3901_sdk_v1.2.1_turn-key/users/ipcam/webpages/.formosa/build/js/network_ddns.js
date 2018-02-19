var network_ddns = {
    initParams: function() {
        $("#ddns_enable").bootstrapSwitch("size", "mini");
        var request = {
            "command": "getInfo",
            "data": ["ddnsInfo"]
        };
        $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#ddns_enable").bootstrapSwitch("state", o.data.ddnsInfo.enable);
                $("tbody input, tbody select").prop("disabled", !o.data.ddnsInfo.enable);
                $("#ddns_type").val(o.data.ddnsInfo.type);
                $("#ddns_domain").val(o.data.ddnsInfo.domain);
                $("#ddns_username").val(o.data.ddnsInfo.username);
                $("#ddns_password").val(o.data.ddnsInfo.password);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get DDNS config failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $(".eye_ctrl").on("click", function() {
            $(this).toggleClass("image-eye-on image-eye-off");
            var type = $(this).hasClass("image-eye-on") ? "text" : "password";
            $(this).parent().find("input").attr("type", type);
        });
        $("#ddns_enable").on("switchChange.bootstrapSwitch", function() {
            $("tbody input, tbody select").prop("disabled", !$(this).bootstrapSwitch("state"));
        });
        $("#ddns_save").on("click", function() {
            var request = {
                "command": "setInfo",
                "data": {
                    "ddnsInfo": {
                        "enable": +$("#ddns_enable").bootstrapSwitch("state"),
                        "type": +$("#ddns_type").val(),
                        "domain": $("#ddns_domain").val(),
                        "username": $("#ddns_username").val(),
                        "password": $("#ddns_password").val()
                    }
                }
            };
            $("#ddns_save").prop("disabled", true);
            $.post("/cgi-bin/network.cgi", JSON.stringify(request), function(o) {
                $("#ddns_save").prop("disabled", false);
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  DDNS config saved.");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Config save failed.");
            }, "json");
        });
    }
};

$(function() {
    network_ddns.initParams();
    network_ddns.bindEvent();
});
