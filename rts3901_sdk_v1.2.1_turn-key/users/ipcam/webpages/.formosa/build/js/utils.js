var rect = {
    current: {},
    canvas: null,
    callback: null,
    number: 0,
    maxNumber: 0,
    init: function(canvas, maxNumber, data, factor1, factor2, callback) {
        rect.maxNumber = maxNumber;
        rect.canvas = canvas;
        rect.callback = callback;
        $(rect.canvas).css("position", "relative");
        $(rect.canvas).off("mousedown").on("mousedown", function(e) {
            rect.beginDrawing(e);
        });
        maxNumber = data.length < maxNumber ? data.length : maxNumber;
        for (var i = 0; i < maxNumber; i++) {
            if (!data[i].startX
                && !data[i].endX
                && !data[i].startY
                && !data[i].endY)
                continue;
            $("<div>").attr("id", "rect" + (i + 1)).css({
                "position": "absolute",
                "width": (data[i].endX - data[i].startX) / factor1,
                "height": (data[i].endY - data[i].startY) / factor2,
                "left": data[i].startX / factor1,
                "top": data[i].startY / factor2,
                "border": "dotted red 1px",
            }).html($("<label>").text(i + 1)).appendTo($(rect.canvas));
            rect.number++;
        }
    },
    getX: function(e) {
        return e.x ? e.x : e.pageX;
    },
    getY: function(e) {
        return e.y ? e.y : e.pageY;
    },
    beginDrawing: function(e) {
        rect.number++;
        if (rect.number > rect.maxNumber) {
            rect.number = rect.maxNumber;
            return;
        }
        rect.current.obj = $("<div>").attr("id", "rect" + rect.number).css({
            "position": "absolute",
            "width": 0,
            "height": 0,
            "left": rect.getX(e) - $(rect.canvas).offset().left,
            "top": rect.getY(e) - $(rect.canvas).offset().top,
            "border": "dotted red 1px",
        });
        rect.current.initLeft = rect.getX(e);
        rect.current.initTop = rect.getY(e);
        $("<label>").text(rect.number).appendTo(rect.current.obj);
        rect.current.obj.appendTo($(rect.canvas));
        $(rect.canvas).on("mousemove", function(e) {
            rect.drawing(e);
        });
        $(rect.canvas).on("mouseup mouseleave", function() {
            rect.endDrawing();
        });
    },
    drawing: function(e) {
        var dx = rect.getX(e) - rect.current.initLeft;
        var dy = rect.getY(e) - rect.current.initTop;
        if (dx < 0)
            rect.current.obj.css("left", rect.getX(e) - $(rect.canvas).offset().left);
        if (dy < 0)
            rect.current.obj.css("top", rect.getY(e) - $(rect.canvas).offset().top);
        rect.current.obj.css({
            "width": Math.abs(dx),
            "height": Math.abs(dy)
        });
    },
    endDrawing: function() {
        var startX = rect.current.obj.offset().left - $(rect.canvas).offset().left;
        var startY = rect.current.obj.offset().top - $(rect.canvas).offset().top;
        var endX = startX + rect.current.obj.width();
        var endY = startY + rect.current.obj.height();
        rect.callback(rect.number, startX, startY, endX, endY);
        $(rect.canvas).off("mousemove mouseup mouseleave");
    },
    clean: function() {
        for (var i = 0; i < rect.maxNumber; i++) {
            $("#rect" + (i + 1)).remove();
        }
        rect.number = 0;
    }
};

