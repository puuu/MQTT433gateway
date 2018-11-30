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

const $status = $('#debugflags-status');
let $fieldset;

function apply(data) {
  $.each(data, (key, value) => {
    $(`#debug-${key}`).prop('checked', value);
    $status.text('');
  });
}

function submit(item) {
  const data = {};
  data[item.name] = item.checked;
  $status.text('Submitting debug flags.');
  $fieldset.prop('disabled', true);
  gateway.pushDebug(data)
    .then(apply)
    .catch((error) => {
      // eslint-disable-next-line no-console
      console.error('pushDebug: ', error);
      $status.text('Submitting of debug flags failed!');
    })
    .then(() => {
      $fieldset.prop('disabled', false);
    });
}

function create(debugFlag, helpText) {
  const checkbox = $('<input>', {
    type: 'checkbox',
    class: 'debug-item',
    id: `debug-${debugFlag}`,
    name: debugFlag,
  });
  checkbox.change(function onDebugClick() {
    submit(this);
  });
  return $('<div>', {
    class: 'pure-u-1 pure-u-md-1-3',
  }).append($('<label>', { class: 'pure-checkbox' }).append([
    checkbox,
    ` ${debugFlag}`,
    $('<span>', {
      class: 'pure-form-message',
      text: helpText,
    }),
  ]));
}

function load() {
  gateway.fetchDebug().then(apply);
}

function init(debugFlags, $container) {
  $fieldset = $container.parent('fieldset');
  $.each(debugFlags, (debugFlag, helpText) => {
    $container.append(create(debugFlag, helpText));
  });
}

export default {
  load,
  init,
};
