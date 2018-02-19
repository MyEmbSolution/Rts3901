$(function() {
    $("#rebootButton").on("click", function() {
        $("#waitModal").modal("show");
        utils.reboot(30 * 1000);
    });
});