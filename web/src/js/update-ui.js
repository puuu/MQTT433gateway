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

const $status = $('#upload-status');

function loadVersion() {
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

function init() {
  $('#update-form').submit((formEvent) => {
    formEvent.preventDefault();
    const file = $('#fw-file').get(0).files[0];
    if (!file) {
      $status.text('Please select a file to upload!');
      return;
    }
    $('#update-form').children('input').prop('disabled', true);
    gateway.uploadFirmware(file, (progressEvent) => {
      const progress = Math.ceil(progressEvent.loaded / progressEvent.total * 100);
      $status.text(`Uploading: ${progress} %`);
    })
      .then((result) => {
        if (result.success) {
          $status.text('Update successful. Device will reboot and try to reconnect in 20 seconds.');
          setTimeout(() => {
            window.location.reload(true);
          }, 20000);
        } else {
          $status.text('Update failed. More information can be found on the log console. Device will reboot with old firmware. Please reconnect and try to flash again.');
        }
      })
      .catch((error) => {
        // eslint-disable-next-line no-console
        console.error('fetchConfig: ', error);
        $status.text('Firmware upload failed! Please try again.');
        $('#update-form').children('input').prop('disabled', false);
      });
  });
}

export default {
  loadVersion,
  init,
};
