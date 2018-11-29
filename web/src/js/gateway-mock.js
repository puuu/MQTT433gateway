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

/* eslint-disable no-console */

const mockData = {
  config: {
    deviceName: 'default',
    mqttBroker: 'broker',
    mqttBrokerPort: 12,
    mqttUser: '',
    mqttRetain: true,
    mqttReceiveTopic: 'recv',
    mqttSendTopic: 'send',
    mqttStateTopic: 'state',
    mqttVersionTopic: 'version',
    rfEchoMessages: false,
    rfReceiverPin: 1,
    rfReceiverPinPullUp: false,
    rfTransmitterPin: 2,
    rfProtocols: [],
    serialLogLevel: 'info',
    webLogLevel: 'error',
    syslogLevel: '',
    syslogHost: '',
    syslogPort: 514,
    ledPin: 1,
    ledActiveHigh: false,
  },
  debug: {
    freeHeap: false,
    systemLoad: false,
    protocolRaw: false,
  },
  firmware: {
    version: 'v0.0.8',
    chipId: 'devMock',
    build_with: {
      ArduinoJson: '5.13.3',
      ArduinoSimpleLogging: '0.2.2',
      ESPiLight: '0.15.0',
      PlatformIO: '3.6.2b6',
      PubSubClient: '2.7',
      Syslog: '2.0.0',
      WebSockets: '2.1.2',
      WifiManager: '0.14',
      espressif8266: '1.8.0',
    },
  },
  protocols: ['x10', 'tfa30', 'tfa', 'teknihall', 'techlico_switch', 'tcm',
    'silvercrest', 'selectremote', 'secudo_smoke_sensor', 'sc2262', 'rsl366',
    'rev3_switch', 'rev2_switch', 'rev1_switch', 'rc101', 'quigg_screen',
    'quigg_gt9000', 'quigg_gt7000', 'quigg_gt1000', 'pollin',
    'ninjablocks_weather', 'mumbi', 'logilink_switch', 'livolo_switch',
    'impuls', 'heitech', 'ev1527', 'eurodomest_switch', 'elro_800_switch',
    'elro_800_contact', 'elro_400_switch', 'elro_300_switch', 'ehome', 'daycom',
    'conrad_rsl_switch', 'conrad_rsl_contact', 'cleverwatts', 'clarus_switch',
    'beamish_switch', 'auriol', 'arctech_switch_old', 'arctech_switch',
    'arctech_screen_old', 'arctech_screen', 'arctech_motion', 'arctech_dusk',
    'arctech_dimmer', 'arctech_contact', 'alecto_wx500', 'alecto_wsd17',
    'alecto_ws1700'],
};

let mockLogListerner;
/**
 * Write log message to LogListerner mock
 * @param {string} msg - message without new line
 */
function mockWriteToLog(msg) {
  if (mockLogListerner !== undefined) {
    mockLogListerner.cb_onMessage(`${msg}\n`);
  }
}

/**
 * Simulate a broken connection of LogListerner
 * @param {number} time - time until reconnect in ms
 */
function mockSimConnectionBroken(time) {
  if (mockLogListerner !== undefined) {
    mockLogListerner.cb_onNewStatus('Connection broken!');
    setTimeout(() => {
      mockLogListerner.cb_onNewStatus('Connected to Mock');
      mockLogListerner.cb_onConnect();
    }, time);
  }
}

const wait = ms => new Promise(resolve => setTimeout(resolve, ms));

