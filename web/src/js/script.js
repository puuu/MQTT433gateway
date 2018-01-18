$(function () {
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

    var passwordFieldFactory = function (item) {
        return '<label for="' + item.name + '">' + item.name + '</label>' +
            '<input type="password" class="pure-input-1 config-item" id="' + item.name + '" name="' + item.name + '" data-field="' + item.name + '">' +
            '<span class="pure-form-message">' + item.help + '</span>';
    };

    var checkboxFactory = function (item) {
        return '<label class="pure-checkbox">' +
            '<input class=" config-item " type="checkbox" value="' + item.name + '" data-field="' + item.name + '" name="' + item.name + '"> ' +
            item.name +
            '<span class="pure-form-message">' + item.help + '</span>' +
            '</label>';
    };

    var legendFactory = function (item) {
        return '<legend>' + item.name + '</legend>';
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
                    ' Protocol ' + value +
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

    var passwordGet = function (item_id) {
        var pwd = $('.config-item[data-field="' + item_id + '"]').val();
        if (pwd.length < 8) {
            alert("Password must have at least 8 characters");
            return undefined;
        }
        return pwd;
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
            return $(x).val();
        });
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
        new ConfigItem("configPassword", passwordFieldFactory, inputApply, passwordGet, "The admin password for the web UI (min. 8 characters)"),

        new GroupItem("MQTT Config", legendFactory),
        new ConfigItem("mqttBroker", inputFieldFactory, inputApply, inputGet, "MQTT Broker host"),
        new ConfigItem("mqttBrokerPort", inputFieldFactory, inputApply, inputGetInt, "MQTT Broker port"),
        new ConfigItem("mqttUser", inputFieldFactory, inputApply, inputGet, "MQTT username (optional)"),
        new ConfigItem("mqttPassword", passwordFieldFactory, inputApply, inputGet, "MQTT password (optional)"),
        new ConfigItem("mqttRetain", checkboxFactory, checkboxApply, checkboxGet, "Retain MQTT messages"),

        new GroupItem("MQTT Topic Config", legendFactory),
        new ConfigItem("mqttReceiveTopic", inputFieldFactory, inputApply, inputGet, "Topic to publish received signal"),
        new ConfigItem("mqttSendTopic", inputFieldFactory, inputApply, inputGet, "Topic to get signals to send from"),

        new GroupItem("433MHz RF Config", legendFactory),
        new ConfigItem("rfEchoMessages", checkboxFactory, checkboxApply, checkboxGet, "Echo sent rf messages back"),
        new ConfigItem("rfReceiverPin", inputFieldFactory, inputApply, inputGetInt, "The GPIO pin used for the rf receiver"),
        new ConfigItem("rfTransmitterPin", inputFieldFactory, inputApply, inputGetInt, "The GPIO pin used for the RF transmitter"),

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

    var closeWebSocket = function () {
    };

    var openWebSocket = function () {
        var container = $('#log-container');
        var pre = container.find('pre');

        var webSocket = new WebSocket("ws://" + location.hostname + ":81");
        var tm;

        closeWebSocket = function () {
            clearTimeout(tm);
            webSocket.close();
        };

        var ping = function () {
            tm = setTimeout(function () {
                webSocket.send("__PING__");

                tm = setTimeout(function () {
                    webSocket.close();
                    openWebSocket();
                }, 2000);
            }, 5000);
        };

        webSocket.onmessage = function (e) {
            var message = e.data;

            if (message === "__PONG__") {
                clearTimeout(tm);
                ping();
                return;
            }

            pre.append(message);
            if (message === '\n' || (typeof message === "string" && message.indexOf('\n') !== -1)) {
                container.scrollTop(pre.get(0).scrollHeight);
            }
        };

        webSocket.onerror = function (ev) {
            webSocket.close();
            setTimeout(function () {
                openWebSocket();
            });
        };

        webSocket.onopen = function (ev) {
            ping();
        };
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
        };
    }


    var registerConfigUi = function (item) {
        var _item = $(item);
        _item.change(function () {
            var name = _item.data("field");
            var new_data = ui_map[name].fetch(name);
            if (new_data !== undefined && JSON.stringify(last_cfg[name]) !== JSON.stringify(new_data)) {
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


    var SystemCommandActions = {
        restart: function () {
            var _body = $("body");
            _body.empty();
            _body.append("<p>Device will reboot!</p><p>Try to reconnect in 15 seconds.</p>");
            setTimeout(function () {
                window.location.reload(true);
            }, 15000);
        },
        reset_wifi: function () {
            var _body = $("body");
            _body.empty();
            _body.append("<p>Devices WIFI settings where cleared!</p><p>Please reconfigure it.</p>");
        },
        reset_config: function () {
            var _body = $("body");
            _body.empty();
            _body.append("<p>Devices Config was reset - reboot device!</p>" +
                "<p>You might have to reconfigure the wifi!</p>" +
                "<p>Reload page in 10 seconds...</p>");
            setTimeout(function () {
                window.location.reload(true);
            }, 10000);
        }
    };

    var sendCommand = throttle(
        function (params) {
            $.ajax({
                       url: '/system',
                       type: 'POST',
                       data: JSON.stringify(params),
                       contentType: 'application/json',
                       success: function () {
                           SystemCommandActions[params.command]();
                       }
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
            registerConfigUi('.config-item[data-field="' + item.name + '"]');
        });
        loadConfig();
        loadDebug();
    };


    var loadDebug = function () {
        var applyDebug = function (data) {
            $.each(data, function (key, value) {
                $('.debug-item[data-dbg-field="' + key + '"]').prop("checked", value);
            });
        };

        $.ajax({
                   url: "/debug",
                   type: "GET",
                   contentType: 'application/json',
                   success: applyDebug
               });
    };


    var saveDebug = function () {
        var data = {};
        $(".debug-item").each(function (_, item) {
            var _item = $(item);
            data[_item.attr("name")] = _item.prop("checked");
        });

        $.ajax({
                   url: '/debug',
                   type: "PUT",
                   data: JSON.stringify(data),
                   contentType: 'application/json',
                   success: loadDebug
               });
    };

    var loadFwVersion = function () {
        $.ajax({
                   url: "/firmware",
                   type: "GET",
                   contentType: 'application/json',
                   success: function (data) {
                       $('#current-fw-version').append('Current version: ' + data["version"]);
                   }
               })
    };

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
                   success: loadConfig

               });

        return false;
    });

    $('#cfg-form-reset').click(function (e) {
        e.preventDefault();
        loadConfig();
        return false;
    });

    $("#debugging-form").change(function (e) {
        e.preventDefault();

        saveDebug();
        return false;
    });

    $('#dbg-form-reset').click(function (e) {
        e.preventDefault();
        loadDebug();
        return false;
    });

    // Clear log
    $('#btn-clear-log').click(function (e) {
        $('#log-container').find('pre').empty();
    });

    $('#update-form').submit(function () {
        closeWebSocket();
    });

    initUi();
    loadFwVersion();
    openWebSocket();
});
