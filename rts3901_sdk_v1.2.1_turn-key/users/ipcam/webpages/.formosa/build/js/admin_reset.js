$(function () {
    $("#btn_reset").on("click", function () {
        var request = {
            command: "reset",
            data: []
        };
        if ($("#reserveTime").prop("checked"))
            request.data.push("time");
        if ($("#reserveNetwork").prop("checked"))
            request.data.push("network");
        $.post("/cgi-bin/reset.cgi", JSON.stringify(request), function (o) {
            if (o.status === "succeed") {
                $("#waitModal").modal("show");
                utils.reboot(30 * 1000);
            } else if (o.status === "failed") {
                showAlert("danger", "<strong>Try Again!</strong>  Reset device failed.");
            }
        }, "json");
    });
});
