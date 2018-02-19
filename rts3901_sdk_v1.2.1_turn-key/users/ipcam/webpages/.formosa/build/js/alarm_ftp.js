var alarm_ftp = {
    validate: function() {
        var valid = regex.ip.test($("#ip").val())
                 && +$("#port").val() > 0
                 && +$("#port").val() < 65536
                 && !!$("#account").val().length
                 && !!$("#password").val().length;
        $("#btn_save").prop("disabled", !valid);
    },
    initParams: function() {
        var request = {
            "command": "getFtpParam"
        };
        $.post("/cgi-bin/ftp.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#ip").val(o.data.ip);
                $("#port").val(o.data.port);
                $("#account").val(o.data.account);
                $("#password").val(o.data.password);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get FTP config failed.");
            }
            alarm_ftp.validate();
        }, "json");
    },
    bindEvent: function() {
        $("table#ftp input").on("keyup", function() {
            alarm_ftp.validate()
        });

        $("#btn_save").on("click", function() {
            var request = {
                    "command": "setFtpParam",
                    "data": {
                        "ip": $("#ip").val(),
                        "port": $("#port").val(),
                        "account": $("#account").val(),
                        "password": $("#password").val()
                    }
                };

            $.post("/cgi-bin/ftp.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    showAlert("success", "<strong>Success!</strong>  FTP config saved.");
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  FTP config save failed.");
                }
                alarm_ftp.validate();
            }, "json");
        });

    }
}

$(function() {
    alarm_ftp.initParams();
    alarm_ftp.bindEvent();
});
