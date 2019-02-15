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
/* global $ */

let lastConfig = {};
let changes = {};

function registerConfigUi(element, item) {
  element.change(() => {
    const newData = item.fetch(element);
    if (newData !== undefined) {
      if (JSON.stringify(lastConfig[item.name]) !== JSON.stringify(newData)) {
        changes[item.name] = newData;
      } else {
        delete changes[item.name];
      }
    }
  });
}

function inputLabelFactory(item) {
  return $('<label>', {
    text: item.name,
    for: `cfg-${item.name}`,
  });
}

function inputHelpFactory(item) {
  return $('<span>', {
    class: 'pure-form-message',
    text: item.help,
  });
}

function logLevelInputFactory(item) {
  const $element = $('<select>', {
    class: 'config-item',
    id: `cfg-${item.name}`,
    name: item.name,
  }).append([
    $('<option>', { value: '', text: 'None' }),
    $('<option>', { value: 'error', text: 'Error' }),
    $('<option>', { value: 'warning', text: 'Warning' }),
    $('<option>', { value: 'info', text: 'Info' }),
    $('<option>', { value: 'debug', text: 'Debug' }),
  ]);
  registerConfigUi($element, item);
  return [
    inputLabelFactory(item),
    $element,
    inputHelpFactory(item),
  ];
}

function inputFieldFactory(item, pattern, required) {
  const $element = $('<input>', {
    type: 'text',
    class: 'pure-input-1 config-item',
    id: `cfg-${item.name}`,
    pattern,
    required,
    name: item.name,
  });
  registerConfigUi($element, item);
  return [
    inputLabelFactory(item),
    $element,
    inputHelpFactory(item),
  ];
}

function deviceNameInputFactory(item) {
  return inputFieldFactory(item, '[.-_A-Za-z0-9]+', true);
}

function hostNameInputFactory(item) {
  return inputFieldFactory(item, '[.-_A-Za-z0-9]*');
}

function mqttTopicInputFactory(item) {
  return inputFieldFactory(item, undefined, true);
}

function inputFieldNumberFactory(item, min, max) {
  const $element = $('<input>', {
    type: 'number',
    class: 'pure-input-1 config-item',
    id: `cfg-${item.name}`,
    name: item.name,
    min,
    max,
  });
  registerConfigUi($element, item);
  return [
    inputLabelFactory(item),
    $element,
    inputHelpFactory(item),
  ];
}

function portNumberInputFactory(item) {
  return inputFieldNumberFactory(item, 1, 65535);
}

function pinNumberInputFactory(item) {
  return inputFieldNumberFactory(item, 0, 16);
}

function passwordFieldFactory(item, minlength) {
  const $element = $('<input>', {
    type: 'password',
    class: 'pure-input-1 config-item',
    id: `cfg-${item.name}`,
    name: item.name,
    minlength,
  });
  registerConfigUi($element, item);
  return [
    inputLabelFactory(item),
    $element,
    inputHelpFactory(item),
  ];
}

function devicePasswordInputFactory(item) {
  const properties = {
    type: 'password',
    class: 'pure-input-1 config-item',
    minlength: 8,
  };
  const $element1 = $('<input>', $.extend(properties, {
    id: `cfg-${item.name}`,
    name: item.name,
  }));
  const $element2 = $('<input>', $.extend(properties, {
    id: `cfg-${item.name}-confirm`,
    name: `${item.name}-confirm`,
  }));
  function validatePassword() {
    let message = '';
    if ($element1.val() !== $element2.val()) {
      message = "Passwords don't match!";
    }
    $element1.get(0).setCustomValidity(message);
    $element2.get(0).setCustomValidity(message);
  }
  registerConfigUi($element1, item);
  registerConfigUi($element2, item);
  $element1.on('input', validatePassword);
  $element2.on('input', validatePassword);
  return [
    inputLabelFactory(item),
    $element1,
    inputLabelFactory({ name: `${item.name} (confirm)` }),
    $element2,
    inputHelpFactory(item),
  ];
}

function checkboxFactory(item) {
  const $element = $('<input>', {
    type: 'checkbox',
    class: 'config-item',
    id: `cfg-${item.name}`,
    name: item.name,
  });
  registerConfigUi($element, item);
  return $('<label>', {
    class: 'pure-checkbox',
  }).append([
    $element,
    ` ${item.name}`,
    inputHelpFactory(item),
  ]);
}

function legendFactory(item) {
  return $('<fieldset>', { class: 'config-group' }).append(
    $('<legend>', { text: item.name }),
  );
}

