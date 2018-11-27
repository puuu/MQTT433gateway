/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2018 Puuu

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

const URLs = {
  config: './config',
  debug: './debug',
  firmware: './firmware',
  protocols: './protocols',
  command: './system',
  log: `ws://${window.location.hostname}:81`,
};

/**
 * Push data to a rest API.
 *
 * @param {string} url - URL of API
 * @param {any} data - Data to push (will be converted to JSON)
 * @param {string} method - 'PUT' (default), 'POST, 'PATCH', ...
 * @return {Promise} A Promise that resolves to a Response object.
 */
function push(url, data, method = 'PUT') {
  return fetch(url, {
    method,
    body: JSON.stringify(data),
    headers: { 'Content-Type': 'application/json' },
  });
}

/**
 * Check response for success and pase the data as JSON.
 *
 * @param {Response} response - Response object from a fetch() call
 * @return {Promise} A Promise that resolves to an Object.
 */
function checkResponse(response) {
  if (!response.ok) {
    throw new Error(
      `Response: (${response.status}) ${response.statusText}`,
    );
  }
  return response.json();
}

/**
 * Fetch configuration of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchConfig() {
  return fetch(URLs.config).then(checkResponse);
}

/**
 * Push configuration to the MQTT433gateway.
 *
 * @param {object} data - Configuration data
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushConfig(data) {
  return push(URLs.config, data).then(checkResponse);
}

/**
 * Fetch object with debug flags and their states of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchDebug() {
  return fetch(URLs.debug).then(checkResponse);
}

/**
 * Push debug flags config to the MQTT433gateway.
 *
 * @param {object} data - Changed debug flags and their states.
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushDebug(data) {
  return push(URLs.debug, data).then(checkResponse);
}

/**
 * Fetch firmware information of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchFirmware() {
  return fetch(URLs.firmware).then(checkResponse);
}

/**
 * Fetch firmware information of the MQTT433gateway.
 *
 * @param {File} file - Firmware binary
 * @param {updloadFirmware~onprogress} onprogress - pogress callback
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function uploadFirmware(file, onprogress) {
  return new Promise((resolve, reject) => {
    const fd = new window.FormData();
    fd.append('file', file);
    const xhr = new window.XMLHttpRequest();
    xhr.open('POST', URLs.firmware);
    xhr.withCredentials = true;
    xhr.upload.onprogress = onprogress;
    xhr.onload = () => {
      const options = {
        status: xhr.status,
        statusText: xhr.statusText,
      };
      options.url = xhr.responseURL;
      const body = 'response' in xhr ? xhr.response : xhr.responseText;
      resolve(new window.Response(body, options));
    };
    xhr.onerror = reject;
    xhr.ontimeout = reject;
    xhr.send(fd);
  }).then(checkResponse);
}

/**
 * Fetch list of available rf protocols of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchProtocols() {
  return fetch(URLs.protocols).then(checkResponse);
}

/**
 * Execute system commands on the MQTT433gateway.
 *
 * @param {array} data - Data including the the system command.
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushCommand(data) {
  return push(URLs.command, data, 'POST').then(checkResponse);
}

/**
 * Log listener class on the MQTT433gateway.
 *
 * The log feature is realized by a WebSocket connection. This class tries to
 * automatically reconnect to the MQTT433gateway if the connection is broken.
 */
class LogListener {
  /**
   * Create and connect LogListerner
   *
   * @param {LogListener~onMessage} onMessage - The callback to handel new
   *                                            log messages
   * @param {LogListener~onNewStatus} onNewStatus - The callback to handel new
   *                                                log listener status
   * @param {LogListener~onConnect} onConnect - The callback to be called on
   *                                            (re-)established connections
   */
  constructor(onMessage, onNewStatus, onConnect) {
    this.webSocket = undefined;
    this.pingTimer = undefined;
    this.cb_onMessage = onMessage;
    this.cb_onNewStatus = onNewStatus;
    this.cb_onConnect = onConnect;
    this.connect();
    this.cb_onNewStatus('Connecting');
  }

  /**
   * Close LogListener connection to the MQTT433gateway
   */
  close() {
    clearTimeout(this.pingTimer);
    this.webSocket.close();
    this.cb_onNewStatus('Closed');
  }

  /**
   * Ping the MQTT433gateway for connection tracking
   */
  ping() {
    clearTimeout(this.pingTimer);
    this.pingTimer = setTimeout(() => {
      this.webSocket.send('__PING__');
      this.pingTimer = setTimeout(() => this.reconnect(), 2000);
    }, 5000);
  }

  /**
   * Connect to the MQTT433gateway
   */
  connect() {
    this.webSocket = new WebSocket(URLs.log);
    this.webSocket.onmessage = (event) => {
      const message = event.data;
      if (message === '__PONG__') {
        this.ping();
        return;
      }
      this.cb_onMessage(message);
    };
    this.webSocket.onopen = () => {
      this.cb_onNewStatus('Connected');
      this.ping();
      this.cb_onConnect();
    };
    this.webSocket.onerror = (error) => {
      this.webSocket.close();
      // eslint-disable-next-line no-console
      console.error('Websocket error: ', error);
      if (this.pingTimer === undefined) {
        this.cb_onNewStatus('Error, reconnecting');
        setTimeout(() => this.connect(), 2000);
      }
    };
  }

  /**
   * Reconnect to the MQTT433gateway
   */
  reconnect() {
    this.webSocket.close();
    this.webSocket.onerror = undefined;
    this.cb_onNewStatus('Broken, reconnecting');
    this.connect();
  }
}
/**
 * This callback receive new new log messages of initLogListener.
 * @callback LogListener~onMessage
 * @param {string} message - New log message
 */
/**
 * This callback indicates a status change of initLogListener.
 * @callback LogListener~onNewStatus
 * @param {string} status - New status
 */
/**
  * This callback is called on every (re-)established connections.
  * @callback LogListener~onConnect
  */

export default {
  fetchConfig,
  pushConfig,
  fetchDebug,
  pushDebug,
  fetchFirmware,
  uploadFirmware,
  fetchProtocols,
  pushCommand,
  LogListener,
};
