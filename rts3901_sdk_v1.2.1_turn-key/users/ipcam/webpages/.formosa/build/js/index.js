var index = {
    current: "liveview",
    switchState: function(id, state) {
        $("#" + id).removeClass("top_menu_bg_on top_menu_bg_off").addClass("top_menu_bg_" + state);
        $("#top_menu_icon_" + id).attr("class", "top_menu_icon_" + id + "_" + state);
        $("#top_menu_text_" + id).attr("class", "top_menu_text_" + state);
    },
    initParams: function() {
        var request = {
            "command": "getMdRect"
        };
        $.post("/cgi-bin/md.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                utils.initMdSlowTimer();
                if (o.data[0].enable) {
                    utils.initMdFastTimer();
                }
            }
        }, "json");
    },
    bindEvent: function() {
        $(".top_menu").on({
            click: function() {
                index.switchState(index.current, "off");
                index.current = this.id;
                index.switchState(index.current, "on");
                if (this.id !== "admin")
                    $("#container-body").load(this.id + ".html");
                else
                    $("#container-body").load("/admin/" + this.id + ".html");
            },
            mouseenter: function() {
                if (this.id !== index.current)
                    index.switchState(this.id, "on");
            },
            mouseleave: function() {
                if (this.id !== index.current)
                    index.switchState(this.id, "off");
            }
        });
    }
};

$(function() {
    $("#container-body").load("liveview.html");
    index.bindEvent();
    index.initParams();
});