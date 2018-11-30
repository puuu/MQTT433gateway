/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2017, 2018 Jan Losinski, Puuu

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

import settingUi from './setting-ui';
import debugUi from './debugflag-ui';
import commandUi from './command-ui';
import updateUi from './update-ui';
import logUi from './log-ui';

require('purecss/build/pure-min.css');
require('purecss/build/grids-responsive-min.css');
require('../css/style.css');
/* global $ */

$(() => {
  'strict mode';

  const CONFIG_ITEMS = [
    new settingUi.GroupItem('General Config'),
    new settingUi.ConfigItem('deviceName', settingUi.Type.deviceName, 'The general name of the device'),
    new settingUi.ConfigItem('configPassword', settingUi.Type.devicePassword, 'The admin password for the web UI (min. 8 characters)'),

    new settingUi.GroupItem('MQTT Config'),
    new settingUi.ConfigItem('mqttBroker', settingUi.Type.hostName, 'MQTT Broker host'),
    new settingUi.ConfigItem('mqttBrokerPort', settingUi.Type.portNumber, 'MQTT Broker port'),
    new settingUi.ConfigItem('mqttUser', settingUi.Type.text, 'MQTT username (optional)'),
    new settingUi.ConfigItem('mqttPassword', settingUi.Type.password, 'MQTT password (optional)'),
    new settingUi.ConfigItem('mqttRetain', settingUi.Type.checkbox, 'Retain MQTT messages'),

    new settingUi.GroupItem('MQTT Topic Config'),
    new settingUi.ConfigItem('mqttReceiveTopic', settingUi.Type.mqttTopic, 'Topic to publish received signal'),
    new settingUi.ConfigItem('mqttSendTopic', settingUi.Type.mqttTopic, 'Topic to get signals to send from'),
    new settingUi.ConfigItem('mqttStateTopic', settingUi.Type.mqttTopic, 'Topic to publish the device state'),
    new settingUi.ConfigItem('mqttVersionTopic', settingUi.Type.mqttTopic, 'Topic to publish the current device version'),

    new settingUi.GroupItem('433MHz RF Config'),
    new settingUi.ConfigItem('rfEchoMessages', settingUi.Type.checkbox, 'Echo sent rf messages back'),
    new settingUi.ConfigItem('rfReceiverPin', settingUi.Type.pinNumber, 'The GPIO pin used for the rf receiver'),
    new settingUi.ConfigItem('rfReceiverPinPullUp', settingUi.Type.checkbox, 'Activate pullup on rf receiver pin (required for 5V protection with reverse diode)'),
    new settingUi.ConfigItem('rfTransmitterPin', settingUi.Type.pinNumber, 'The GPIO pin used for the RF transmitter'),

    new settingUi.GroupItem('Enabled RF protocols'),
    new settingUi.ConfigItem('rfProtocols', settingUi.Type.protocol),

    new settingUi.GroupItem('Log Config'),
    new settingUi.ConfigItem('serialLogLevel', settingUi.Type.logLevel, 'Level for serial logging'),
    new settingUi.ConfigItem('webLogLevel', settingUi.Type.logLevel, 'Level for logging to the web UI'),
    new settingUi.ConfigItem('syslogLevel', settingUi.Type.logLevel, 'Level for syslog logging'),
    new settingUi.ConfigItem('syslogHost', settingUi.Type.hostName, 'Syslog server (optional)'),
    new settingUi.ConfigItem('syslogPort', settingUi.Type.portNumber, 'Syslog port (optional)'),

    new settingUi.GroupItem('Status LED'),
    new settingUi.ConfigItem('ledPin', settingUi.Type.pinNumber, 'The GPIO pin used for the status LED'),
    new settingUi.ConfigItem('ledActiveHigh', settingUi.Type.checkbox, 'The way how the LED is connected to the pin (false for built-in led)'),
  ];

  const DEBUG_FLAGS = {
    protocolRaw: 'Enable Raw RF message logging',
    systemLoad: 'Show the processed loop() iterations for each second',
    freeHeap: 'Show the free heap memory every second',
  };

  const SystemCommandActions = {
    restart() {
      const body = $('body');
      body.empty();
      body.append('<p>Device will reboot!</p><p>Try to reconnect in 15 seconds.</p>');
      setTimeout(() => {
        window.location.reload(true);
      }, 15000);
    },
    reset_wifi() {
      const body = $('body');
      body.empty();
      body.append('<p>Devices WIFI settings where cleared!</p><p>Please reconfigure it.</p>');
    },
    reset_config() {
      const body = $('body');
      body.empty();
      body.append('<p>Devices Config was reset - reboot device!</p>'
                + '<p>You might have to reconfigure the wifi!</p>'
                + '<p>Reload page in 10 seconds...</p>');
      setTimeout(() => {
        window.location.reload(true);
      }, 10000);
    },
  };

  function onConnect() {
    updateUi.loadVersion();
    debugUi.load();
  }

  settingUi.init(CONFIG_ITEMS);
  debugUi.init(DEBUG_FLAGS, $('#debugflags'));
  commandUi.init(SystemCommandActions);
  updateUi.init();
  logUi.init(onConnect);
});
