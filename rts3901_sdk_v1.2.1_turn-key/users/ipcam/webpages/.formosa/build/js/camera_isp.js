var camera_isp = {
    port: 43794,
    backup: {},
    fillParams: function (params) {
        $.each(params, function (id, value) {
            var param_entry = $("#" + id);
            switch (id) {
                case "whiteBalance":
                    $("#autoWhiteBalance").bootstrapSwitch("state", !value);
                    if (value) {
                        $("#manualWhiteBalance").show().parent().parent().css("height", "66px");
                        $("#whiteBalance").val(value).next("span").text(value);
                    } else {
                        $("#manualWhiteBalance").hide().parent().parent().css("height", "44px");
                    }
                    break;
                case "mirror":
                case "flip":
		  case "ir":
                case "ldc":
                case "3dnr":
                case "dehaze":
                    param_entry.bootstrapSwitch("state", value);
                    break;
                case "gamma":
                case "brightness":
                case "contrast":
                case "saturation":
                case "sharpness":
                case "wdrLevel":
                    param_entry.val(value);
                    param_entry.next("span").text(value);
                    break;
                case "wdrMode":
                    param_entry.val(value);
                    $("#wdrEntry").css("height", value === 4 ? "73px" : "44px");
                    value === 4 ? $("#manualWdrLevel").show() : $("#manualWdrLevel").hide();
                    break;
                default:
                    param_entry.val(value);
            }
        });
    },
    setParam: function (element) {
        var request = {
            command: "setParam",
            data: {}
        };
        var id = element.id;
        var value;
        switch (id) {
            case "mirror":
	     case "ir":
            case "flip":
            case "ldc":
            case "3dnr":
            case "dehaze":
                value = $(element).bootstrapSwitch("state");
                break;
            case "whiteBalance":
            case "autoWhiteBalance":
                id = "whiteBalance";
                var autoWhiteBalance = $("#autoWhiteBalance");
                var whiteBalance = $("#whiteBalance");
                value = autoWhiteBalance.bootstrapSwitch("state") ? 0 : +whiteBalance.val();
                if (autoWhiteBalance.bootstrapSwitch("state")) {
                    $("#manualWhiteBalance").hide().parent().parent().css("height", "44px");
                } else {
                    $("#manualWhiteBalance").show().parent().parent().css("height", "66px");
                    whiteBalance.next("span").text(value);
                }
                break;
            case "gamma":
            case "brightness":
            case "contrast":
            case "saturation":
            case "sharpness":
            case "wdrLevel":
                value = +$(element).val();
                $(element).next("span").text(value);
                break;
            case "rotate":
                value = +$(element).val();
                break;
            case "wdrMode":
                value = +$(element).val();
                $("#wdrEntry").css("height", value === 4 ? "73px" : "44px");
                value === 4 ? $("#manualWdrLevel").show() : $("#manualWdrLevel").hide();
                break;
            default:
                value = $(element).val();
        }
        request.data[id] = value;
        $("input[type!=checkbox],select").prop("disabled", true);
        $("input[type=checkbox]").bootstrapSwitch("disabled", true);
        $.post("/cgi-bin/isp.cgi", JSON.stringify(request), function (o) {
            if (o.status === "succeed") {
                camera_isp.backup[id] = value;
            } else if (o.status === "failed") {
                camera_isp.fillParams(camera_isp.backup);
                showAlert("danger", "<strong>Try Again!</strong>  Set param " + id + " failed.");
            }
            $("input[type!=checkbox],select").prop("disabled", false);
            $("input[type=checkbox]").bootstrapSwitch("disabled", false);
        }, "json");
    },
    initParams: function () {
        $.ajaxSetup("cache", false);
        if (!window.localStorage.protocol)
            localStorage.protocol = "udp";
        if (!window.localStorage.caching)
            localStorage.caching = "1000";
        if (!window.localStorage.stream)
            localStorage.stream = "profile1";
        $("#mirror,#flip,#osdEnable,#corridor,#3dnr,#ldc,#dehaze,#ir").bootstrapSwitch("size", "mini");
        $("#autoWhiteBalance").bootstrapSwitch({
            "onText": "Auto",
            "offText": "Manual",
            "size": "mini"
        });
        var request = {
            command: "getCapability",
            data: []
        };
        $.post("/cgi-bin/isp.cgi", JSON.stringify(request), function (o) {
            if (o.status === "succeed") {
                $("#antiFlicker").empty();
                $.each(o.data.antiFlicker.data, function (i, value) {
                    $("#antiFlicker").append(new Option(value.toUpperCase(), value));
                });
                $.each(o.data, function (field, capa) {
                    if (capa.type === "step" && field !== "rotate" && field !== "wdrMode") {
                        $("#" + field).attr({
                            "min": capa.data.min,
                            "max": capa.data.max,
                            "step": capa.data.step
                        });
                    }
                });
                if (o.data.ldc)
                    $("#param_menu_entry_ldc").show();
                request.command = "getParam";
                request.data = [];
                $.post("/cgi-bin/isp.cgi", JSON.stringify(request), function (o) {
                    if (o.status === "succeed") {
                        camera_isp.backup = o.data;
                        var checkbox = $("input[type=checkbox]");
                        checkbox.off("switchChange.bootstrapSwitch");
                        camera_isp.fillParams(o.data);
                        checkbox.on("switchChange.bootstrapSwitch", function () {
                            camera_isp.setParam(this);
                        });
                    } else if (o.status === "failed") {
                        showAlert("danger", "<strong>Try Again!</strong>  Get isp param failed.");
                    }
                }, "json");
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get isp capability failed.");
            }
        }, "json");
    },
    initVideo: function () {
        var request = {
            command: "getRtspInfo",
            data: ["port"]
        };
        $.ajax({
                url: '/cgi-bin/rtsp.cgi',
                type: 'POST',
                data: JSON.stringify(request),
                cache: false,
                dataType: "json"
            })
            .done(function (o) {
                if (o.status === "succeed")
                    camera_isp.port = o.data.port;
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Init video failed.");
            })
            .always(function () {
                var options = [":network-caching=" + localStorage.caching];
                if (localStorage.protocol === "tcp")
                    options.push(":rtsp-tcp");
                addVideoPlugin("#video", "rtsp://" + document.location.host + ":" + camera_isp.port + "/" + localStorage.stream, localStorage.plugin, options);
            });
    },
    bindEvent: function () {
        $("select,input[type!=checkbox]").on("change", function () {
            camera_isp.setParam(this);
        });
        $("#btn_reset").on("click", function () {
            $("#btn_reset").prop("disabled", true);
            var request = {
                "command": "reset"
            };
            $.post("/cgi-bin/isp.cgi", JSON.stringify(request), function (o) {
                $("#btn_reset").prop("disabled", false);
                if (o.status === "succeed") {
                    showAlert("success", "<strong>Success!</strong>  Isp params have been reseted.");
                    camera_isp.initParams();
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Reset isp params failed.");
                }
            }, "json");
        });
    }
};

$(function () {
    camera_isp.initParams();
    camera_isp.initVideo();
    camera_isp.bindEvent();
});
