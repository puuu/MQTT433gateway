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

#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include <ArduinoSimpleLogging.h>
#include <WiFiManager.h>

#include <ConfigWebServer.h>
#include <MqttClient.h>
#include <RfHandler.h>
#include <Settings.h>
#include <StatusLED.h>
#include <SyslogLogTarget.h>
#include <SystemHeap.h>
#include <SystemLoad.h>

WiFiClient wifi;

Settings settings;
RfHandler *rf = nullptr;
ConfigWebServer *webServer = nullptr;
MqttClient *mqttClient = nullptr;

StatusLED *statusLED = nullptr;

SyslogLogTarget *syslogLog = nullptr;

SystemLoad *systemLoad = nullptr;
SystemHeap *systemHeap = nullptr;

void reconnectMqtt(const Settings &) {
  if (mqttClient != nullptr) {
    delete mqttClient;
    mqttClient = nullptr;
  }

  if (!settings.hasValidPassword()) {
    Logger.warning.println(
        F("No valid config password set - do not connect to MQTT!"));
    return;
  }

  if (settings.mqttBroker.length() <= 0) {
    Logger.warning.println(F("No MQTT broker configured yet"));
    return;
  }

  mqttClient = new MqttClient(settings, wifi);
  mqttClient->begin();
  Logger.info.println(F("MQTT instance created."));

  mqttClient->registerRfDataHandler(
      [](const String &protocol, const String &data) {
        if (rf) rf->transmitCode(protocol, data);
      });
}

void setupRf(const Settings &) {
  if (rf) {
    delete rf;
    rf = nullptr;
  }

  if (!settings.hasValidPassword()) {
    Logger.warning.println(
        F("No valid config password set - do not start RF handler!"));
    return;
  }

  rf = new RfHandler(settings);
  rf->registerReceiveHandler([](const String &protocol, const String &data) {
    if (mqttClient) {
      mqttClient->publishCode(protocol, data);
    }
  });
  rf->begin();
  Logger.info.println(F("RfHandler Instance created."));
}

void setupWebLog() {
  if (webServer) {
    if (settings.webLogLevel.length() > 0) {
      Logger.addHandler(Logger.stringToLevel(settings.webLogLevel),
                        webServer->logTarget());
    } else {
      Logger.removeHandler(webServer->logTarget());
    }
  }
}

void setupWebServer() {
  webServer = new ConfigWebServer(settings);

  webServer->registerSystemCommandHandler(F("restart"), []() {
    delay(100);
    ESP.restart();
  });
  webServer->registerSystemCommandHandler(F("reset_wifi"), []() {
    WiFi.disconnect(true);
    delay(100);
    ESP.restart();
  });
  webServer->registerSystemCommandHandler(F("reset_config"), []() {
    settings.reset();
    delay(100);
    ESP.restart();
  });

  webServer->registerProtocolProvider([]() {
    if (rf) {
      return rf->availableProtocols();
    }
    return String(F("[]"));
  });

  webServer->registerOtaHook([]() {
    if (statusLED) statusLED->setState(StatusLED::ota);
    if (rf) {
      delete rf;
      rf = nullptr;
    }
    WiFiUDP::stopAll();
  });

  webServer->registerDebugFlagHandler(
      F("protocolRaw"), []() { return rf && rf->isRawModeEnabled(); },
      [](bool state) {
        if (rf) rf->setRawMode(state);
      });

  webServer->registerDebugFlagHandler(
      F("systemLoad"), []() { return systemLoad != nullptr; },
      [](bool state) {
        if (state) {
          systemLoad = new SystemLoad(Logger.debug);
        } else if (systemLoad) {
          delete systemLoad;
          systemLoad = nullptr;
        }
      });

  webServer->registerDebugFlagHandler(
      F("freeHeap"), []() { return systemHeap != nullptr; },
      [](bool state) {
        if (state) {
          systemHeap = new SystemHeap(Logger.debug);
        } else if (systemHeap) {
          delete systemHeap;
          systemHeap = nullptr;
        }
      });

  webServer->begin();

  setupWebLog();
}

