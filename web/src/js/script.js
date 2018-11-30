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

import gateway from './gateway';
import settingUi from './setting-ui';
import debugUi from './debugflag-ui';
import commandUi from './command-ui';

require('purecss/build/pure-min.css');
require('purecss/build/grids-responsive-min.css');
require('../css/style.css');
/* global $ */

$(() => {
  'strict mode';

  const CONFIG_ITEMS = [
    new settingUi.GroupItem('General Config', settingUi.legendFactory),
    new settingUi.ConfigItem('deviceName', settingUi.deviceNameInputFactory, settingUi.inputApply, settingUi.inputGet, 'The general name of the device'),
    new settingUi.ConfigItem('configPassword', settingUi.devicePasswordInputFactory, settingUi.devicePasswordApply, settingUi.inputGet, 'The admin password for the web UI (min. 8 characters)'),

    new settingUi.GroupItem('MQTT Config', settingUi.legendFactory),
    new settingUi.ConfigItem('mqttBroker', settingUi.hostNameInputFactory, settingUi.inputApply, settingUi.inputGet, 'MQTT Broker host'),
    new settingUi.ConfigItem('mqttBrokerPort', settingUi.portNumberInputFactory, settingUi.inputApply, settingUi.inputGetInt, 'MQTT Broker port'),
    new settingUi.ConfigItem('mqttUser', settingUi.inputFieldFactory, settingUi.inputApply, settingUi.inputGet, 'MQTT username (optional)'),
    new settingUi.ConfigItem('mqttPassword', settingUi.passwordFieldFactory, settingUi.inputApply, settingUi.inputGet, 'MQTT password (optional)'),
    new settingUi.ConfigItem('mqttRetain', settingUi.checkboxFactory, settingUi.checkboxApply, settingUi.checkboxGet, 'Retain MQTT messages'),

    new settingUi.GroupItem('MQTT Topic Config', settingUi.legendFactory),
    new settingUi.ConfigItem('mqttReceiveTopic', settingUi.mqttTopicInputFactory, settingUi.inputApply, settingUi.inputGet, 'Topic to publish received signal'),
    new settingUi.ConfigItem('mqttSendTopic', settingUi.mqttTopicInputFactory, settingUi.inputApply, settingUi.inputGet, 'Topic to get signals to send from'),
    new settingUi.ConfigItem('mqttStateTopic', settingUi.mqttTopicInputFactory, settingUi.inputApply, settingUi.inputGet, 'Topic to publish the device state'),
    new settingUi.ConfigItem('mqttVersionTopic', settingUi.mqttTopicInputFactory, settingUi.inputApply, settingUi.inputGet, 'Topic to publish the current device version'),

    new settingUi.GroupItem('433MHz RF Config', settingUi.legendFactory),
    new settingUi.ConfigItem('rfEchoMessages', settingUi.checkboxFactory, settingUi.checkboxApply, settingUi.checkboxGet, 'Echo sent rf messages back'),
    new settingUi.ConfigItem('rfReceiverPin', settingUi.pinNumberInputFactory, settingUi.inputApply, settingUi.inputGetInt, 'The GPIO pin used for the rf receiver'),
    new settingUi.ConfigItem('rfReceiverPinPullUp', settingUi.checkboxFactory, settingUi.checkboxApply, settingUi.checkboxGet, 'Activate pullup on rf receiver pin (required for 5V protection with reverse diode)'),
    new settingUi.ConfigItem('rfTransmitterPin', settingUi.pinNumberInputFactory, settingUi.inputApply, settingUi.inputGetInt, 'The GPIO pin used for the RF transmitter'),

    new settingUi.GroupItem('Enabled RF protocols', settingUi.legendFactory),
    new settingUi.ConfigItem('rfProtocols', settingUi.protocolInputField, settingUi.protocolApply, settingUi.protocolGet, ''),

    new settingUi.GroupItem('Log Config', settingUi.legendFactory),
    new settingUi.ConfigItem('serialLogLevel', settingUi.logLevelInputFactory, settingUi.inputApply, settingUi.inputGet, 'Level for serial logging'),
    new settingUi.ConfigItem('webLogLevel', settingUi.logLevelInputFactory, settingUi.inputApply, settingUi.inputGet, 'Level for logging to the web UI'),
    new settingUi.ConfigItem('syslogLevel', settingUi.logLevelInputFactory, settingUi.inputApply, settingUi.inputGet, 'Level for syslog logging'),
    new settingUi.ConfigItem('syslogHost', settingUi.hostNameInputFactory, settingUi.inputApply, settingUi.inputGet, 'Syslog server (optional)'),
    new settingUi.ConfigItem('syslogPort', settingUi.portNumberInputFactory, settingUi.inputApply, settingUi.inputGetInt, 'Syslog port (optional)'),

    new settingUi.GroupItem('Status LED', settingUi.legendFactory),
    new settingUi.ConfigItem('ledPin', settingUi.pinNumberInputFactory, settingUi.inputApply, settingUi.inputGetInt, 'The GPIO pin used for the status LED'),
    new settingUi.ConfigItem('ledActiveHigh', settingUi.checkboxFactory, settingUi.checkboxApply, settingUi.checkboxGet, 'The way how the LED is connected to the pin (false for built-in led)'),
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

  function loadFwVersion() {
    gateway.fetchFirmware().then((data) => {
      $('#current-fw-version').text(data.version);
      $('#chip-id').text(data.chipId);
      const container = $('#fw-build-with');
      container.empty();
      $.each(data.build_with, (dependency, version) => {
        container.append($('<li>', { text: `${dependency}: ${version}` }));
      });
    });
  }

  function openLogListener() {
    function onNewStatus(state) {
      $('#log-status').text(state);
    }

    function onMessage(message) {
      const pre = $('#log-container');
      const element = pre.get(0);
      const isScrollDown = (element.scrollTop === element.scrollHeight - element.clientHeight);
      pre.append(message);
      if (isScrollDown) {
        // scroll down if current bottom is shown
        element.scrollTop = element.scrollHeight - element.clientHeight;
      }
    }

    function onConnect() {
      loadFwVersion();
    }

    return new gateway.LogListener(onMessage, onNewStatus, onConnect);
  }
  // Clear log
  $('#btn-clear-log').click(() => {
    $('#log-container').empty();
  });

  $('#update-form').submit((formEvent) => {
    formEvent.preventDefault();
    $('#update-form').children('input').prop('disabled', true);
    const file = $('#fw-file').get(0).files[0];
    const statusElement = $('#upload-status');
    gateway.uploadFirmware(file, (progressEvent) => {
      const progress = Math.ceil(progressEvent.loaded / progressEvent.total * 100);
      statusElement.text(`Uploading: ${progress} %`);
    }).then((result) => {
      if (result.success) {
        statusElement.text('Update successful. Device will reboot and try to reconnect in 20 seconds.');
        setTimeout(() => {
          window.location.reload(true);
        }, 20000);
      } else {
        statusElement.text('Update failed. More information can be found on the log console. Device will reboot with old firmware. Please reconnect and try to flash again.');
      }
    });
  });

  settingUi.init(CONFIG_ITEMS);
  debugUi.init(DEBUG_FLAGS, $('#debugflags'));
  commandUi.init(SystemCommandActions);
  openLogListener();
});
