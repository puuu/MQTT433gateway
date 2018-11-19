/**
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2017 Jan Losinski

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

#include <ArduinoSimpleLogging.h>

#include <Version.h>

#include "MqttClient.h"

class PayloadString : public String {
 public:
  PayloadString(const uint8_t *data, unsigned int length) : String() {
    if (reserve(length)) {
      memcpy(buffer, data, length);
      len = length;
    }
  }
};

MqttClient::MqttClient(const Settings &settings, WiFiClient &client)
    : settings(settings), mqttClient(client), lastConnectAttempt(0) {}

MqttClient::~MqttClient() { mqttClient.disconnect(); }

void MqttClient::begin() {
  using namespace std::placeholders;
  mqttClient.setServer(settings.mqttBroker.c_str(), settings.mqttBrokerPort);

  mqttClient.setCallback(std::bind(&MqttClient::onMessage, this, _1, _2, _3));

  reconnect();
}

static String stateMessage(const bool online) {
  return String(F("{\"chipId\":\"")) + chipId() +
         String(F("\",\"firmware\":\"")) + fwVersion() + F("\",\"state\":\"") +
         String(online ? F("online") : F("offline")) + String(F("\"}"));
}

bool MqttClient::connect() {
  if (0 == settings.mqttUser.length()) {
    return mqttClient.connect(settings.deviceName.c_str(),
                              settings.mqttStateTopic.c_str(), 0, true,
                              stateMessage(false).c_str());
  } else {
    return mqttClient.connect(
        settings.deviceName.c_str(), settings.mqttUser.c_str(),
        settings.mqttPassword.c_str(), settings.mqttStateTopic.c_str(), 0, true,
        stateMessage(false).c_str());
  }
}

void MqttClient::reconnect() {
  if (lastConnectAttempt > 0 &&
      (millis() - lastConnectAttempt) < MQTT_CONNECTION_ATTEMPT_DELAY) {
    return;
  }

  if (!mqttClient.connected()) {
    Logger.debug.println(F("Try to (re)connect to MQTT broker"));
    if (connect()) {
      Logger.info.println(F("MQTT connected."));
      if (subsrcibe()) {
        mqttClient.publish(settings.mqttStateTopic.c_str(),
                           stateMessage(true).c_str(), true);
        Logger.info.println(F("MQTT subscribed."));
      } else {
        Logger.error.println(F("MQTT subsrcibe failed!"));
      }
    } else {
      Logger.error.println(F("MQTT connect failed!"));
    }
  }

  lastConnectAttempt = millis();
}

bool MqttClient::subsrcibe() {
  String topic = settings.mqttSendTopic + "+";

  Logger.debug.print(F("MQTT subscribe to topic: "));
  Logger.debug.println(topic);

  return mqttClient.subscribe(topic.c_str());
}

void MqttClient::loop() {
  reconnect();
  mqttClient.loop();
}

void MqttClient::onMessage(char *topic, uint8_t *payload, unsigned int length) {
  PayloadString strPayload(payload, length);
  String strTopic(topic);

  Logger.debug.print(F("New MQTT message: "));
  Logger.debug.print(strTopic);
  Logger.debug.print(F(" .. "));
  Logger.debug.println(strPayload);

  if (strTopic.startsWith(settings.mqttSendTopic)) {
    if (onRfDataCallback) {
      onRfDataCallback(String(topic + settings.mqttSendTopic.length()),
                       strPayload);
    }
  }
}

void MqttClient::registerRfDataHandler(const MqttClient::RfDataCb &cb) {
  onRfDataCallback = cb;
}

void MqttClient::publishCode(const String &protocol, const String &payload) {
  String topic = settings.mqttReceiveTopic + protocol;

  Logger.debug.print(F("Publish MQTT message: "));
  Logger.debug.print(topic);
  Logger.debug.print(F(" retain="));
  Logger.debug.print(settings.mqttRetain);
  Logger.debug.print(F(" .. "));
  Logger.debug.println(payload);
  mqttClient.publish(topic.c_str(), payload.c_str(), settings.mqttRetain);
}

bool MqttClient::isConnected() { return mqttClient.connected(); }
