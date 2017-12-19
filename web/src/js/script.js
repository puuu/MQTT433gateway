var logLevelInputFactory = function (item) {
    return '<label for="' + item.name + '">' + item.name + '</label>' +
        '<select class="config-item" data-field="' + item.name + '">' +
        '<option value="">None</option>' +
        '<option value="error">Error</option>' +
        '<option value="warning">Warning</option>' +
        '<option value="info">Info</option>' +
        '<option value="debug">Debug</option>' +
        '</select>' +
        '<span class="pure-form-message">' + item.help + '</span>';

};


var inputFieldFactory = function (item) {
    return '<label for="' + item.name + '">' + item.name + '</label>' +
        '<input type="text" class="pure-input-1 config-item" id="' + item.name + '" name="' + item.name + '" data-field="' + item.name + '">' +
        '<span class="pure-form-message">' + item.help + '</span>';
};

var checkboxFactory = function (item) {
    return '<label class="pure-checkbox">' +
        '<input class=" config-item " type="checkbox" value="' + item.name + '" data-field="' + item.name + '" name="' + item.name + '">' +
        item.name +
        '<span class="pure-form-message">' + item.help + '</span>' +
        '</label>';
};

var legendFactory = function (item) {
    return '<legend>' + item.name + '</legend>'
};

var protocolInputField = function (item) {
    return '<div id="rfProtocols"></div>';
};


var inputApply = function (item_id, data) {
    $('.config-item[data-field="' + item_id + '"]').val(data);
};

var checkboxApply = function (item_id, data) {
    $('.config-item[data-field="' + item_id + '"]').prop("checked", data);
};