var regex = {
    ip: /^(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])(\.(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])){3}$/,
    netmask: /^(254|252|248|240|224|192|128|0)\.0\.0\.0$|^255\.(254|252|248|240|224|192|128|0)\.0\.0$|^255\.255\.(254|252|248|240|224|192|128|0)\.0$|^255\.255\.255\.(254|252|248|240|224|192|128|0)$/,
    wepPasswd: /^(.{5}|.{10}|.{13}|.{26})$/,
    wpaPasswd: /^.{8,63}$/,
    user: /^.{1,20}$/,
    passwd: /^.{6,20}$/,
    date: /^20\d{2}-(0*[1-9]|1[0-2])-(0*[1-9]|[1-2][0-9]|3[0-1])$/,
    time: /^(0*[0-9]|1[0-9]|2[0-3]):(0*[0-9]|[1-5][0-9]):(0*[0-9]|[1-5][0-9])$/,
    wpsPin: /^[0-9]{8}$/,
    hostname: /^.{1,63}$/,
    prefixLength: /^(\d{1,2}|1[0-1]\d|12[0-8])$/
};

function addVideoPlugin(target, source, plugin, options) {
    var plugin = plugin ? plugin : "vlc";
    var content;

    $(target).css("text-align", "center");
    if (plugin === "vlc") {
        content = $("<object>").attr({
                "id": "vlc-ie",
                "width": $(target).width(),
                "height": $(target).height(),
                "classid": "clsid:9BE31822-FDAD-461B-AD51-BE1D1C159921",
                "codebase": "http://download.videolan.org/pub/videolan/vlc/last/win32/axvlc.cab"
            }).append($("<param>").attr({
                    "name": "src",
                    "value": source
                }))
            .append($("<param>").attr({
                    "name": "toolbar",
                    "value": "false"
                }))
            .append('<h3 id="noplugin-info">Please install / active vlc player first.</h3>')
            .append($("<embed>").attr({
                    "id": "vlc",
                    "width": "100%",
                    "height": "100%",
                    "type": "application/x-vlc-plugin",
                    "pluginspage": "http://www.videolan.org",
                    "toolbar": "false"
            }));
    } else if (plugin === "quicktime") {
        content = $("<object>").attr({
            "id": "quicktime",
            "classid": "clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B",
            "codebase": "http://www.apple.com/qtactivex/qtplugin.cab"
        }).append($("<param>").attr({
            "name": "qtsrc",
            "value": source
        })).append($("<param>").attr({
            "name": "autoplay",
            "value": true
        })).append($("<param>").attr({
            "name": "scale",
            "value": "aspect"
        })).append($("<param>").attr({
            "name": "controller",
            "value": false
        })).append("<h3>Please install / active quicktime player first.</h3>")
        .css({"width": "100%", "height": "100%"});
    } else if (plugin === "webview") {
        content = $("<object>").attr({
            "id": "webview",
            "width": "100%",
            "height": "100%",
            "classid": "clsid:5C661CD6-6518-423E-A8DC-4CF366C66052",
        }).append("<h3>Please install / active webview player first.</h3>")
        .css({"width": "100%", "height": "100%"});
    }
    $(target).empty().html(content);
    if (plugin === "vlc") {
        $("#caching").prop("disabled", false);
        $("#protocol").prop("disabled", false);
        if (!utils.isIE()) {
            var vlc = document.getElementById("vlc");
            if (!vlc.playlist)
                return;
            $("#noplugin-info").hide();
            vlc.playlist.playItem(vlc.playlist.add(source, "video", options));
        } else {
            setTimeout(function() {
                $("#vlc-ie").css({"width": "100%", "height": "100%"});
            }, 1000);
        }
    } else if (plugin === "quicktime") {
        $("#caching").prop("disabled", true);
        $("#protocol").prop("disabled", true);
    } else if (plugin === "webview") {
        $("#caching").prop("disabled", true);
        $("#protocol").prop("disabled", false);
        if (options.indexOf(":rtsp-tcp") > -1) {
            $("#webview")[0].SetRtspOverType(1);
        } else if (options.indexOf(":rtsp-http") > -1) {
            $("#webview")[0].SetRtspOverType(2);
        } else {
            $("#webview")[0].SetRtspOverType(0);
        }
        $("#webview")[0].setUrl(source, $(target).width(), $(target).height());
    }
}

function showAlert(status, alertContent) {
    $("#responseAlert").removeClass("alert-success alert-info alert-warning alert-danger").addClass("alert-" + status)
        .html(alertContent).slideDown(1000).delay(5000).slideUp(1000);
}

