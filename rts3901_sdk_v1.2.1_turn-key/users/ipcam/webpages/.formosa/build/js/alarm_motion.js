var alarm_motion = {
    imgWidth: 1280,
    imgHeight: 720,
    setMdRect: {
        "command": "setMdRect",
        "data": {
            "resolution": {
		"width": 1280,
                "height": 720
            },
	    "rects": [{
                "enable": true,
                "sensitivity": 0,
                "percentage": 0,
                "startX": 0,
                "startY": 0,
                "endX": 0,
                "endY": 0
            }]
        }
    },
    refresh: function() {
        var path = "/cgi-bin/snapshot.cgi?" + Math.random();
        $("#video").css({"background": "url(" + path + ") no-repeat 0% 0% / 100% 100%", "margin": "auto"});
        var img = new Image;
        img.onload = function() {
            alarm_motion.setMdRect.data.resolution.width = alarm_motion.imgWidth = img.width;
            alarm_motion.setMdRect.data.resolution.height = alarm_motion.imgHeight = img.height;

            var widthFactor = $("#video").parent().width() / alarm_motion.imgWidth * 100;
            var heightFactor = $("#video").parent().height() / alarm_motion.imgHeight * 100;
            if (widthFactor < heightFactor) {
                $("#video").css({"width": "100%", "height": widthFactor / heightFactor * 100 + "%", "top": (1 - widthFactor / heightFactor) * 50 + "%"});
            } else {
                $("#video").css({"width": heightFactor / widthFactor * 100 + "%", "height": "100%"});
            }
            var factor1 = alarm_motion.imgWidth / $("#video").width();
            var factor2 = alarm_motion.imgHeight / $("#video").height();
            var request = {
                "command": "getMdRect"
            };
            $.post("/cgi-bin/md.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    $("#md_enable").bootstrapSwitch("state", o.data[0].enable);
                    $("#md_sensitivity").val(o.data[0].sensitivity);
                    $("#md_percentage").val(o.data[0].percentage);
                    alarm_motion.setMdRect.data.rects[0] = o.data[0];
                    rect.clean();
                    rect.init("#video", 1, o.data, factor1, factor2, function(number, startX, startY, endX, endY) {
                        alarm_motion.setMdRect.data.rects[0].startX = Math.floor(startX * factor1);
                        alarm_motion.setMdRect.data.rects[0].startY = Math.floor(startY * factor2);
                        alarm_motion.setMdRect.data.rects[0].endX = Math.floor(endX * factor1);
                        alarm_motion.setMdRect.data.rects[0].endY = Math.floor(endY * factor2);
                    });
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Get motion config failed.");
                }
            }, "json");
        };
        img.src = path;
    },
    initParams: function() {
        $.ajaxSetup("cache", false);
        $("input[type=checkbox]").bootstrapSwitch("size", "mini");
        for (var i = 0; i <= 100; i += 5)
            $("#md_sensitivity,#md_percentage").append(new Option(i));
        alarm_motion.refresh();
    },
    bindEvent: function() {
        $("#btn_refresh").on("click", function() {
            alarm_motion.refresh();
        });
        $("#btn_clean").on("click", function() {
            rect.clean();
            alarm_motion.setMdRect.data.rects[0].startX = 0;
            alarm_motion.setMdRect.data.rects[0].startY = 0;
            alarm_motion.setMdRect.data.rects[0].endX = 0;
            alarm_motion.setMdRect.data.rects[0].endY = 0;
        });
        $("#btn_save").on("click", function() {
            alarm_motion.setMdRect.data.rects[0].enable = $("#md_enable").bootstrapSwitch("state");
            alarm_motion.setMdRect.data.rects[0].sensitivity = Number($("#md_sensitivity").val());
            alarm_motion.setMdRect.data.rects[0].percentage = Number($("#md_percentage").val());
            $.post("/cgi-bin/md.cgi", JSON.stringify(alarm_motion.setMdRect), function(o) {
                if (o.status === "succeed") {
                    showAlert("success", "<strong>Success!</strong>  Motion config saved.");
                    if (alarm_motion.setMdRect.data[0].enable) {
                        utils.initMdFastTimer();
                    } else {
                        utils.initMdSlowTimer();
                    }
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Motion config save failed.");
                }
            }, "json");
        });
    }
};

$(function() {
    alarm_motion.initParams();
    alarm_motion.bindEvent();
});
