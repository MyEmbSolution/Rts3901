var camera_audio = {
    sliceSize: (64 * 1024),
    uploadSlice: function(sliceIndex) {
        var demofile = $("#demofile")[0].files[0];
        var slice = demofile.slice(sliceIndex * camera_audio.sliceSize, (sliceIndex + 1) * camera_audio.sliceSize);
        $.ajax({
                url: '/cgi-bin/uploadDemo.cgi',
                type: 'POST',
                data: slice,
                cache: false,
                contentType: false,
                processData: false,
                dataType: "json"
            })
            .done(function(o) {
                if (o.status === "succeed") {
            var percentage = (sliceIndex + 1) * camera_audio.sliceSize * 100 / demofile.size;
                    percentage = percentage > 100 ? 100 : percentage;
                    $("#progress").css("width", Math.round(percentage) + "%");
                    $("#modalLabel").html("Uploading " + Math.round(percentage) + "%");
                    if (percentage === 100) {
                        $("#modalLabel").html("Upload Demo File Success!");
                        $("#progress").addClass("progress-bar-success");
                        setTimeout(function() {
                            $("#waitModal").modal("hide");
                        }, 3000);
                    } else {
                        camera_audio.uploadSlice(++sliceIndex);
                    }
                } else {
                    $("#modalLabel").html("Upload failed! Please try again!");
                    $("#progress").addClass("progress-bar-danger");
                    setTimeout(function() {
                        $("#waitModal").modal("hide");
                    }, 3000);
                }
            })
            .fail(function(o) {
                $("#modalLabel").html("Upload failed! Please try again!");
                $("#progress").addClass("progress-bar-danger");
                setTimeout(function() {
                    $("#waitModal").modal("hide");
                }, 3000);
            });
    },
    initBasic: function() {
        var request = {
            command: "getCapability",
            data: []
        };
        $("#denoise,#aec,#aecDemo").bootstrapSwitch("size", "mini");
        $.post("/cgi-bin/audio.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#captureVolume").attr({
                    "min": o.data.captureVolume.data.min,
                    "max": o.data.captureVolume.data.max,
                    "step": o.data.captureVolume.data.step
                });
                $("#playbackVolume").attr({
                    "min": o.data.playbackVolume.data.min,
                    "max": o.data.playbackVolume.data.max,
                    "step": o.data.playbackVolume.data.step
                });
                $.each(o.data.codec.data, function(i, codec) {
                        $("#codec").append(new Option(codec));
                });
                request.command = "getParam";
                $.post("/cgi-bin/audio.cgi", JSON.stringify(request), function(o) {
                    if (o.status === "succeed") {
                        $("#captureVolume").val(o.data.captureVolume).next("span").text(o.data.captureVolume);
                        $("#playbackVolume").val(o.data.playbackVolume).next("span").text(o.data.playbackVolume);
                        $("#codec").val(o.data.codec);
                        $("#denoise").bootstrapSwitch("state", o.data.denoise);
                        $("#aec").bootstrapSwitch("state", o.data.aec);
                        if (o.data.aec)
                            $("#denoise").bootstrapSwitch("disabled", true);
                        $("#aecDemo").bootstrapSwitch("state", o.data.aecDemo);
                        $("#denoise,#aec,#aecDemo").on("switchChange.bootstrapSwitch", function() {
                            var request = {command: "setParam", data: {}};
                            request.data[this.id] = $(this).bootstrapSwitch("state");
                            if (this.id === "aec") {
                                if ($(this).bootstrapSwitch("state")) {
                                    $("#denoise").bootstrapSwitch("state", true);
                                    $("#denoise").bootstrapSwitch("disabled", true);
                                } else {
                                    $("#denoise").bootstrapSwitch("disabled", false);
                                }
                            }
                            $.post("/cgi-bin/audio.cgi", JSON.stringify(request), function(o) {
                                if (o.status === "failed")
                                    showAlert("danger", "<strong>Try Again!</strong>  Set audio param failed.");
                            }, "json");
                        });
                    } else if (o.status === "failed") {
                        showAlert("danger", "<strong>Try Again!</strong>  Get audio param failed.");
                    }
                }, "json");
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get audio capability failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $("#playbackVolume,#captureVolume").on("change", function() {
            var request = {
                command: "setParam",
                data: {}
            };
            request.data[this.id] = +$(this).val();
            $(this).next("span").text($(this).val());
            $("#playbackVolume,#captureVolume,#codec").prop("disabled", true);
            $.post("/cgi-bin/audio.cgi", JSON.stringify(request), function(o) {
                $("#playbackVolume,#captureVolume,#codec").prop("disabled", false);
                if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Set volume failed.");
            }, "json");
        });
        $("#codec").on("change", function() {
            var request = {
                command: "setParam",
                data: {
                    codec: $(this).val()
                }
            };
            $("#playbackVolume,#captureVolume,#codec").prop("disabled", true);
            $.post("/cgi-bin/audio.cgi", JSON.stringify(request), function(o) {
                $("#playbackVolume,#captureVolume,#codec").prop("disabled", false);
                if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Set codec failed.");
            }, "json");
        });
    $("#upload").on("click", function() {
        $("#demofile").click();
    });
    $("#demofile").on("change", function() {
        $("#progress").removeClass("progress-bar-danger progress-bar-success").css("width", "0%");
        $("#modalLabel").html("Uploading 0%");
        $("#waitModal").modal("show");
        $.post("/cgi-bin/uploadDemo.cgi", "newfile", function(o) {
            if (o.status == "succeed")
                camera_audio.uploadSlice(0);
        }, "json");
    });
    }
};

$(function() {
    camera_audio.initBasic();
    camera_audio.bindEvent();
});