void setupMdns() {
  if (0 == settings.deviceName.length()) {
    return;
  }
  if (!MDNS.begin(settings.deviceName.c_str())) {
    Logger.error.println(F("Error setting up MDNS responder"));
  }

  MDNS.addService("http", "tcp", 80);
}

void setupStatusLED(const Settings &s) {
  delete statusLED;
  statusLED = new StatusLED(s.ledPin, s.ledActiveHigh);
  Logger.debug.print("Change status LED config: pin=");
  Logger.debug.print(s.ledPin);
  Logger.debug.print(" activeHigh=");
  Logger.debug.println(s.ledActiveHigh);
}

void setupWifi() {
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback([](WiFiManager *) {
    Logger.info.println(F("Start wifimanager config portal."));
    if (statusLED) statusLED->setState(StatusLED::wifimanager);
  });

  // Restart after we had the portal running
  wifiManager.setSaveConfigCallback([]() {
    Logger.info.println(F("Wifi config changed. Restart."));
    delay(100);
    ESP.restart();
  });

  if (!wifiManager.autoConnect(settings.deviceName.c_str(),
                               settings.configPassword.c_str())) {
    Logger.warning.println(F("Try connecting again after reboot"));
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Logger.addHandler(Logger.DEBUG, Serial);
  if (!SPIFFS.begin()) {
    Logger.error.println(F("Initializing of SPIFFS failed!"));
  }

  settings.registerChangeHandler(STATUSLED, setupStatusLED);

  settings.registerChangeHandler(MQTT, reconnectMqtt);

  settings.registerChangeHandler(RF_ECHO, [](const Settings &s) {
    if (rf) rf->setEchoEnabled(s.rfEchoMessages);
  });

  settings.registerChangeHandler(RF_PROTOCOL, [](const Settings &s) {
    if (rf) rf->filterProtocols(s.rfProtocols);
  });

  settings.registerChangeHandler(WEB_CONFIG, [](const Settings &s) {
    if (!webServer) {
      setupWebServer();
    }
  });

  settings.registerChangeHandler(SYSLOG, [](const Settings &s) {
    if (syslogLog) {
      Logger.removeHandler(*syslogLog);
      delete syslogLog;
    }
    if (s.syslogLevel.length() > 0 && s.syslogHost.length() > 0 &&
        s.syslogPort != 0) {
      syslogLog = new SyslogLogTarget();
      syslogLog->begin(s.deviceName, s.syslogHost, s.syslogPort);
      Logger.addHandler(Logger.stringToLevel(s.syslogLevel), *syslogLog);
    }
  });

  settings.registerChangeHandler(RF_CONFIG, setupRf);

  settings.registerChangeHandler(LOGGING, [](const Settings &s) {
    if (s.serialLogLevel.length() > 0) {
      Logger.addHandler(Logger.stringToLevel(settings.serialLogLevel), Serial);
    } else {
      Logger.removeHandler(Serial);
    }
    setupWebLog();
  });

  Logger.info.println(F("Load Settings..."));
  settings.load();

  setupStatusLED(settings);
  if (statusLED) statusLED->setState(StatusLED::wifiConnect);

  setupWifi();

  // Notify all setting listeners
  settings.notifyAll();
  if (statusLED) statusLED->setState(StatusLED::startup);

  Logger.debug.println(F("Current configuration:"));
  settings.serialize(Logger.debug, true, false);

  setupMdns();

  Logger.info.print(F("\nListen on IP: "));
  Logger.info.println(WiFi.localIP());
}

void loop() {
  if (rf && mqttClient) {
    if (statusLED) statusLED->setState(StatusLED::normalOperation);
  } else {
    if (statusLED) statusLED->setState(StatusLED::requireConfiguration);
  }
  if (statusLED) {
    statusLED->loop();
  }

  webServer->loop();

  if (mqttClient) {
    mqttClient->loop();
  }

  if (rf) {
    rf->loop();
  }

  if (systemLoad) {
    systemLoad->loop();
  }
  if (systemHeap) {
    systemHeap->loop();
  }
}
