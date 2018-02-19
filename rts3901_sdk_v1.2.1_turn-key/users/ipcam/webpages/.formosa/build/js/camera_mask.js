var camera_mask = {
    maxMskRect: 3,
    imgWidth: 1280,
    imgHeight: 720,
    initParams: function() {
        $.ajaxSetup("cache", false);
        $("input[type=checkbox]").bootstrapSwitch("size", "mini");
        camera_mask.refresh();
    },
    refresh: function() {
        var path = "/cgi-bin/snapshot.cgi?" + Math.random();
        $("#video").css({"background": "url(" + path + ") no-repeat 0% 0% / 100% 100%", "margin": "auto"});
        var img = new Image;
        img.onload = function() {
            camera_mask.imgWidth = img.width;
            camera_mask.imgHeight = img.height;
            var widthFactor = $("#video").parent().width() / camera_mask.imgWidth * 100;
            var heightFactor = $("#video").parent().height() / camera_mask.imgHeight * 100;
            if (widthFactor < heightFactor) {
                $("#video").css({"width": "100%", "height": widthFactor / heightFactor * 100 + "%", "top": (1 - widthFactor / heightFactor) * 50 + "%"});
            } else {
                $("#video").css({"width": heightFactor / widthFactor * 100 + "%", "height": "100%"});
            }
            var factor1 = camera_mask.imgWidth / $("#video").width();
            var factor2 = camera_mask.imgHeight / $("#video").height();
            var request = {
                "command": "getMskRect"
            };
            $.post("/cgi-bin/msk.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    $.each(o.data, function(i, mskRect) {
                        $("#enable_" + (i + 1)).bootstrapSwitch("state", mskRect.enable);
                        $("#start_" + (i + 1)).html(mskRect.startX + "," + mskRect.startY);
                        $("#end_" + (i + 1)).html(mskRect.endX + "," + mskRect.endY);
                    });
                    rect.clean();
                    rect.init("#video", camera_mask.maxMskRect, o.data, factor1, factor2, function(number, startX, startY, endX, endY) {
                        $("#start_" + number).html(Math.floor(startX * factor1) + "," + Math.floor(startY * factor2));
                        $("#end_" + number).html(Math.floor(endX * factor1) + "," + Math.floor(endY * factor2));
                    });
                } else if (o.status === "failed") {
                    showAlert("danger", "<strong>Try Again!</strong>  Get mask config failed.");
                }
            }, "json");
        };
        img.src = path;
    },
    bindEvent: function() {
        $("#refresh").on("click", function() {
            camera_mask.refresh();
        });
        $("#clean").on("click", function() {
            rect.clean();
            for (var i = 0; i < camera_mask.maxMskRect; i++) {
                $("#enable_" + (i + 1)).bootstrapSwitch("state", false);
                $("#start_" + (i + 1)).html("0,0");
                $("#end_" + (i + 1)).html("0,0");
            }
        });
        $("#save").on("click", function() {
            var request = {
                "command": "setMskRect",
                "data": {
                    "resolution": {
                        "width": camera_mask.imgWidth,
                        "height": camera_mask.imgHeight
                    },
                    "rects": []
		}
            };
            for (var i = 0; i < camera_mask.maxMskRect; i++) {
                request.data.rects[i] = {};
                request.data.rects[i].enable = $("#enable_" + (i + 1)).bootstrapSwitch("state");
                request.data.rects[i].startX = +$("#start_" + (i + 1)).html().split(",")[0];
                request.data.rects[i].startY = +$("#start_" + (i + 1)).html().split(",")[1];
                request.data.rects[i].endX = +$("#end_" + (i + 1)).html().split(",")[0];
                request.data.rects[i].endY = +$("#end_" + (i + 1)).html().split(",")[1];
            }
            $("#save").prop("disabled", true);
            $.post("/cgi-bin/msk.cgi", JSON.stringify(request), function(o) {
                $("#save").prop("disabled", false);
                if (o.status === "succeed")
                    showAlert("success", "<strong>Success!</strong>  Mask config saved.");
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Mask config save failed.");
            }, "json");
        });
    }
};

$(function() {
    camera_mask.initParams();
    camera_mask.bindEvent();
});
