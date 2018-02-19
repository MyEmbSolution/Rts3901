var alarm_schedule = {
	data:[],
	schedule_rect:[],
	rules:[],
	actions:[["SAVESTREAM:SD","Save stream to sd"],
			["SAVESNAPSHOT:SD","Save snapshot to sd"],
			["ACTIVATEDIGITALOUTPUT","Activate digital output"]],

	addRecordEvent: function(i, o) {
		var thisTr = $("<tr>")
			.append($("<td>").addClass("eventName"))
			.append($("<td>").addClass("eventType"))
			.append($("<td>").addClass("eventSchedule"))
			.append($("<td>").addClass("eventAction"))
			.append($("<td>").html($("<input>").attr("type", "checkbox").addClass("enable")))
			.append($("<td>").html($("<button>").addClass("btn btn-danger btn-xs btn-delete").text("Delete")))
			.append($("<td>").html($("<button>").addClass("btn btn-danger btn-xs btn-modify").text("Modify")))
			.append($("</tr>"))
			.appendTo($("#eventList"));

		thisTr.find(".enable").bootstrapSwitch("size", "mini");
		thisTr.find(".eventName").append(o.token);
		if (o.event_name === "MD") {
	 		thisTr.find(".eventType").append("Motion Detection");
		}
		if (o.event_name === "SCHEDULE") {
	 		thisTr.find(".eventType").append("Schedule");
		}
		if (o.schedule_type === "0") {
	 		thisTr.find(".eventSchedule").append("Always");
		}
		if (o.schedule_type === "1") {
	 		thisTr.find(".eventSchedule").append("Scheduled");
		}
		var action = "";
		for (var m = 0; m < alarm_schedule.actions.length; m++) {
			if (o.actions.indexOf(alarm_schedule.actions[m][0]) >= 0) {
				if (action !== "") {
					action = action + " and " + alarm_schedule.actions[m][1];
				} else {
					action = alarm_schedule.actions[m][1];
				}
			}
   		}
		thisTr.find(".eventAction").append(action);
		if (o.enabled === "1") {
	 		thisTr.find(".enable").bootstrapSwitch("state",  true);
		} else {
			thisTr.find(".enable").bootstrapSwitch("state",  false);
		}

		thisTr.find(".btn-delete").on("click", function() {
			var request = {
              		command: "removeAlarmRules",
				data: []
       		};
			request.data.push(alarm_schedule.rules[i].token);
      			$.post("/cgi-bin/alarm.cgi", JSON.stringify(request), function(o) {
				if (o.status == "succeed") {
					showAlert("success", "<strong>Success!</strong>  delete rule ok.");
					alarm_schedule.initBasic();
				} else if (o.status === "failed") {
                			showAlert("danger", "<strong>Try Again!</strong>  Get alarm rules failed.");
				}
			}, "json");
		});
		thisTr.find(".btn-modify").on("click", function() {
			alarm_schedule.resetRule();
			alarm_schedule.data = alarm_schedule.rules[i];
			alarm_schedule.setRule();
			$('#scheduleCell').attr("readonly", true);
			$("#setModal").modal("show");
		});

		thisTr.find(".enable").on("switchChange.bootstrapSwitch", function() {
			var request = {
				command: "addAlarmRules",
				data: [],
			}
			alarm_schedule.rules[i].enabled = thisTr.find(".enable").bootstrapSwitch("state") ? "1" : "0";
			request.data.push(alarm_schedule.rules[i]);
			$.post("/cgi-bin/alarm.cgi", JSON.stringify(request), function(o) {
			if (o.status === "succeed")
				showAlert("success", "<strong>Success!</strong>  Alarm rules saved.");
			else if (o.status === "failed")
				showAlert("danger", "<strong>Try Again!</strong>  Set alarm rules failed.");
			}, "json");
		});
	},

   	initScheduleTable: function() {
		var canvas=document.getElementById('myCanvas');
		var ctx=canvas.getContext('2d');

		canvas.width = 24*40+60+20;
		canvas.height = 30*9+1;

		for ( var i = 2; i * 30 < canvas.height; i++ ) {
                	ctx.beginPath();
                	ctx.moveTo(0,i * 30);
                	ctx.lineTo(canvas.width-20,i* 30);
                	ctx.stroke();
       	}

		for ( var j = 0; j * 40 < canvas.width; j++ ) {
        		var x = 0;
       	 	var y = 0;
			if (! ((j +5)% 6)) {
        			ctx.lineWidth = 2;
        			y = 30;
        		} else {
        			ctx.lineWidth = 1;
        			y=60;
       		}
			ctx.beginPath();
        		if (j==0) {
         			ctx.moveTo(0, y);
            			ctx.lineTo(0, canvas.height);
	 		} else if (j==1) {
        		 	ctx.moveTo(j * 60, 0+y);
              	 	ctx.lineTo(j * 60, canvas.height);
       	 	} else {
               		ctx.moveTo(j * 40+20, y);
               	 	ctx.lineTo(j * 40+20, canvas.height);
         		}
         		ctx.stroke();
  		}

		ctx.font="15px Georgia";
		ctx.fillText("0:00",45,20);
		ctx.fillText("6:00",285,20);
		ctx.fillText("12:00",520,20);
		ctx.fillText("18:00",760,20);
		ctx.fillText("24:00",1000,20);
		ctx.fillText("SUN",15,80);
		ctx.fillText("MON",15,110);
		ctx.fillText("TUE",15,140);
		ctx.fillText("WED",15,170);
		ctx.fillText("THU",15,200);
		ctx.fillText("FRI",15,230);
		ctx.fillText("SAT",15,260);

		function getMousePos(canvas, evt) {
    			var rect = canvas.getBoundingClientRect();
   	 		return {
      				x: evt.clientX - rect.left * (canvas.width / rect.width),
     	 			y: evt.clientY - rect.top * (canvas.height / rect.height)
    			}
  		};

		canvas.addEventListener("click", function (evt) {
			var mousePos = getMousePos(canvas, evt);
   			ctx.fillStyle='#337ab7';
   			if (mousePos.x>60&&mousePos.x<1020&&mousePos.y>60) {
   				var xx = Math.floor((Math.floor(mousePos.x)-60)/40)*40+60;
				var yy =  Math.floor((Math.floor(mousePos.y)-60)/30)*30+60;
				var i = Math.floor((Math.floor(mousePos.y)-60)/30);
				var j = Math.floor((Math.floor(mousePos.x)-60)/40);

   				if (alarm_schedule.schedule_rect[i][j]==0) {
					ctx.fillRect(xx+1,yy+1,38,28);
					alarm_schedule.schedule_rect[i][j]=1;
   				} else {
					ctx.clearRect(xx+1,yy+1,38,28);
					alarm_schedule.schedule_rect[i][j]=0;
				}
   			}
  		}, false);
    	},

	scheduleConvert:function(schedule_time){
		if (!schedule_time) {
			for (var i = 0; i < 7; i++) {
				for (var j =0;j<24;j++) {
					alarm_schedule.schedule_rect[i][j]= 0;
				}
			}
			return;
		}
		for (var i = 0; i < 7; i++) {
			var weekday = parseInt(schedule_time.substring(i*7,i*7+6),16);
			for (var j =0;j<24;j++) {
				alarm_schedule.schedule_rect[i][23-j]= weekday%2;
				weekday >>=1;
			}
		}
    	},

    	scheduleConvertToString:function(){
    		var schedule_time = "";
		for (var i = 0; i < 7; i++) {
			var weekday = 0;
			for (var j =0;j<24;j++) {
				if (alarm_schedule.schedule_rect[i][j]==1) {
					weekday++;
				}
				if (j<23) {
					weekday<<=1;
				}
			}
			var weekdays = "000000";
			schedule_time=schedule_time+weekdays.substr(0,6-(weekday.toString(16)).length) + weekday.toString(16);;
			if (i<6) {
				schedule_time+=",";
			}
		}
		return schedule_time;
	},

	drawSchedule:function(){
		var canvas=document.getElementById('myCanvas');
		var ctx=canvas.getContext('2d');
		ctx.fillStyle='#337ab7';
		for (var i = 0; i<7;i++) {
			for (var j = 0; j<24; j++) {
				if (alarm_schedule.schedule_rect[i][j]==1) {
					ctx.fillRect(j*40+60+1,i*30+60+1,38,28);
   				} else {
					ctx.clearRect(j*40+60+1,i*30+60+1,38,28);
				}
			}
		}
	},

	selectAll:function() {
		var canvas=document.getElementById('myCanvas');
		var ctx=canvas.getContext('2d');
		ctx.fillStyle='#337ab7';
		for (var i=0;i<7;i++) {
			for (var j=0;j<24;j++) {
				alarm_schedule.schedule_rect[i][j]=1;
				ctx.fillRect(j*40+60+1,i*30+60+1,38,28);
			}
		}
    	},

	clearAll:function() {
		var canvas=document.getElementById('myCanvas');
		var ctx=canvas.getContext('2d');
		for (var i=0;i<7;i++) {
			for (var j=0;j<24;j++) {
				alarm_schedule.schedule_rect[i][j]=0;
				ctx.clearRect(j*40+60+1,i*30+60+1,38,28);
			}
		}
    	},

    	addRule:function() {
		var request = {
                	command: "addAlarmRules",
                	data: []
            	};
		var param = {};
		param.token = $("#scheduleCell").val();
		if ($("#scheduleMdEnable").is(":checked")) {
			param.enabled = "1";
		} else {
			param.enabled = "0";
		}
		param.event_name = $("select[name='event']").val();
		if ($("#saveStreamTo").is(":checked")) {
			if (typeof(param.actions) == "undefined") {
				param.actions = "SAVESTREAM:"+$("select[name='saveStreamTo']").val();
			} else {
				param.actions = param.actions + ","+"SAVESTREAM:"+$("select[name='saveStreamTo']").val();
			}
		}
		if ($("#sendSnapshotTo").is(":checked")) {
			if (typeof(param.actions) == "undefined") {
				param.actions = "SAVESNAPSHOT:"+$("select[name='sendSnapshotTo']").val();
			} else {
				param.actions = param.actions + ","+"SAVESNAPSHOT:"+$("select[name='sendSnapshotTo']").val();
			}
		}
		if ($("#activateDigitalOutput").is(":checked")) {
			if (typeof(param.actions) == "undefined") {
				param.actions = "ACTIVATEDIGITALOUTPUT";
			} else {
				param.actions = param.actions+","+"ACTIVATEDIGITALOUTPUT";
			}
		}
		if ($("#sendToEmail").is(":checked")) {
			if (typeof(param.actions) == "undefined") {
				param.actions = "SENDTOEMAIL";
			} else {
				param.actions = param.actions+","+"SENDTOEMAIL";
			}
		}

		if ($("#typeAlways").is(":checked")) {
			param.schedule_type = "0";
		}
		if ($("#typeSchedule").is(":checked")) {
			param.schedule_type = "1";
		}

		var schedule_time = alarm_schedule.scheduleConvertToString();
		param.schedule_time = schedule_time;

		if (typeof(param.actions) == "undefined") {
			alarm_schedule.showWarn("danger", "<strong>Try Again!</strong>  action is needed.");
			return -1;
		}
		if ($('#scheduleCell').attr("readonly")!=="readonly") {
			for (var i = 0; i < alarm_schedule.rules.length; i++) {
				if (param.token === alarm_schedule.rules[i].token) {
					alarm_schedule.showWarn("danger", "<strong>Try Again!</strong>  rule named "+param.token+" exists.");
					return -1;
				}
			}
		}
		if ($('#scheduleCell').val()==="") {
			alarm_schedule.showWarn("danger", "<strong>Try Again!</strong>  rule name is needed.");
			return -1;
		}
	     	request.data.push(param);

            	$.post("/cgi-bin/alarm.cgi", JSON.stringify(request), function(o) {
                	if (o.status === "succeed") {
				alarm_schedule.initBasic();
                   		showAlert("success", "<strong>Success!</strong>  Schedule config saved.");
                	}
                	else if (o.status === "failed")
                    		showAlert("danger", "<strong>Try Again!</strong>  Schedule config save failed.");
            	}, "json");
		return 0;
    	},

	setRule:function() {
		if (alarm_schedule.data.length == 0) {
			alarm_schedule.resetRule();
			return;
		}
		$("#scheduleCell").val(alarm_schedule.data.token);
		if (alarm_schedule.data.enabled === "1") {
			$("#scheduleMdEnable").prop("checked", true);
		} else {
			$("#scheduleMdEnable").prop("checked", false);
		}
		if (alarm_schedule.data.actions.indexOf("SAVESTREAM:SD") >= 0) {
			$("select[name='saveStreamTo']").val("SD");
			$("#saveStreamTo").prop("checked", true);
              }
		if (alarm_schedule.data.actions.indexOf("SAVESTREAM:E-mail") >= 0) {
			$("select[name='saveStreamTo']").val("E-mail");
			$("#saveStreamTo").prop("checked", true);
		}
		if (alarm_schedule.data.actions.indexOf("SNAPSHOT:SD") >= 0) {
			$("select[name='sendSnapshotTo']").val("SD");
			$("#sendSnapshotTo").prop("checked", true);
		}
		if (alarm_schedule.data.actions.indexOf("SNAPSHOT:E-mail") >= 0) {
			$("select[name='sendSnapshotTo']").val("E-mail");
			$("#sendSnapshotTo").prop("checked", true);
		}
		if (alarm_schedule.data.actions.indexOf("ACTIVATEDIGITALOUTPUT") >= 0) {
			$("#activateDigitalOutput").prop("checked", true);
		}
		$("select[name='event']").val(alarm_schedule.data.event_name);
		if (alarm_schedule.data.schedule_type==="0") {
			$("#typeAlways").prop("checked", true);
			$("#scheduleTable").hide();
		}
		if (alarm_schedule.data.schedule_type==="1") {
			$("#typeSchedule").prop("checked", true);
			$("#scheduleTable").show();
		}
		alarm_schedule.scheduleConvert(alarm_schedule.data.schedule_time);
		alarm_schedule.drawSchedule();
	},

	resetRule:function() {
    		$("#scheduleCell").val("Realtek");
    		$("#scheduleMdEnable").prop("checked", true);
		$("#saveStreamTo").prop("checked", false);
		$("select[name='saveStreamTo']").val("SD");
		$("#sendSnapshotTo").prop("checked", false);
		$("select[name='sendSnapshotTo']").val("SD");
		$("#activateDigitalOutput").prop("checked", false);
		$("#sendToEmail").prop("checked", false);
		$("select[name='event']").val("MD");
		$("#typeSchedule").prop("checked", true);
		$("#scheduleTable").show();
		var canvas=document.getElementById('myCanvas');
		var ctx=canvas.getContext('2d');
		for (var i=0;i<7;i++) {
			for (var j=0;j<24;j++) {
				alarm_schedule.schedule_rect[i][j]=0;
				ctx.clearRect(j*40+60+1,i*30+60+1,38,28);
			}
		}
		alarm_schedule.data = [];
    	},
	showWarn:function(status, alertContent) {
    		$("#responseWarn").removeClass("alert-success alert-info alert-warning alert-danger").addClass("alert-" + status)
        	.html(alertContent).slideDown(1000).delay(5000).slideUp(1000);
	},
	initBasic: function() {
		$.ajaxSetup("cache", false);
		alarm_schedule.schedule_rect = new Array();
		for (var i = 0;i<7;i++) {
	 		alarm_schedule.schedule_rect[i] = new Array();
	 		for (var j = 0;j<24;j++) {
	 			alarm_schedule.schedule_rect[i][j]=0;
	 		}
	 	}
		$("#eventList tbody").html("");
		alarm_schedule.rules = [];
		alarm_schedule.data = [];
		var request = {
              	command: "getAlarmRules",
			data: []
       	};
       	$.post("/cgi-bin/alarm.cgi", JSON.stringify(request), function(o) {
			if (o.status == "succeed") {
				alarm_schedule.rules = o.data;
				$.each(o.data, function(i, rule) {
					alarm_schedule.addRecordEvent(i, rule);
				});
			} else if (o.status === "failed") {
                	showAlert("danger", "<strong>Try Again!</strong>  Get alarm rules failed.");
			}
		}, "json");
    	},

    	bindEvent: function() {
		$("#btn_selectall").on("click",function() {
			alarm_schedule.selectAll();
		});
		$("#btn_clearall").on("click",function() {
			alarm_schedule.clearAll();
		});
		$("#btn_apply").on("click",function() {
			if (alarm_schedule.addRule() == 0) {
				$("#setModal").modal("hide");
			}
		});
		$("#btn_cancel").on("click",function() {
			$("#setModal").modal("hide");
		});
		$("#btn_undoall").on("click",function() {
			alarm_schedule.setRule();
		});
		$("#btn_add").on("click", function() {
			alarm_schedule.resetRule();
			$('#scheduleCell').attr("readonly", false);
			$("#setModal").modal("show");
		});
		$('input:radio[name="scheduleType"]').change(function() {
			if ($("#typeAlways").is(":checked")) {
				$("#scheduleTable").hide();
			} else {
				$("#scheduleTable").show();
			}
		});
	}
};

$(function() {
    alarm_schedule.initBasic();
    alarm_schedule.initScheduleTable();
    alarm_schedule.bindEvent();
});
