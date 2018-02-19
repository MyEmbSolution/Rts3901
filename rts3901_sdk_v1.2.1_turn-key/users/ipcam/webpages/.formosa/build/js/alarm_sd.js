var alarm_sd = {
    mid_path: "",
    fileName: "",
    getSize: function(size) {
        if (size.indexOf("Byte") > 0)
            return Number(size.replace("Byte", ""));
        else if (size.indexOf("KB") > 0)
            return Number(size.replace("KB", "")) * 1024;
        else if (size.indexOf("MB") > 0)
            return Number(size.replace("MB", "")) * 1024 * 1024;
        else if (size.indexOf("GB") > 0)
            return Number(size.replace("GB", "")) * 1024 * 1024 * 1024;
    },
    getInfo: function() {
        var request = {
            command: "getInfo"
        };
        $.post("/cgi-bin/storage.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#totalSize").text(o.total);
                $("#freeSize").text(o.free);
                var totalSize = alarm_sd.getSize(o.total);
                var freeSize = alarm_sd.getSize(o.free);
                var sd_capacity = (totalSize - freeSize) / totalSize * 100;
                $("#storage_capability").css("width", sd_capacity + "%");
                for (var i = 1; i <= 3; i++)
                    alarm_sd.getRemainingTime(freeSize, i);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get sd info failed.");
            }
        }, "json");
    },
    getRemainingTime: function(freeSpace, index) {
        var request = {
            command: "getParam",
            stream: "profile" + index,
            data: ["bitRate"]
        };
        $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                var leftTime = freeSpace * 8 / o.data.bitRate / 3600;
                $("#leftTime" + index).text(leftTime.toFixed(2) + " hours");
            } else if (o.status === "failed") {
               showAlert("danger", "<strong>Try Again!</strong>  Get stream param failed.");
            }
        }, "json");
    },
    listFile: function() {
        var request = {
            command: "listFile",
            data: alarm_sd.mid_path
        };
        $.post("/cgi-bin/storage.cgi", JSON.stringify(request), function(o) {
            var type;
            $("#fileTree").empty();
            if (o.status === "succeed") {
                $("<li>").text("Parent Directory").addClass("list-parent-dir").addClass("list-group-item").appendTo($("#fileTree"));
                var compare = function(obj1, obj2) {
                    var name1 = obj1.name;
                    var name2 = obj2.name;
                    if (name1 < name2)
                        return -1;
                    else if (name1 > name2)
                        return 1;
                    else
                        return 0;
                }
                var sortedData = o.data.sort(compare);
                $.each(sortedData, function(i, ent) {
                    var ent_text;
                    if (ent.type === "file") {
                        if (ent.name.split(".mp4").length > 1)
                            type = "list-video";
                        else if (ent.name.split(".png").length > 1 || ent.name.split(".jpg").length > 1)
                            type = "list-image";
                        else
                            type = "list-unsupported";
                        ent_text = ent.name;
                    } else if (ent.type === "directory") {
                        type = "list-directory";
                        ent_text = alarm_sd.mid_path + "/" + ent.name;
                    }
                    $("<li>").text(ent_text).addClass(type).addClass("list-group-item").appendTo($("#fileTree"));
                });
                $("li").on("click", function() {
                    $(this).prop("disabled", true);
                    switch ($(this).attr("class").replace(" list-group-item", "")) {
                        case "list-parent-dir":
                            var lastIndex = alarm_sd.mid_path.lastIndexOf("/");
                            alarm_sd.mid_path = alarm_sd.mid_path.substring(0, lastIndex);
                            alarm_sd.listFile();
                            $("#remove").prop("disabled", true);
                            break;
                        case "list-directory":
                            alarm_sd.mid_path = $(this).text();
                            alarm_sd.listFile();
                            $("#remove").prop("disabled", true);
                            break;
                        case "list-video":
                            $("#preview").removeAttr("style");
                            addVideoPlugin("#preview", "http://admin:123456@" + document.location.host + "/sdcard/" + alarm_sd.mid_path + "/" + $(this).text(), localStorage.plugin,[]);
                            alarm_sd.fileName = $(this).text();
                            $("#remove").prop("disabled", false);
                            break;
                        case "list-image":
                            $("#preview").html("");
                            alarm_sd.fileName = $(this).text();
                            var img = new Image;
                            img.onload = function() {
                                var factor1 = img.width / $("#preview").width();
                                var factor2 = img.height / $("#preview").height();
                                if (factor1 > factor2)
                                    $("#preview").css({
                                        "width": $("#preview").width(),
                                        "height": (img.height / factor1)
                                    });
                                else
                                    $("#preview").css({
                                        "width": (img.width / factor2),
                                        "height": $("#preview").height()
                                    });
                            };
                            img.src = "/sdcard/" + alarm_sd.mid_path + "/" + $(this).text();
                            $("#preview").css({
                                "backgroundSize": "100% 100%",
                                "backgroundImage": "url(/sdcard/" + alarm_sd.mid_path + "/" + $(this).text() + ")"
                            });
                            $("#remove").prop("disabled", false);
                            break;
                    }
                    $(this).prop("disabled", false);
                });
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  List files failed.");
            }
        }, "json");
    },
    initParams: function() {
        $.ajaxSetup("cache", false);
        alarm_sd.getInfo();
        alarm_sd.listFile();
    },
    bindEvent: function() {
        $("parentDir").on("click", function() {
            alarm_sd.mid_path += $(this).text();
            alarm_sd.listFile();
        });
        $("#remove").on("click", function() {
            $("#preview").html("");
            var request = {
                command: "removeFile",
                data: alarm_sd.mid_path + "/" + alarm_sd.fileName
            };
            $.post("/cgi-bin/storage.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    $("#remove").prop("disabled", true);
                    alarm_sd.listFile();
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Remove file failed.");
                }
            }, "json");
        });
        $("#format").on("click", function() {
            $("#format").prop("disabled", true);
            var request = {
                command: "format"
            };
            $.post("/cgi-bin/storage.cgi", JSON.stringify(request), function(o) {
                $("#format").prop("disabled", false);
                if (o.status === "succeed") {
                    alarm_sd.getInfo();
                    alarm_sd.listFile();
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Format sd failed.");
                }
            }, "json");
        });
    }
};

$(function() {
    alarm_sd.initParams();
    alarm_sd.bindEvent();
});
