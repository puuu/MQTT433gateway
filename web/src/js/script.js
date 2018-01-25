$(function () {
    'strict mode';

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
        new ConfigItem("rfReceiverPinPullUp", checkboxFactory, checkboxApply, checkboxGet, "Activate pullup on rf receiver pin (required for 5V protection with reverse diode)"),
        new ConfigItem("rfTransmitterPin", inputFieldFactory, inputApply, inputGetInt, "The GPIO pin used for the RF transmitter"),

        new GroupItem("Enabled RF protocols", legendFactory),
        new ConfigItem("rfProtocols", protocolInputField, protocolApply, protocolGet, ""),

        new GroupItem("Log Config", legendFactory),
        new ConfigItem("serialLogLevel", logLevelInputFactory, inputApply, inputGet, "Level for serial logging"),
        new ConfigItem("webLogLevel", logLevelInputFactory, inputApply, inputGet, "Level for logging to the web UI"),
        new ConfigItem("syslogLevel", logLevelInputFactory, inputApply, inputGet, "Level for syslog logging"),
        new ConfigItem("syslogHost", inputFieldFactory, inputApply, inputGet, "Syslog server (optional)"),
        new ConfigItem("syslogPort", inputFieldFactory, inputApply, inputGetInt, "Syslog port (optional)"),

        new GroupItem("Status LED", legendFactory),
        new ConfigItem("ledPin", inputFieldFactory, inputApply, inputGetInt, "The GPIO pin used for the status LED"),
        new ConfigItem("ledActiveHigh", checkboxFactory, checkboxApply, checkboxGet, "The way how the LED is connected to the pin (false for built-in led)")
    ];

    var DEBUG_FLAGS = {
        protocolRaw: "Enable Raw RF message logging",
        systemLoad: "Show the processed loop() iterations for each second",
        freeHeap: "Show the free heap memory every second"
    };

    function inputLabelFactory(item) {
        return $('<label>', {
            text: item.name,
            for: 'cfg-' + item.name,
        });
    }

    function inputHelpFactory(item) {
        return $('<span>', {
            class: 'pure-form-message',
            text: item.help,
        });
    }

    function logLevelInputFactory(item) {
        var element = $('<select>', {
            class: 'config-item',
            id: 'cfg-' + item.name,
            name: item.name,
        }).append([
            $('<option>', {value: '', text: "None"}),
            $('<option>', {value: 'error', text: 'Error'}),
            $('<option>', {value: 'warning', text: 'Warning'}),
            $('<option>', {value: 'info', text: 'Info'}),
            $('<option>', {value: 'debug', text: 'Debug'}),
        ]);
        registerConfigUi(element, item);
        return [
            inputLabelFactory(item),
            element,
            inputHelpFactory(item),
        ];
    }

    function inputFieldFactory(item) {
        var element = $('<input>', {
            type: 'text',
            class: 'pure-input-1 config-item',
            id: 'cfg-' + item.name,
            name: item.name,
        });
        registerConfigUi(element, item);
        return [
            inputLabelFactory(item),
            element,
            inputHelpFactory(item),
        ];
    }

    function passwordFieldFactory(item) {
        var element = $('<input>', {
            type: 'password',
            class: 'pure-input-1 config-item',
            id: 'cfg-' + item.name,
            name: item.name,
        });
        registerConfigUi(element, item);
        return [
            inputLabelFactory(item),
            element,
            inputHelpFactory(item),
        ];
    }

    function checkboxFactory(item) {
        var element = $('<input>', {
            type: 'checkbox',
            class: 'config-item',
            id: 'cfg-' + item.name,
            name: item.name,
        });
        registerConfigUi(element, item);
        return $('<label>', {
            class: 'pure-checkbox',
        }).append([
            element,
            ' ' + item.name,
            inputHelpFactory(item),
        ]);
    }

    function legendFactory(item) {
        return $('<legend>', { text: item.name });
    }

    var protocols;
    function protocolInputField(item) {
        var container = $('<div>', { id: 'cfg-' + item.name });
        registerConfigUi(container, item);
        function protocolListFactory(protos) {
            protos.forEach(function (value) {
                var element = $('<input>', {
                    type: 'checkbox',
                    class: 'config-item protocols-item',
                    id: 'cfg-' + item.name + '-' + value,
                    name: item.name,
                    value: value,
                });
                container.append($('<label>', {
                    class: 'pure-checkbox',
                }).append([
                    element,
                    ' Protocol ' + value,
                ]));
            });
            protocols = protos;
        }
        $.ajax({
           url: "/protocols",
           type: "GET",
           contentType: 'application/json',
           success: protocolListFactory
        });
        return container;
    }

    function inputApply(itemName, data) {
        $('#cfg-' + itemName).val(data);
    }

    function checkboxApply(itemName, data) {
        $('#cfg-' + itemName).prop("checked", data);
    }

    function protocolApply(itemName, data) {
        if (protocols === undefined) {
            setTimeout(protocolApply(itemName, data), 100);
            return;
        }
        if (data.length == 0) {
            data = protocols;
        }
        data.forEach(function (value) {
            $('#cfg-' + itemName + '-' + value).prop('checked', true);
        });
    }

    function inputGet(element) {
        return element.val();
    }

    function passwordGet(element) {
        var pwd = element.val();
        if (pwd.length < 8) {
            alert("Password must have at least 8 characters");
            return undefined;
        }
        return pwd;
    }

    function inputGetInt(element) {
        return parseInt(inputGet(element));
    }

    function checkboxGet(element) {
        return element.prop("checked");
    }

    function protocolGet(element) {
        var checked = $('.protocols-item:checked');
        if ($('.protocols-item').length === checked.length) {
            return [];
        }
        return $.map(checked, function (x) {
            return $(x).val();
        });
    }

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

    function openWebSocket() {
        var container = $('#log-container');
        var pre = container.find('pre');

        var webSocket = new WebSocket("ws://" + location.hostname + ":81");
        var tm;

        function ping() {
            clearTimeout(tm);
            tm = setTimeout(function () {
                webSocket.send("__PING__");
                tm = setTimeout(function () {
                    webSocket.close();
                    openWebSocket();
                }, 2000);
            }, 5000);
        }

        webSocket.onmessage = function (event) {
            var message = event.data;

            if (message === "__PONG__") {
                ping();
                return;
            }

            pre.append(message);
            if (message === '\n' || (typeof message === "string" && message.indexOf('\n') !== -1)) {
                container.scrollTop(pre.get(0).scrollHeight);
            }
        };

        webSocket.onerror = function (event) {
            webSocket.close();
            if (tm === undefined) {
                openWebSocket();
            }
        };

        webSocket.onopen = function (event) {
            ping();
        };
    }

    var lastConfig = {};
    var changes = {};

    function throttle(callback, limit) {
        var wait = false;
        return function () {
            if (!wait) {
                callback.apply(this, arguments);
                wait = true;
                setTimeout(function () {
                    wait = false;
                }, limit);
            }
        };
    }

    function registerConfigUi(element, item) {
        element.change(function (event) {
            var newData = item.fetch(element);
            if (newData !== undefined) {
                if (JSON.stringify(lastConfig[item.name]) !== JSON.stringify(newData)) {
                    changes[item.name] = newData;
                } else {
                    delete changes[item.name];
                }
            }
        });
    }

    function applyConfig(data) {
        CONFIG_ITEMS.forEach(function (item) {
            if (item.apply) {
                item.apply(item.name, data[item.name]);
            }
        });
        changes = {};
        lastConfig = data;
    }

    function loadConfig() {
        $.ajax({
                   url: '/config',
                   type: 'GET',
                   contentType: 'application/json',
                   success: applyConfig
               });
    }

    var SystemCommandActions = {
        restart: function () {
            var body = $("body");
            body.empty();
            body.append("<p>Device will reboot!</p><p>Try to reconnect in 15 seconds.</p>");
            setTimeout(function () {
                window.location.reload(true);
            }, 15000);
        },
        reset_wifi: function () {
            var body = $("body");
            body.empty();
            body.append("<p>Devices WIFI settings where cleared!</p><p>Please reconfigure it.</p>");
        },
        reset_config: function () {
            var body = $("body");
            body.empty();
            body.append("<p>Devices Config was reset - reboot device!</p>" +
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

    function initConfigUi() {
        var settings = $("#settings");
        CONFIG_ITEMS.forEach(function (item) {
            settings.append(item.factory(item));
        });
        loadConfig();
    }

    function initDebugUi(debugFlags, container) {
        function create(debugFlag, helpText) {
            var checkbox = $('<input>', {
                type: 'checkbox',
                class: 'debug-item',
                id: 'debug-' + debugFlag,
                name: debugFlag,
            });
            checkbox.change(function (event) {
                submit(this);
            });
            return $('<label>', {class: 'pure-checkbox'}).append([
                checkbox,
                ' ' + debugFlag,
                $('<span>', {
                    class: 'pure-form-message',
                    text: helpText,
                }),
            ]);
        }

        function apply(data) {
            $.each(data, function (key, value) {
                $('#debug-' + key).prop("checked", value);
            });
        }

        function submit(item) {
            var data = {};
            data[item.name] = item.checked;
            $.ajax({
                url: '/debug',
                type: "PUT",
                data: JSON.stringify(data),
                contentType: 'application/json',
                success: apply
            });
        }

        $.each(debugFlags, function(debugFlag, helpText) {
            container.append(create(debugFlag, helpText));
        });
        $.ajax({
            url: "/debug",
            type: "GET",
            contentType: 'application/json',
            success: apply
        });
    }

    function loadFwVersion() {
        $.ajax({
                   url: "/firmware",
                   type: "GET",
                   contentType: 'application/json',
                   success: function (data) {
                       $('#current-fw-version').append('Current version: ' + data.version);
                   }
               });
    }

    $('.system-btn').click(function (event) {
        sendCommand({command: $(this).data('command')});
    });

    $('#settings-form').submit(function (event) {
        event.preventDefault();
        $.ajax({
                   url: "/config",
                   type: 'PUT',
                   contentType: 'application/json',
                   data: JSON.stringify(changes),
                   success: applyConfig

               });

        return false;
    });

    $('#cfg-form-reset').click(function (event) {
        event.preventDefault();
        loadConfig();
        return false;
    });

    // Clear log
    $('#btn-clear-log').click(function (event) {
        $('#log-container').find('pre').empty();
    });

    initConfigUi();
    initDebugUi(DEBUG_FLAGS, $("#debugflags"));
    loadFwVersion();
    openWebSocket();
});