var protocolApply = function (item_id, data) {

    var fillProtocolData = function (protos) {
        $("#rfProtocols").empty();
        protos.forEach(function (value) {
            var elem = '<label class="pure-checkbox">' +
                '<input class=" config-item protocols-item" id="proto_check_' + value + '" type="checkbox" value="' + value + '" data-field="' + item_id + '" name="' + item_id + '">' +
                'Protocol ' + value +
                '</label>';
            $("#rfProtocols").append(elem);
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
    };

    $.ajax({
               url: "/protocols",
               type: "GET",
               contentType: 'application/json',
               success: fillProtocolData
           });
};

var inputGet = function (item_id) {
    return $('.config-item[data-field="' + item_id + '"]').val();
};

var inputGetInt = function (item_id) {
    return parseInt(inputGet(item_id));
};

var checkboxGet = function (item_id) {
    return $('.config-item[data-field="' + item_id + '"]').prop("checked");
};

var protocolGet = function (item_id) {
    var checked = $('.protocols-item:checked');
    if ($('.protocols-item').length === checked.length) {
        return [];
    }
    return $.map(checked, function (x) {
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

function GroupItem(name, factory) {
    this.name = name;
    this.factory = factory;
}

var CONFIG_ITEMS = [
    new GroupItem("General Config", legendFactory),
    new ConfigItem("deviceName", inputFieldFactory, inputApply, inputGet, "The general name of the device"),
    new ConfigItem("mdnsName", inputFieldFactory, inputApply, inputGet, "The name the device shall announce via mdns"),
    new ConfigItem("configUser", inputFieldFactory, inputApply, inputGet, "The admin username for the web UI"),
    new ConfigItem("configPassword", inputFieldFactory, inputApply, inputGet, "The admin password for the web UI"),
    new ConfigItem("otaPassword", inputFieldFactory, inputApply, inputGet, "The password for pull OTA"),
    new ConfigItem("otaUrl", inputFieldFactory, inputApply, inputGet, "The OTA pull URL"),

    new GroupItem("MQTT Config", legendFactory),
    new ConfigItem("mqttBroker", inputFieldFactory, inputApply, inputGet, "MQTT Broker host"),
    new ConfigItem("mqttBrokerPort", inputFieldFactory, inputApply, inputGetInt, "MQTT Broker port"),
    new ConfigItem("mqttUser", inputFieldFactory, inputApply, inputGet, "MQTT username (optional)"),
    new ConfigItem("mqttPassword", inputFieldFactory, inputApply, inputGet, "MQTT password (optional)"),
    new ConfigItem("mqttRetain", checkboxFactory, checkboxApply, checkboxGet, "Retain MQTT messages"),

    new GroupItem("MQTT Topic Config", legendFactory),
    new ConfigItem("mqttReceiveTopic", inputFieldFactory, inputApply, inputGet, "Topic to publish received signal"),
    new ConfigItem("mqttLogTopic", inputFieldFactory, inputApply, inputGet, "Topic to publish log signals"),
    new ConfigItem("mqttRawRopic", inputFieldFactory, inputApply, inputGet, "Topic to publish raw signals"),
    new ConfigItem("mqttSendTopic", inputFieldFactory, inputApply, inputGet, "Topic to get signals to send from"),
    new ConfigItem("mqttConfigTopic", inputFieldFactory, inputApply, inputGet, "Topic to get config values from"),
    new ConfigItem("mqttOtaTopic", inputFieldFactory, inputApply, inputGet, "Topic to get OTA update information from"),

    new GroupItem("433MHz RF Config", legendFactory),
    new ConfigItem("rfEchoMessages", checkboxFactory, checkboxApply, checkboxGet, "Echo sent rf messages back"),
    new ConfigItem("rfReceiverPin", inputFieldFactory, inputApply, inputGetInt, "The pin used for the rf receiver"),
    new ConfigItem("rfTransmitterPin", inputFieldFactory, inputApply, inputGetInt, "The pin used for the RF transmitter"),

    new GroupItem("Enabled RF protocols", legendFactory),
    new ConfigItem("rfProtocols", protocolInputField, protocolApply, protocolGet, ""),

    new GroupItem("Log Config", legendFactory),
    new ConfigItem("serialLogLevel", logLevelInputFactory, inputApply, inputGet, "Level for serial logging"),
    new ConfigItem("webLogLevel", logLevelInputFactory, inputApply, inputGet, "Level for logging to the web UI"),
    new ConfigItem("syslogLevel", logLevelInputFactory, inputApply, inputGet, "Level for syslog logging"),
    new ConfigItem("syslogHost", inputFieldFactory, inputApply, inputGet, "Syslog server (optional)"),
    new ConfigItem("syslogPort", inputFieldFactory, inputApply, inputGetInt, "Syslog port (optional)")
];

var ui_map = {};
CONFIG_ITEMS.forEach(function (value) {
    if (value.fetch !== undefined) {
        ui_map[value.name] = value;
    }
});


var webSocket = new WebSocket("ws://" + location.hostname + ":81");
webSocket.onmessage = function (e) {
    var message = e.data;
    var container = $('#log-container');
    var pre = container.find('pre');
    pre.append(message);
    if (message === '\n' || (typeof message === "string" && message.indexOf('\n') !== -1)) {
        container.scrollTop(pre.get(0).scrollHeight);
    }
};


var last_cfg = {};
var changes = {};

function throttle(callback, limit) {
    var wait = false;
    return function (args) {
        if (!wait) {
            callback.apply(this, arguments);
            wait = true;
            setTimeout(function () {
                wait = false;
            }, limit);
        }
    }
}


var registerConfigUi = function (item) {
    var _item = $(item);
    _item.change(function () {
        var name = _item.data("field");
        var new_data = ui_map[name].fetch(name);
        if (JSON.stringify(last_cfg[name]) !== JSON.stringify(new_data)) {
            changes[name] = new_data;
        }
    });
};

var loadConfig = function () {
    var applyConfig = function (data) {
        $.each(data, function (key, value) {
            ui_map[key].apply(key, value);
        });
        changes = {};
    };

    $.ajax({
               url: '/config',
               type: 'GET',
               contentType: 'application/json',
               success: applyConfig
           });
};


var sendCommand = throttle(
    function (params) {
        $.ajax({
                   url: '/system',
                   type: 'POST',
                   data: JSON.stringify(params),
                   contentType: 'application/json'
               }
        );
    },
    1000
);


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


    $('#settings-form').submit(function (e) {
        e.preventDefault();

        $.ajax({
                   url: "/config",
                   type: 'PUT',
                   contentType: 'application/json',
                   data: JSON.stringify(changes),
                   sucess: loadConfig

               });

        return false;
    });

        $('#cfg-form-reset').click(function (e) {
        e.preventDefault();

        loadConfig();

        return false;
    });

    initUi();
});
