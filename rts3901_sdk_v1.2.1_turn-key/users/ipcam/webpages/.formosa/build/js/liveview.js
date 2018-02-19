var liveview = {
    port: 43794,
    getStreamList: function() {
        var request = {
            "command": "getStreamList"
        };
        $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $.each(o.data, function(i, stream) {
                    $("#stream").append(new Option(stream.replace("profile", "Stream "), stream));
                });
                $("#stream").val(localStorage.stream);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get stream list failed.");
            }
        }, "json");
    },
    initParams: function() {
        if (!window.localStorage.protocol)
            localStorage.protocol = "udp";
        if (!window.localStorage.caching)
            localStorage.caching = "1000";
        if (!window.localStorage.stream)
            localStorage.stream = "profile1";
        if (!window.localStorage.plugin || !utils.isIE())
            localStorage.plugin = "vlc";

        $("#plugin").val(localStorage.plugin);
        $("#protocol").val(localStorage.protocol);
        $("#caching").val(localStorage.caching);
        utils.isIE() ? $("#plugin option:gt(0)").show() : $("#plugin option:gt(0)").hide();
        liveview.getStreamList();
    },
    initVideo: function() {
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
            .done(function(o) {
                if (o.status === "succeed")
                    liveview.port = o.data.port;
                else if (o.status === "failed")
                    showAlert("danger", "<strong>Try Again!</strong>  Init video failed.");
            })
            .always(function() {
                var options = [":network-caching=" + localStorage.caching];
                var url = "rtsp://" + document.location.host + ":" + liveview.port + "/" + localStorage.stream;
                if (localStorage.protocol === "tcp") {
                    options.push(":rtsp-tcp");
                } else if (localStorage.protocol === "http") {
                    options.push(":rtsp-http");
                    url = "rtsp://" + document.location.host + "/" + localStorage.stream;
                }
                addVideoPlugin("#video", url, localStorage.plugin, options);
            });
    },
    bindEvent: function() {
        $("select").change(function() {
            localStorage[this.id] = $(this).val();
            liveview.initVideo();
        });
    }
}

$(function() {
    liveview.initParams();
    liveview.initVideo();
    liveview.bindEvent();
});
