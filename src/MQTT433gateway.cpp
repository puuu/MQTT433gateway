/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2016 Puuu

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

#include "config.h"

#include <ESP8266httpUpdate.h>

#include <ESPiLight.h>

#include <Heartbeat.h>
#include <MqttClient.h>
#include <SHAauth.h>
#include <Settings.h>
#include <WifiConnection.h>
#include <debug_helper.h>

#ifndef myMQTT_USERNAME
#define myMQTT_USERNAME nullptr
#define myMQTT_PASSWORD nullptr
#endif

#ifndef mySSID
#define mySSID nullptr
#define myWIFIPASSWD nullptr
#endif

const int RECEIVER_PIN = 12;  // avoid 0, 2, 15, 16
const int TRANSMITTER_PIN = 4;
const int HEARTBEAD_LED_PIN = 0;

WiFiClient wifi;

Settings settings(myMQTT_BROCKER, myMQTT_USERNAME, myMQTT_PASSWORD);
MqttClient *mqttClient = nullptr;

Heartbeat beatLED(HEARTBEAD_LED_PIN);
ESPiLight rf(TRANSMITTER_PIN);

SHAauth *otaAuth = nullptr;

bool logMode = false;
bool rawMode = false;

void transmitt(const String &protocol, const String &message) {
  Debug(F("rf send "));
  Debug(message);
  Debug(F(" with protocol "));
  DebugLn(protocol);

  if (protocol == F("RAW")) {
    uint16_t rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen =
        rf.stringToPulseTrain(message, rawpulses, MAXPULSESTREAMLENGTH);
    if (rawlen > 0) {
      rf.sendPulseTrain(rawpulses, rawlen);
    }
  } else {
    rf.send(protocol, message);
  }
}

void handleOta(const String &topic, const String &payload) {
  if (topic == F("url")) {
    settings.updateOtaUrl(payload);
    mqttClient->sendOta(F("nonce"), otaAuth->nonce());
    return;
  }

  if ((topic == F("passwd")) && (settings.otaUrl.length() > 7)) {
    if (otaAuth->verify(payload)) {
      beatLED.on();
      Debug(F("Start OTA update from: "));
      DebugLn(settings.otaUrl);
      rf.disableReceiver();
      t_httpUpdate_return ret = ESPhttpUpdate.update(settings.otaUrl);
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Debug(F("HTTP_UPDATE_FAILD Error ("));
          Debug(ESPhttpUpdate.getLastError());
          Debug(F("): "));
          DebugLn(ESPhttpUpdate.getLastErrorString());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          DebugLn(F("HTTP_UPDATE_NO_UPDATES"));
          break;
        case HTTP_UPDATE_OK:
          DebugLn(F("HTTP_UPDATE_OK"));  // may not called ESPhttpUpdate
          // reboot the ESP?
          ESP.restart();
          break;
      }
      rf.enableReceiver();
    } else {
      DebugLn(F("OTA authentication failed!"));
    }
  }
}

void rfCallback(const String &protocol, const String &message, int status,
                int repeats, const String &deviceID) {
  Debug(F("RF signal arrived ["));
  Debug(protocol);
  Debug(F("]/["));
  Debug(deviceID);
  Debug(F("] ("));
  Debug(status);
  Debug(F(") "));
  Debug(message);
  Debug(F(" ("));
  Debug(repeats);
  DebugLn(F(")"));

  if (!mqttClient) {
    return;
  }

  if (status == VALID) {
    if (deviceID != nullptr) {
      mqttClient->sendCode(protocol + "/" + deviceID, message);
    }
    mqttClient->sendCode(protocol, message);
  }
  if (logMode) {
    mqttClient->sendLog(status, protocol, message);
  }
}

void rfRawCallback(const uint16_t *pulses, int length) {
  if (rawMode) {
    String data = rf.pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Debug(F("RAW RF signal ("));
      Debug(length);
      Debug(F("): "));
      Debug(data);
      DebugLn();
      if (mqttClient) {
        mqttClient->sendRaw(data);
      }
    }
  }
}

void reconnectMqtt(const Settings &) {
  if (mqttClient != nullptr) {
    delete mqttClient;
    mqttClient = nullptr;
  }

  mqttClient = new MqttClient(settings, wifi);
  mqttClient->begin();

  mqttClient->onSet(F("log"),
                    [](const String &payload) { logMode = payload[0] == '1'; });
  mqttClient->onSet(F("raw"),
                    [](const String &payload) { rawMode = payload[0] == '1'; });
  mqttClient->onSet(F("protocols"), [](const String &payload) {
    settings.updateProtocols(payload);
  });

  mqttClient->onRfData(transmitt);
  mqttClient->onOta(handleOta);
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  if (!connectWifi(mySSID, myWIFIPASSWD, []() { beatLED.loop(); })) {
    DebugLn(F("Try connectiing again after reboot"));
    ESP.restart();
  }

  settings.onChange(MQTT, reconnectMqtt);
  settings.onChange(
      RF_ECHO, [](const Settings &s) { rf.setEchoEnabled(s.rfEchoMessages); });
  settings.onChange(
      RF_PROTOCOL, [](const Settings &s) { rf.limitProtocols(s.rfProtocols); });
  settings.onChange(OTA, [](const Settings &s) {
    if (otaAuth) {
      delete (otaAuth);
    }
    otaAuth = new SHAauth(s.otaPassword);
  });

  settings.load();

  pinMode(RECEIVER_PIN,
          INPUT_PULLUP);  // 5V protection with reverse diode needs pullup
  rf.setCallback(rfCallback);
  rf.setPulseTrainCallBack(rfRawCallback);
  rf.initReceiver(RECEIVER_PIN);
  DebugLn();
  Debug(F("Name: "));
  DebugLn(String(ESP.getChipId(), HEX));
}

void loop() {
  mqttClient->loop();

  rf.loop();
  beatLED.loop();
}
