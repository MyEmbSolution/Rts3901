var camera_stream = {
    bitrate: ["64K", "128K", "256K", "384K", "512K", "768K", "1M", "1.5M", "2M", "2.5M", "3M", "4M"],
    bitrate_mode: ["CBR", "CVBR"],
    bitrate_max: ["64K", "128K", "256K", "384K", "512K", "768K", "1M", "1.5M", "2M", "2.5M", "3M", "4M"],
    bitrate_min: ["64K", "128K", "256K", "384K", "512K", "768K", "1M", "1.5M", "2M", "2.5M", "3M", "4M"],
    showBitrate: function(id, value) {
        var existed = false;
        $.each($(id + " option"), function(i, option) {
            if (option.value == value) {
                existed = true;
                return false;
            }
        });
        if (!existed) {
            if (value < 1024)
                text = value + "B";
            else
                text = Math.floor(value / 1024) + "K";
            $(id).append(new Option(text, value));
        }
        $(id).val(value);
    },
    getStream: function(index) {
        var table = ''
            + '<table class="table column2">'
            + '  <thead><tr><th colspan="2">Stream ' + index + '</th></tr></thead>'
            + '  <tbody>'
            + '    <tr><th>Resolution</th><td><select id="resolution_' + index + '"></select></td></tr>'
            + '    <tr><th>Frame Rate</th><td><select id="fps_' + index + '"></select></td></tr>'
            + '    <tr><th>Bitrate Mode</th><td><select id="bitrateMode_' + index + '"></select></td></tr>'
            + '    <tr id="bitrate_cbr_' + index + '"><th>Bitrate</th><td><select id="bitRate_' + index + '"></select></td></tr>'
            + '    <tr id="bitrate_cvbr_max_' + index + '"><th>Bitrate Max</th><td><select id="bitrateMax_' + index + '"></select></td></tr>'
            + '    <tr id="bitrate_cvbr_min_' + index + '"><th>Bitrate Min</th><td><select id="bitrateMin_' + index + '"></select></td></tr>'
            + '    <tr><th>GOP</th><td><select id="gop_' + index + '"></select></td></tr>'
            + '    <tr><th>H264 Profile</th><td><select id="profile_' + index + '"></select></td></tr>'
            + '    <tr><th>H264 Level</th><td><select id="level_' + index + '"></select></td></tr>'
            + '  </tbody>'
            + '</table>';
        $("#stream_table_container").append(table);
	 $.each(camera_stream.bitrate_mode, function(i, mode) {
            $("#bitrateMode_" + index).append(new Option(mode));
        });
        $.each(camera_stream.bitrate, function(i, bitrate) {
            var value = 0;
            if (bitrate.split("K").length > 1)
                value = bitrate.split("K")[0] * 1024;
            else if (bitrate.split("M").length > 1)
                value = bitrate.split("M")[0] * 1024 * 1024;
            $("#bitRate_" + index).append(new Option(bitrate, value));
        });
	$.each(camera_stream.bitrate_max, function(i, bitrate) {
            var value = 0;
            if (bitrate.split("K").length > 1)
                value = bitrate.split("K")[0] * 1024;
            else if (bitrate.split("M").length > 1)
                value = bitrate.split("M")[0] * 1024 * 1024;
            $("#bitrateMax_" + index).append(new Option(bitrate, value));
        });
	$.each(camera_stream.bitrate_min, function(i, bitrate) {
            var value = 0;
            if (bitrate.split("K").length > 1)
                value = bitrate.split("K")[0] * 1024;
            else if (bitrate.split("M").length > 1)
                value = bitrate.split("M")[0] * 1024 * 1024;
            $("#bitrateMin_" + index).append(new Option(bitrate, value));
        });
        var request = {
            "command": "getCapability",
            "stream": "profile" + index,
            "data": []
        };
        $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#resolution_" + index).empty();
                $.each(o.data.resolution.data, function(i, resolution) {
                    $("#resolution_" + index).append(new Option(resolution));
                });
                $("#fps_" + index).empty();
                $.each(o.data.fps.data, function(i, fps) {
                    if (fps > 5)
                        $("#fps_" + index).append(new Option(fps));
                });
                for (var i = o.data.gop.data.min; i <= o.data.gop.data.max;)
                {
                    if (i <= 120)
                        $("#gop_" + index).append(new Option(i));
                    i += o.data.gop.data.step;
                }
                var request = {
                    "command": "getParam",
                    "stream": "profile" + index,
                    "data": []
                };
                $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
                    if (o.status === "succeed") {
                        $("#resolution_" + index).val(o.data.resolution);
                        $("#fps_" + index).val(o.data.fps);
                        $("#gop_" + index).val(o.data.gop);
                        $("#bitrateMode_" + index).val(o.data.bitrateMode);
                        camera_stream.showBitrate("#bitRate_" + index, o.data.bitRate);
                        camera_stream.showBitrate("#bitrateMax_" + index, o.data.bitrateMax);
                        camera_stream.showBitrate("#bitrateMin_" + index, o.data.bitrateMin);
                        if (o.data.bitrateMode === "CBR") {
                            $("#bitrate_cbr_" + index).show();
                            $("#bitrate_cvbr_max_" + index).hide();
                            $("#bitrate_cvbr_min_" + index).hide();
                        } else {
                            $("#bitrate_cvbr_max_" + index).show();
                            $("#bitrate_cvbr_min_" + index).show();
                            $("#bitrate_cbr_" + index).hide();
                        }

                    } else if (o.status === "failed") {
                        showAlert("danger", "<strong>Try Again!</strong>  Get stream param failed.");
                    }
                }, "json");
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get stream capability failed.");
            }
        }, "json");
        var request = {
            "command": "getCapability",
            "stream": "profile" + index,
            "data": ["profile", "level"]
        };
        $.post("/cgi-bin/h264.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#profile_" + index).empty();
                $("#level_" + index).empty();
                $.each(o.data.profile.data, function(i, profile) {
                    $("#profile_" + index).append(new Option(profile));
                });
                $.each(o.data.level.data, function(i, level) {
                    $("#level_" + index).append(new Option(level));
                });
                var request = {
                    "command": "getParam",
                    "stream": "profile" + index,
                    "data": ["profile", "level"]
                };
                $.post("/cgi-bin/h264.cgi", JSON.stringify(request), function(o) {
                    if (o.status === "succeed") {
                        $("#profile_" + index).val(o.data.profile);
                        $("#level_" + index).val(o.data.level);
                    } else {
                        showAlert("danger", "<strong>Try Again!</strong>  Get H264 config failed.");
                    }
                }, "json");
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get H264 config failed.");
            }
        }, "json");
    },
    initParams: function() {
        $.ajaxSetup("cache", false);
        $('a[data-toggle="tab"]').on("click", function() {
            var tab = this.href.split("#")[1].split("_")[1];
            if (tab === "stream")
                camera_stream.initTabStream();
            else if (tab === "rtsp")
                camera_stream.initTabRtsp();
        });
    },
    initTabStream: function() {
        var request = {
            "command": "getStreamList"
        };
        $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                var num = o.data.length;
                $("#stream_table_container").empty();
                for (var i = 1; i <= num; i++)
                    camera_stream.getStream(i);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get stream list failed.");
            }
        }, "json");
    },
    initTabRtsp: function() {
        var request = {
            "command": "getRtspInfo",
            "data": []
        };
        $.post("/cgi-bin/rtsp.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#max_connections").val(o.data.max_connections);
                $("#port").val(o.data.port);
                $("#" + o.data.auth).prop("checked", true);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get rtsp config failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $("#stream_table_container").on("change", "select",function() {
            var request = {
                "command": "setParam",
                "stream": "profile" + this.id.split("_")[1],
                "data": {}
            };
            var target = "/cgi-bin/stream.cgi";
            var param = this.id.split("_")[0];
            if (param === "bitrateMode" || param === "bitRate" || param === "bitrateMax" || param === "bitrateMin") {
                if ($("#bitrateMode_"+ this.id.split("_")[1]).val() === "CBR") {
                    request.data["bitrateMode"] = "CBR";
                    request.data["bitRate"] = $("#bitRate_"+ this.id.split("_")[1]).val();
                } else {
                    request.data["bitrateMode"] = "CVBR";
                    request.data["bitrateMax"] = $("#bitrateMax_"+ this.id.split("_")[1]).val();
                    request.data["bitrateMin"] = $("#bitrateMin_"+ this.id.split("_")[1]).val();
                }
            } else {
                if (param === "resolution" || param === "profile" || param === "level")
                    request.data[param] = $(this).val();
                else
                    request.data[param] = +$(this).val();
            }
            if (param === "bitrateMode") {
                if ($(this).val() === "CBR") {
                    $("#bitrate_cbr_" + this.id.split("_")[1]).show();
                    $("#bitrate_cvbr_max_" + this.id.split("_")[1]).hide();
                    $("#bitrate_cvbr_min_" + this.id.split("_")[1]).hide();
                } else {
                    $("#bitrate_cvbr_max_" + this.id.split("_")[1]).show();
                    $("#bitrate_cvbr_min_" + this.id.split("_")[1]).show();
                    $("#bitrate_cbr_" + this.id.split("_")[1]).hide();
                }
            }
            if (param === "profile" || param === "level")
                target = "/cgi-bin/h264.cgi";

            $.post(target, JSON.stringify(request), function(o) {
                if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Set param " + param + " failed.");
            }, "json");
        });
	$("#max_connections, #port").on("keyup change", function() {
            $("#saveRtsp").prop("disabled", (+$("#max_connections").val() < 1) || (+$("#port").val() < 1) || (+$("#port").val() > 65535));
	});
        $("#saveRtsp").on("click", function() {
            var request = {
                "command": "setRtspInfo",
                "data": {
                    "max_connections": +$("#max_connections").val(),
                    "port": +$("#port").val(),
                    "auth": ($("#enable").prop("checked") ? "enable" : "disable")
                }
            };
            $("#saveRtsp").prop("disable", true);
            $.post("/cgi-bin/rtsp.cgi", JSON.stringify(request), function(o) {
                $("#saveRtsp").prop("disable", false);
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  Rtsp config saved.");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Rtsp config save failed.");
            }, "json");
        });
    }
}

$(function() {
    camera_stream.initParams();
    camera_stream.initTabStream();
    camera_stream.bindEvent();
});
