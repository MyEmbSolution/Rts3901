var network_ipfilter = {
    ruleNumber: 0,
    maxRuleNumber: 8,
    validate: function () {
        var valid = true;
        if ($('#ipfilterMode').val() === '1' && !network_ipfilter.ruleNumber) {
            valid = false;
        } else {
            $.each($('#ipfilterRules tbody tr'), function () {
                var begin = $(this).find('input.beginIp').val();
                var end = $(this).find('input.endIp').val();
                valid = valid && regex.ip.test(begin);
                valid = valid && regex.ip.test(end);
                if (!valid)
                    return false;

                if (+begin.split('.')[0] !== +end.split('.')[0]
                    || +begin.split('.')[1] !== +end.split('.')[1]
                    || +begin.split('.')[2] !== +end.split('.')[2]
                    || +begin.split('.')[3] > +end.split('.')[3]
                ) {
                    valid = false;
                    return false;
                }
            });
        }
        $('#btn_save').prop('disabled', !valid);
    },
    enableStatusChange: function () {
        var state = $('#ipfilterEnable').bootstrapSwitch('state');
        $('#ipfilterMode').prop('disabled', !state);
        $('#ipfilterRules tbody input').prop('disabled', !state);
        $('#ipfilterRules tbody button').prop('disabled', !state);
        $('#btn_add').prop('disabled', !state);
        if (!state)
            $('#btn_save').prop('disabled', false);
        else
            network_ipfilter.validate();
    },
    addRule: function (o) {
        $('<tr>').html(''
            + '<td><input class="beginIp" value="' + (o ? o.begin_ip : '') + '"></td>'
            + '<td><input class="endIp" value="' + (o ? o.end_ip : '') + '"></td>'
            + '<td><button class="btn btn-danger btn-xs">Delete</button></td>'
        ).appendTo($('#ipfilterRules tbody'));
        ++network_ipfilter.ruleNumber;
    },
    initBasic: function () {
        $.ajaxSetup('cache', false);
        $('#ipfilterEnable').bootstrapSwitch('size', 'mini');
        var request = {
            command: 'getInfo',
            data: ["ipfilterInfo"]
        };
        $.post('/cgi-bin/network.cgi', JSON.stringify(request), function (o) {
            if (o.status === 'succeed') {
                $('#ipfilterEnable').bootstrapSwitch('state', o.data.ipfilterInfo.enable);
                $('#ipfilterMode').val(o.data.ipfilterInfo.mode);
                $.each(o.data.ipfilterInfo.rules, function (i, rule) {
                    network_ipfilter.addRule(rule);
                });
                network_ipfilter.enableStatusChange();
            } else if (o.status === 'failed') {
                showAlert('danger', '<strong>Try Again!</strong>  Get IP Filter config failed.');
            }
        }, 'json');
    },
    bindEvent: function () {
        $('#btn_add').on('click', function () {
            if (network_ipfilter.ruleNumber < network_ipfilter.maxRuleNumber) {
                network_ipfilter.addRule();
                network_ipfilter.validate();
            }
        });
        $('#ipfilterEnable').on('switchChange.bootstrapSwitch', function () {
            network_ipfilter.enableStatusChange();
        });
        $('#ipfilterMode').on('change', function () {
            network_ipfilter.validate();
        });
        $('#ipfilterRules').on('click', 'button.btn-danger', function () {
            $(this).parent().parent().remove();
            --network_ipfilter.ruleNumber;
            network_ipfilter.validate();
        });
        $('#ipfilterRules').on('keyup', 'input', function () {
            network_ipfilter.validate();
        });
        $('#btn_save').on('click', function () {
            var request = {
                command: 'setInfo',
                data: {
                    ipfilterInfo: {
                        enable: +$('#ipfilterEnable').bootstrapSwitch('state'),
                        mode: +$('#ipfilterMode').val(),
                        rules: []
                    }
                }
            };

            for (var i = 0; i < network_ipfilter.ruleNumber; i++) {
                request.data.ipfilterInfo.rules.push({
                    begin_ip: $('#ipfilterRules tbody tr:eq(' + i + ')').find('.beginIp').val(),
                    end_ip: $('#ipfilterRules tbody tr:eq(' + i + ')').find('.endIp').val()
                });
            }

            $('#btn_save').prop('disabled', true);
            $.post('/cgi-bin/network.cgi', JSON.stringify(request), function (o) {
                $('#btn_save').prop('disabled', false);
                if (o.status === 'succeed')
                    showAlert('success', '<strong>Success!</strong>  IPFilter config saved.');
                else if (o.status === 'failed')
                    showAlert('danger', '<strong>Try Again!</strong>  IPFilter config save failed.');
            }, 'json');
        });
    }
};

$(function () {
    network_ipfilter.initBasic();
    network_ipfilter.bindEvent();
});