var submenu = {
    switchState: function(id, state) {
        $("#submenu_icon_" + id)
            .removeClass("submenu_icon_" + id + "_on submenu_icon_" + id + "_off")
            .addClass("submenu_icon_" + id + "_" + state);
        $("#submenu_text_" + id)
            .removeClass("submenu_text_on submenu_text_off")
            .addClass("submenu_text_" + state);
        $("#submenu_arrow_" + id)
            .removeClass("submenu_arrow_on submenu_arrow_off")
            .addClass("submenu_arrow_" + state);
    },
    init: function(_prefix, _current) {
        var prefix = _prefix;
        var current = _current;
        var submenu_entry = $(".submenu_entry");
        var length = submenu_entry.length;

        if (prefix !== "admin")
            $("#main_content").load(prefix + "_" + current + ".html");
        else
            $("#main_content").load("/admin/" + prefix + "_" + current + ".html");

        $(".submenu > div").on({
            click: function() {
                submenu.switchState(current, "off");
                current = this.id;
                submenu.switchState(current, "on");
                if (prefix !== "admin")
                    $("#main_content").load(prefix + "_" + this.id + ".html");
                else
                    $("#main_content").load("/admin/" + prefix + "_" + this.id + ".html");
            },
            mouseenter: function() {
                if (this.id !== current)
                    submenu.switchState(this.id, "on");
            },
            mouseleave: function() {
                if (this.id !== current)
                    submenu.switchState(this.id, "off");
            }
        });
    }
};

var utils = {
    isIE: function() {
        return ("ActiveXObject" in window) ? true : false;
    },
    getStatus: function(status) {
        $.ajaxSetup("cache", false);
        var request = {
            command: "getStatus",
            data: [status]
        };
        $.post("/cgi-bin/status.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                $.each(o.data[status], function(id, text) {
                    $("#" + id).text(id !== "dhcpc" ? text : (text ? "dhcp" : "static"));
                });
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Get " + status + " status failed.");
            }
        }, "json");
    },
    reboot: function(time, url) {
        var percentage = 0;
        var request = {
            "command": "reboot"
        };
        $.post("/cgi-bin/reboot.cgi", JSON.stringify(request), function(o) {
            if (o.status === "succeed") {
                var timer = setInterval(function() {
                    ++percentage;
                    $("#rebootProgress").css("width", percentage + "%");
                    $("#modalLabel").html("Rebooting " + percentage + "%");
                    if (percentage === 100) {
                        clearInterval(timer);
                        $.get("/?" + Math.random()).done(function() {
                            location.replace(url ? url : location.href);
                        });
                        setTimeout(function() {
                            $("#waitModal").modal("hide");
                            location.replace(url ? url : location.href);
                        }, 60);
                    }
                }, time / 100);
            } else if (o.status === "failed") {
                $("#modalLabel").html("Reboot failed!");
            }
        }, "json");
    },
    initMdSlowTimer: function() {
        if (window.mdSlowTimer)
            clearInterval(mdSlowTimer);
        if (window.mdFastTimer)
            clearInterval(mdFastTimer);
        mdSlowTimer = setInterval(function() {
            var request = {
                "command": "getMdRect"
            };
            $.post("/cgi-bin/md.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    if (o.data[0].enable) {
                        utils.initMdFastTimer();
                    } else {
                        if (window.mdFastTimer)
                            clearInterval(mdFastTimer);
                    }
                }
            }, "json");
        }, 30 * 1000);
    },
    initMdFastTimer: function() {
        if (window.mdFastTimer)
            clearInterval(mdFastTimer);
        mdFastTimer = setInterval(function() {
            var request = {
                "command": "getMdStatus"
            };
            $.post("/cgi-bin/md.cgi", JSON.stringify(request), function(o) {
                if (o.status === "succeed") {
                    if (o.data[0] === 1)
                        $("#mdAlert").show(300).delay(1000).hide(300);
                }
            }, "json");
        }, 3 * 1000);
    }
};
