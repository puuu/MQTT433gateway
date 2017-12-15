var logLevelInputFactory = function (item) {
    return '<div class="form-group">' +
        '<label for="' + item.name + '">' + item.name + '</label>' +
        '<select class="form-control config-item" data-field="' + item.name + '">' +
        '<option value="">None</option>' +
        '<option value="error">Error</option>' +
        '<option value="warning">Warning</option>' +
        '<option value="info">Info</option>' +
        '<option value="debug">Debug</option>' +
        '</select>' +
        '</div>';
};


var inputFieldFactory = function (item) {
    return '<div class="form-group">' +
        '<label for="' + item.name + '">' + item.name + '</label>' +
        '<input type="text" class="form-control config-item" id="' + item.name + '" name="' + item.name + '" data-field="' + item.name + '">' +
        '</div>';
};

var protocolInputField = function (item) {
    return '<div id="rfProtocols"><h4>Enabled protocols</h4></div>';
};


var inputApply = function (item_id, data) {
    $('.config-item[data-field="' + item_id + '"]').val(data);
};

var protocolApply = function (item_id, data) {
    $.ajax("/protocols",
           {
               method: "GET",
               contentType: 'application/json'
           }).done(function (protos) {
        $("#rfProtocols").empty();
        protos.forEach(function (value) {
            var elem = '<div class="form-check">' +
                '<label class="form-check-label">' +
                '<input class="form-check-input config-item protocols-item" id="proto_check_' + value + '" type="checkbox" value="' + value + '" data-field="' + item_id + '" name="' + item_id + '">' +
                'Protocol ' + value +
                '</label>' +
                '</div>';
            $("#rfProtocols").append(elem)
            registerConfigUi('#proto_check_' + value);
        });
        if (data.length > 0) {
            data.forEach(function (value) {
                $('#proto_check_' + value).prop('checked', true);
            });
        } else {
            $(".protocols-item").each(function (_, value) {
                $(value).prop("checked", true);
            });
        }
    })
};

var inputGet = function (item_id) {
    return $('.config-item[data-field="' + item_id + '"]').val();
};

var protocolGet = function (item_id) {
    var checked = $('.protocols-item:checked');
    if ($('.protocols-item').length === checked.length) {
        return [];
    }
    return $.map($('.protocols-item:checked'), function (x) {
        return $(x).val()
    })
};

function ConfigItem(name, factory, apply, fetch, help) {
    this.name = name;
    this.factory = factory;
    this.apply = apply;
    this.fetch = fetch;
    this.help = help;
}

var CONFIG_ITEMS = [
    new ConfigItem("deviceName", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mdnsName", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttReceiveTopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttLogTopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttRawRopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttSendTopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttConfigTopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttOtaTopic", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttBroker", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttBrokerPort", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttUser", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttPassword", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("mqttRetain", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("rfReceiverPin", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("rfTransmitterPin", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("rfEchoMessages", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("rfProtocols", protocolInputField, protocolApply, protocolGet, "HELP"),
    new ConfigItem("otaPassword", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("otaUrl", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("serialLogLevel", logLevelInputFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("webLogLevel", logLevelInputFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("configUser", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("configPassword", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("syslogLevel", logLevelInputFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("syslogHost", inputFieldFactory, inputApply, inputGet, "HELP"),
    new ConfigItem("syslogPort", inputFieldFactory, inputApply, inputGet, "HELP")
];

var ui_map = {};
CONFIG_ITEMS.forEach(function (value) {
    ui_map[value.name] = value;
});


var webSocket = new WebSocket("ws://" + location.hostname + ":81");
webSocket.onmessage = function (e) {
    var message = e.data;
    var container = $('#log-container');
    var pre = container.find('>pre');
    pre.append(message);
    if (message === '\n' || (typeof message === "string" && message.indexOf('\n') !== -1)) {
        container.animate({scrollTop: pre.get(0).scrollHeight}, 10);
    }
};


var last_cfg = {};
var changes = {};

var registerConfigUi = function (item) {
    var _item = $(item)
    _item.change(function () {
        var name = _item.data("field");
        var new_data = ui_map[name].fetch(name);
        if (JSON.stringify(last_cfg[name]) !== JSON.stringify(new_data)) {
            changes[name] = new_data;
        }
    });
};

var loadConfig = function () {
    $.ajax(
        '/config',
        {
            method: 'GET',
            contentType: 'application/json'
        }
    ).done(function (data) {
               $.each(data, function (key, value) {
                   ui_map[key].apply(key, value);
               });
           }
    );
};


var sendCommand = _.throttle(
    function (params) {
        $.ajax(
            '/system',
            {
                method: 'POST',
                data: JSON.stringify(params),
                contentType: 'application/json'
            }
        );
    },
    1000
);


var sendConfig = function (config) {
    $.ajax(
        '/config',
        {
            method: 'PUT',
            data: JSON.stringify(config),
            contentType: 'application/json'
        }
    ).done(function (result) {
        if (result) {
            loadConfig();
        }
    });
};


var initUi = function () {
    var settings = "";
    CONFIG_ITEMS.forEach(function (item) {
        settings += item.factory(item);
    });
    $("#settings").prepend(settings);
    CONFIG_ITEMS.forEach(function (item) {
        registerConfigUi('.config-item[data-field="' + item.name + '"]')
    });
    loadConfig();
};


$(function () {
    $('.system-btn').click(function () {
        sendCommand({command: $(this).data('command')});
    });

    $('#web-log-level').change(function () {
        sendConfig({webLogLevel: $('#web-log-level').val()});
    });

    $('#settings').submit(function (e) {
        e.preventDefault();

        $.ajax(
            "/config",
            {
                method: 'PUT',
                contentType: 'application/json',
                data: JSON.stringify(changes)
            }
        ).done(function () {
            loadConfig();
        });

        return false;
    });

    initUi();
});
