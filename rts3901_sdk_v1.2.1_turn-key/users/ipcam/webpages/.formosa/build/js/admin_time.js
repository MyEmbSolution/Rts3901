var admin_time = {
    timezone: [
        ["(GMT-11:00) 阿拉斯加诺姆", "NT", ""],
        ["(GMT-10:00) 夏威夷-阿拉斯加", "AHST", ""],
        ["(GMT-09:00) 阿拉斯加", "AKST", "PDT"],
        ["(GMT-08:00) 太平洋时间(美国和加拿大)", "PST", "PDT"],
        ["(GMT-07:00) 山地时间(美国和加拿大)", "MST", "PDT"],
        ["(GMT-06:00) 中部时间(美国和加拿大)", "CST", "PDT"],
        ["(GMT-05:00) 东部时间(美国和加拿大)", "EST", "PDT"],
        ["(GMT-04:00) 大西洋时间(加拿大)", "AST", ""],
        ["(GMT-03:00) 巴西利亚", "BRT", "PDT"],
        ["(GMT-02:00) 纽芬兰", "NDT", ""],
        ["(GMT-01:00) 西非", "WAT", "PDT"],
        ["(GMT) 格林威治标准时间: 伦敦", "UTC", "PDT"],
        ["(GMT+01:00) 马德里，巴黎,阿姆斯特丹，柏林，罗马", "CET", "PDT"],
        ["(GMT+02:00) 瑞典夏时制", "SST", ""],
        ["(GMT+03:00) 希腊地中海", "HMT", ""],
        ["(GMT+04:00) 毛里求斯", "MUT", ""],
        ["(GMT+05:00) 马尔代夫", "MVT", ""],
        ["(GMT+06:00) 新西伯利亚", "ALMT", ""],
        ["(GMT+07:00) 曼谷，河内，雅加达", "CXT", ""],
        ["(GMT+08:00) 北京,新加坡,台北", "CCT", ""],
        ["(GMT+09:00) 首尔,东京", "JST", ""],
        ["(GMT+10:00) 关岛，墨尔本，悉尼", "LIGT", "PDT"],
        ["(GMT+11:00) 澳大利亚东部标准夏时制", "AESST", ""],
        ["(GMT+12:00) 斐济", "NZT", ""],
    ],
    initBasic: function() {
        $.ajaxSetup("cache", false);
        for (var i = 0; i < 24; i++)
            $("#timezone").append(new Option(admin_time.timezone[i][0],
                admin_time.timezone[i][1]));
        $("input[type=checkbox]").bootstrapSwitch("size", "mini");
        var request = {
            command: "getParam"
        };
        $.post("/cgi-bin/time.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#timezone option:eq(" + (11 - Number(o.data.timezone.substr(3))) + ")").prop("selected", true);
                admin_time.showDst();
                $.each(o.data.builtInNtpServer, function(i, server) {
                    $("#ntpServer").append(new Option(server));
                });
                $("#ntpServer").append(new Option("Custom", "custom"));
                if (o.data.ntpType === "builtIn") {
                    $("#ntpServer").val(o.data.currentNtpServer);
                    $("#customNtpServer").remove();
                } else if (o.data.ntpType === "custom") {
                    $("#ntpServer").val("custom");
                    $("#ntpServer").after($("<input>").attr("id", "customNtpServer").val(o.data.currentNtpServer));
                }
                $("#ntpEnable").bootstrapSwitch("state", o.data.ntpEnable);
                $("#dstEnable").bootstrapSwitch("state", o.data.dstEnable);
                if (o.data.ntpEnable) {
                    admin_time.showNtpConfig();
                } else {
                    admin_time.showCustomTime();
                }
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get time config failed.");
            }
        }, "json");
    },
    validate: function() {
        var valid = true;
        valid = valid && regex.date.test($("#customTime").val().split(" ")[0]);
        valid = valid && regex.time.test($("#customTime").val().split(" ")[1]);
        $("#save").prop("disabled", !valid);
    },
    showNtpConfig: function() {
        $("#forNtp").show();
        $("#noNtp").hide();
    },
    showCustomTime: function() {
        $("#forNtp").hide();
        $("#noNtp").show();
        var date = new Date();
        $("#customTime").val(date.getFullYear() + "-" + (date.getMonth() + 1) + "-" + date.getDate() + " " + date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds());
        admin_time.validate();
    },
    showDst: function() {
        var index = $("#timezone").prop("selectedIndex");
        if (admin_time.timezone[index][2] === "PDT")
            $("#dstTr").show();
        else
            $("#dstTr").hide();
    },
    bindEvent: function() {
        $("#timezone").on("change", function() {
            admin_time.showDst();
        });
        $("#ntpServer").on("change", function() {
            if ($("#ntpServer").val() === "custom")
                $("#ntpServer").after($("<input>").attr("id", "customNtpServer"));
            else
                $("#customNtpServer").remove();
        });
        $("#customTime").on("keyup", function() {
            admin_time.validate();
        });
        $("#ntpEnable").on("switchChange.bootstrapSwitch", function() {
            if ($("#ntpEnable").bootstrapSwitch("state")) {
                admin_time.showNtpConfig();
            } else {
                admin_time.showCustomTime();
            }
        });
        $("#save").on("click", function() {
            var request = {
                command: "setTimezone",
                timezone: $("#timezone").val(),
                dstEnable: $("#dstEnable").bootstrapSwitch("state")
            };
            $.post("/cgi-bin/time.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    var request = {
                        command: "setNtpServer",
                        ntpEnable: $("#ntpEnable").bootstrapSwitch("state"),
                        ntpType: $("#ntpServer").val() === "custom" ? "custom" : "builtIn",
                        server: $("#ntpServer").val() === "custom" ? $("#customNtpServer").val() : $("#ntpServer").val()
                    };
                    $.post("/cgi-bin/time.cgi", JSON.stringify(request), function(o) {
                        if (o.status === "succeed") {
                            if ($("#ntpEnable").bootstrapSwitch("state")) {
                                showAlert("success", "<strong>Success!</strong>  Time config saved.");
                            } else {
                                var request = {
                                    command: "setTime",
                                    data: $("#customTime").val().replace(/[ :]/g, "-")
                                }
                                $.post("/cgi-bin/time.cgi", JSON.stringify(request), function(o) {
                                    if (o.status === "succeed")
                                        showAlert("success", "<strong>Success!</strong>  Time config saved.");
                                    else if (o.status === "failed") {
                                        showAlert("danger", "<strong>Try Again!</strong>  Time config save failed.");
                                    }
                                }, "json");
                            }
                        } else if (o.status === "failed") {
                            showAlert("danger", "<strong>Try Again!</strong>  Time config save failed.");
                        }
                    }, "json");
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Time config save failed.");
                }
            }, "json");
        });
    }
};

$(function() {
    admin_time.initBasic();
    admin_time.bindEvent();
});