/**
 * Fetch configuration of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchConfig() {
  return wait(500).then(() => mockData.config);
}

/**
 * Push configuration to the MQTT433gateway.
 *
 * @param {object} data - Configuration data
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushConfig(data) {
  console.log('pushConfig: ', data);
  mockData.config = Object.assign(mockData.config, data);
  mockWriteToLog(`Got new config patch: ${JSON.stringify(data)}`);
  mockWriteToLog(`Config: ${JSON.stringify(mockData.config)}`);
  return fetchConfig();
}

/**
 * Fetch object with debug flags and their states of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchDebug() {
  return wait(500).then(() => mockData.debug);
}

let freeHeapInterval;
let systemLoadInterval;
/**
 * Push debug flags config to the MQTT433gateway.
 *
 * @param {object} data - Changed debug flags and their states.
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushDebug(data) {
  console.log('pushDebug: ', data);
  mockWriteToLog(`Got new debug data: ${JSON.stringify(data)}`);
  mockData.debug = Object.assign(mockData.debug, data);
  if (mockData.debug.freeHeap) {
    if (freeHeapInterval === undefined) {
      freeHeapInterval = setInterval(() => {
        mockWriteToLog('Free heap: 0815');
      }, 1000);
    }
  } else if (freeHeapInterval !== undefined) {
    freeHeapInterval = clearInterval(freeHeapInterval);
  }
  if (mockData.debug.systemLoad) {
    if (systemLoadInterval === undefined) {
      systemLoadInterval = setInterval(() => {
        mockWriteToLog(
          `Loop iterations since last interval: ${
            Math.floor(Math.random() * 50000)}`,
        );
      }, 1000);
    }
  } else if (systemLoadInterval !== undefined) {
    systemLoadInterval = clearInterval(systemLoadInterval);
  }
  return fetchDebug();
}

/**
 * Fetch firmware information of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchFirmware() {
  return wait(500).then(() => mockData.firmware);
}

/**
 * Fetch firmware information of the MQTT433gateway.
 *
 * @param {File} file - Firmware binary
 * @param {updloadFirmware~onprogress} onprogress - pogress callback
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function uploadFirmware(file, onprogress) {
  console.log('uploadFirmware', file);
  mockWriteToLog('Firmware update stared.');
  let p = Promise.resolve();
  for (let i = 0; i < 100; i += 1) {
    const event = {
      lengthComputable: true,
      loaded: Math.ceil(file.size * i / 100),
      total: file.size,
    };
    p = p.then(() => wait(50).then(() => onprogress(event)));
  }
  p = p.then(() => wait(200));
  return p.then(() => {
    mockWriteToLog('Firmware update finish. Rebooting!');
    return { success: true };
  });
}

/**
 * Fetch list of available rf protocols of the MQTT433gateway.
 *
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function fetchProtocols() {
  return wait(400).then(() => mockData.protocols);
}

/**
 * Execute system commands on the MQTT433gateway.
 *
 * @param {array} data - Data including the the system command.
 * @return {Promise} A Promise that resolves to a parsed JSON object.
 */
function pushCommand(data) {
  console.log('pushCommand: ', data);
  mockWriteToLog(`Execute command: ${JSON.stringify(data)}`);
  return wait(300);
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
   * @param {LogListener~onNewStatus} onNewStatus - The callbackto handel new
   *                                                log listener status
   * @param {LogListener~onConnect} onConnect - The callback to be called on
   *                                            (re-)established connections
   */
  constructor(onMessage, onNewStatus, onConnect) {
    this.cb_onMessage = onMessage;
    this.cb_onNewStatus = onNewStatus;
    this.cb_onConnect = onConnect;
    mockLogListerner = this;
    setTimeout(() => {
      this.cb_onNewStatus('Connected to Mock');
      this.cb_onConnect();
    }, 500);
    this.cb_onMessage('This is only a Mock!\n');
  }

  /**
   * Close LogListener connection to the MQTT433gateway
   */
  static close() {}
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

const gateway = {
  fetchConfig,
  pushConfig,
  fetchDebug,
  pushDebug,
  fetchFirmware,
  uploadFirmware,
  fetchProtocols,
  pushCommand,
  LogListener,

  mockData,
  mockWriteToLog,
  mockSimConnectionBroken,
};

export default gateway;

window.gateway = gateway;
