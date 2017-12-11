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

#include <ConfigWebServer.h>
#include <Heartbeat.h>
#include <MqttClient.h>
#include <RfHandler.h>
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

const int HEARTBEAD_LED_PIN = 0;

WiFiClient wifi;

Settings settings(myMQTT_BROCKER, myMQTT_USERNAME, myMQTT_PASSWORD);
RfHandler *rf = nullptr;
ConfigWebServer *webServer;
MqttClient *mqttClient = nullptr;

Heartbeat beatLED(HEARTBEAD_LED_PIN);

SHAauth *otaAuth = nullptr;

bool logMode = false;
bool rawMode = false;

void handleOta(const String &topic, const String &payload) {
  if (topic == F("url")) {
    settings.updateOtaUrl(payload);
    mqttClient->publishOta(F("nonce"), otaAuth->nonce());
    return;
  }

  if ((topic == F("passwd")) && (settings.otaUrl.length() > 7)) {
    if (otaAuth->verify(payload)) {
      beatLED.on();
      Debug(F("Start OTA update from: "));
      DebugLn(settings.otaUrl);
      rf->disableReceiver();
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
      rf->enableReceiver();
    } else {
      DebugLn(F("OTA authentication failed!"));
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

  mqttClient->registerSetHandler(F("log"), [](const String &payload) {
    if (rf) rf->setLogMode(payload[0] == '1');
  });
  mqttClient->registerSetHandler(F("raw"), [](const String &payload) {
    if (rf) rf->setRawMode(payload[0] == '1');
  });
  mqttClient->registerSetHandler(F("protocols"), [](const String &payload) {
    settings.updateProtocols(payload);
  });

  mqttClient->registerRfDataHandler(
      [](const String &protocol, const String &data) {
        if (rf) rf->transmitCode(protocol, data);
      });
  mqttClient->registerOtaHandler(handleOta);
}

void setupRf(const Settings &) {
  // Init only on first load. We need to reboot else.
  if (rf) {
    return;
  }
  rf =
      new RfHandler(settings,
                    [](const String &protocol, const String &data) {
                      if (mqttClient) {
                        mqttClient->publishCode(protocol, data);
                      }
                    },
                    [](int statuc, const String &protocol, const String &data) {
                      if (mqttClient) {
                        mqttClient->publishLog(statuc, protocol, data);
                      }
                    },
                    [](const String &data) {
                      if (mqttClient) {
                        mqttClient->publishRaw(data);
                      }
                    });
  rf->begin();
}

void setupWebServer(const Settings &s) {
  webServer = new ConfigWebServer();

  webServer->registerSystemCommandHandler(F("restart"), []() {
    delay(100);
    ESP.restart();
  });
  webServer->registerSystemCommandHandler(F("reset_wifi"), resetWifiConfig);
  webServer->registerSystemCommandHandler(F("reset_config"),
                                          []() { settings.reset(); });

  webServer->begin(s);
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

  if (!connectWifi(mySSID, myWIFIPASSWD, []() { beatLED.loop(); })) {
    DebugLn(F("Try connectiing again after reboot"));
    ESP.restart();
  }

  settings.registerChangeHandler(MQTT, reconnectMqtt);

  settings.registerChangeHandler(RF_ECHO, [](const Settings &s) {
    if (rf) rf->setEchoEnabled(s.rfEchoMessages);
  });

  settings.registerChangeHandler(RF_PROTOCOL, [](const Settings &s) {
    if (rf) rf->filterProtocols(s.rfProtocols);
  });

  settings.registerChangeHandler(OTA, [](const Settings &s) {
    if (otaAuth) {
      delete (otaAuth);
    }
    otaAuth = new SHAauth(s.otaPassword);
  });

  settings.registerChangeHandler(WEB_CONFIG, [](const Settings &s) {
    if (!webServer) {
      setupWebServer(s);
    } else {
      webServer->updateSettings(s);
    }
  });

  settings.registerChangeHandler(RF_CONFIG, setupRf);

  DebugLn(F("Load Settings..."));
  settings.load();

#ifdef DEBUG
  DebugLn(F("Current configuration:"));
  settings.serialize(Serial, true, false);
#endif

  DebugLn("\n");
  Debug(F("Name: "));
  DebugLn(String(ESP.getChipId(), HEX));
}

void loop() {
  webServer->handleClient();

  mqttClient->loop();

  rf->loop();
  beatLED.loop();
}
