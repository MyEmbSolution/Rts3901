var admin_fwupgrade = {
    firmware: {},
    sliceSize: (64 * 1024),
    percentage: 0,
    rebootTime: 30000,
    resetModal: function() {
        $("#checkProgress").removeClass("progress-bar-danger")
            .addClass("progress-bar-warning")
            .css("width", "0%");
        $("#burnProgress").removeClass("progress-bar-danger")
            .css("width", "0%");
    },
    showProgress: function(status) {
        if (status === "checking") {
            $("#checkProgress").css("width", Math.round(admin_fwupgrade.percentage) + "%");
            $("#modalLabel").html("Checking " + Math.round(admin_fwupgrade.percentage) + "%");
        } else if (status === "burning") {
            $("#checkProgress").css("width", Math.round(100 - admin_fwupgrade.percentage) + "%");
            $("#burnProgress").css("width", Math.round(admin_fwupgrade.percentage) + "%");
            $("#modalLabel").html("Burning " + Math.round(admin_fwupgrade.percentage) + "%");
            if (admin_fwupgrade.percentage === 100) {
                $("#modalLabel").html("Success! Waiting for device reboot!");
                $("#burnProgress").addClass("progress-bar-success");
                setTimeout(function() {
                    $.get("/?" + Math.random()).done(function() {
                        location.replace(location.href);
                    });
                }, admin_fwupgrade.rebootTime);
                setTimeout(function() {
                    $("#waitModal").modal("hide");
                    location.replace(location.href);
                }, 2 * admin_fwupgrade.rebootTime);
            }
        }
    },
    showError: function(status, reason) {
        if (status === "checking")
            $("#checkProgress").removeClass("progress-bar-warning").addClass("progress-bar-danger");
        else if (status === "burning")
            $("#burnProgress").addClass("progress-bar-danger");

        $("#modalLabel").html(reason);
        setTimeout(function() {
            $("#waitModal").modal("hide");
        }, 10000);
    },
    uploadSlice: function(sliceIndex, status) {
        admin_fwupgrade.firmware = $("#firmware")[0].files[0];
        var slice = admin_fwupgrade.firmware.slice(sliceIndex * admin_fwupgrade.sliceSize, (sliceIndex + 1) * admin_fwupgrade.sliceSize);
        $.ajax({
                url: '/cgi-bin/firmware.cgi',
                type: 'POST',
                data: slice,
                cache: false,
                contentType: false,
                processData: false,
                dataType: "json"
            })
            .done(function(o) {
                if (o.status === "succeed") {
                    admin_fwupgrade.percentage = (sliceIndex + 1) * admin_fwupgrade.sliceSize * 100 / admin_fwupgrade.firmware.size;
                    admin_fwupgrade.percentage = admin_fwupgrade.percentage > 100 ? 100 : admin_fwupgrade.percentage;
                    admin_fwupgrade.showProgress(status);
                    if (admin_fwupgrade.percentage === 100) {
                        if (status === "checking")
                            admin_fwupgrade.uploadSlice(0, "burning");
                    } else {
                        admin_fwupgrade.uploadSlice(++sliceIndex, status);
                    }
                } else {
                    admin_fwupgrade.percentage = 0;
                    admin_fwupgrade.showError(status, "Upgrade failed! Please try again!");
                }
            })
            .fail(function(o) {
                admin_fwupgrade.percentage = 0;
                admin_fwupgrade.showError(status, "Upgrade failed! Please try again!");
            });
    },
    initParams: function() {
        $("#upgrade").prop("disabled", !$("#firmware")[0].files.length);
    },
    bindEvent: function() {
        $('#btn_browse').on('click', function() {
            $('#firmware').click();
        });
        $("#firmware").on("change", function() {
            $("#firmware_name").val($('#firmware')[0].files[0].name);
            $("#upgrade").prop("disabled", !$("#firmware")[0].files.length);
        });
        $("#upgrade").on("click", function() {
            admin_fwupgrade.resetModal();
            $("#waitModal").modal("show");
            admin_fwupgrade.uploadSlice(0, "checking");
        });
    }
};

$(function() {
    admin_fwupgrade.initParams();
    admin_fwupgrade.bindEvent();
});