let protocols;
function protocolInputField(item) {
  const $container = $('<div>', {
    id: `cfg-${item.name}`,
    class: 'pure-g',
  });
  registerConfigUi($container, item);
  function protocolListFactory(protos) {
    protos.forEach((value) => {
      const $element = $('<input>', {
        type: 'checkbox',
        class: 'config-item protocols-item',
        id: `cfg-${item.name}-${value}`,
        name: item.name,
        value,
      });
      $container.append($('<div>', {
        class: 'pure-u-1 pure-u-md-1-2 pure-u-lg-1-3 pure-u-xl-1-4',
      }).append($('<label>', {
        class: 'pure-checkbox',
      }).append([
        $element,
        ` Protocol ${value}`,
      ])));
    });
    protocols = protos;
  }
  gateway.fetchProtocols().then(protocolListFactory);
  return $container;
}

function inputApply(itemName, data) {
  $(`#cfg-${itemName}`).val(data);
}

function devicePasswordApply(itemName, data) {
  $(`#cfg-${itemName}`).val(data);
  $(`#cfg-${itemName}-confirm`).val(data);
}

function checkboxApply(itemName, data) {
  $(`#cfg-${itemName}`).prop('checked', data);
}

function protocolApply(itemName, data) {
  if (protocols === undefined) {
    setTimeout(() => protocolApply(itemName, data), 100);
    return;
  }
  let protocolList = data;
  if (protocolList.length === 0) {
    protocolList = protocols;
  }
  protocolList.forEach((value) => {
    $(`#cfg-${itemName}-${value}`).prop('checked', true);
  });
}

function inputGet(element) {
  if (!element.get(0).checkValidity()) {
    return undefined;
  }
  return element.val();
}

function inputGetInt(element) {
  return parseInt(inputGet(element), 10);
}

function checkboxGet(element) {
  return element.prop('checked');
}

function protocolGet() {
  const $checked = $('.protocols-item:checked');
  if ($('.protocols-item').length === $checked.length) {
    return [];
  }
  return $.map($checked, x => $(x).val());
}

function ConfigItem(name, type, help) {
  this.name = name;
  this.factory = type.factory;
  this.apply = type.apply || inputApply;
  this.fetch = type.fetch || inputGet;
  this.help = help;
}

function GroupItem(name) {
  this.name = name;
  this.container = true;
  this.factory = legendFactory;
}

const Type = {
  deviceName: { factory: deviceNameInputFactory },
  devicePassword: { factory: devicePasswordInputFactory, apply: devicePasswordApply },
  hostName: { factory: hostNameInputFactory },
  portNumber: { factory: portNumberInputFactory, fetch: inputGetInt },
  text: { factory: inputFieldFactory },
  password: { factory: passwordFieldFactory },
  checkbox: { factory: checkboxFactory, apply: checkboxApply, fetch: checkboxGet },
  mqttTopic: { factory: mqttTopicInputFactory },
  pinNumber: { factory: pinNumberInputFactory, fetch: inputGetInt },
  protocol: { factory: protocolInputField, apply: protocolApply, fetch: protocolGet },
  logLevel: { factory: logLevelInputFactory },
};

function init(configItems) {
  const $settings = $('#settings');

  function applyConfig(data) {
    configItems.forEach((item) => {
      if (item.apply) {
        item.apply(item.name, data[item.name]);
      }
    });
    changes = {};
    lastConfig = data;
  }

  function loadConfig() {
    gateway.fetchConfig().then(applyConfig);
  }

  let $container;
  configItems.forEach((item) => {
    const $result = item.factory(item);
    if (item.container) {
      $result.appendTo($settings);
      $container = $result;
    } else {
      $container.append($result);
    }
  });
  loadConfig();
  $('#settings-form').submit((event) => {
    event.preventDefault();
    let onSuccess = applyConfig;
    if ('configPassword' in changes) {
      // reload after new password to force password question
      onSuccess = loadConfig;
    }
    if (('deviceName' in changes)
              && (window.location.hostname.toLowerCase() === `${lastConfig.deviceName.toLowerCase()}.local`)) {
      const onSuccessOld = onSuccess;
      onSuccess = (data) => {
        // eslint-disable-next-line no-alert
        if (window.confirm('deviceName was changed. Did you like to reload with new deviceName?')) {
          const mdnsname = `${changes.deviceName}.local`;
          const url = `${window.location.protocol}//${mdnsname}`;
          window.location.assign(url);
          const $body = $('body');
          $body.empty();
          $body.append(`<a href="${url}">${mdnsname}</a>`);
          return undefined;
        }
        return onSuccessOld(data);
      };
    }
    gateway.pushConfig(changes).then(onSuccess);
    return false;
  });
  $('#cfg-form-reset').click((event) => {
    event.preventDefault();
    loadConfig();
    return false;
  });
}

export default {
  GroupItem,
  ConfigItem,
  Type,

  init,
};
