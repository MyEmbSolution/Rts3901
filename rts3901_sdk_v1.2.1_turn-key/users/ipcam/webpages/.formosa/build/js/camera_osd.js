var camera_osd = {
    getOsd: function(index) {
        var table = ''
            + '<table class="table table-striped column2">'
            + '  <thead><tr><th colspan="2">Stream ' + index + '</th></tr></thead>'
            + '  <tbody>'
            + '    <tr><th>Enable</th><td><input id="osdEnable_' + index + '" type="checkbox"></td></tr>'
            + '    <tr><th>Screen Text</th><td><input id="screenText_' + index + '"></td></tr>'
            + '  </tbody>'
            + '</table>';
        $("#osd_table_container").append(table);
        $("#osdEnable_" + index).bootstrapSwitch("size", "mini");
        var request = {
            "command": "getParam",
            "stream": "profile" + index,
            "data": ["osdEnable", "screenText"]
        };
        $.post("/cgi-bin/osd.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $("#osdEnable_" + index)
                    .bootstrapSwitch("state", o.data.osdEnable)
                    .on("switchChange.bootstrapSwitch", function() {
                        camera_osd.setParam(this);
                    });
                $("#screenText_" + index).val(decodeURIComponent(o.data.screenText));
                $("#screenText_" + index).prop("disabled", !o.data.osdEnable);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get osd param failed.");
            }
        }, "json");
    },
    setParam: function(element) {
        var request = {
            "command": "setParam",
            "stream": "profile" + element.id.split("_")[1],
            "data": {}
        };
        if (element.id.split("_")[0] === "osdEnable") {
            request.data.osdEnable = $(element).bootstrapSwitch("state");
            $("#screenText_" + element.id.split("_")[1]).prop("disabled", !request.data.osdEnable);
        } else {
            request.data[element.id.split("_")[0]] = encodeURIComponent($(element).val());
        }
        $.post("/cgi-bin/osd.cgi", JSON.stringify(request), function(o) {
            if (o.status === "failed")
                showAlert("danger", "<strong>Try Again!</strong>  Set osd param failed.");
        }, "json");
    },
    initParams: function() {
        $.ajaxSetup("cache", false);
        var request = {
            "command": "getStreamList"
        };
        $.post("/cgi-bin/stream.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                var num = o.data.length;
                $("#osd_table_container").empty();
                for (var i = 1; i <= num; i++)
                    camera_osd.getOsd(i);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get stream list failed.");
            }
        }, "json");
    },
    bindEvent: function() {
        $("#osd_table_container").on("change", "input[type!=checkbox]",function() {
            camera_osd.setParam(this);
        });
    }
}

$(function() {
    camera_osd.initParams();
    camera_osd.bindEvent();
});
