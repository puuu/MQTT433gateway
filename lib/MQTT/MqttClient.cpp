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

#include <PubSubClient.h>

#include <ArduinoSimpleLogging.h>

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
    : settings(settings),
      mqttClient(new PubSubClient(client)),
      lastConnectAttempt(0) {
  Logger.debug.println(F("New MQTT client instance."));
}

MqttClient::~MqttClient() {
  if (mqttClient) {
    mqttClient->disconnect();
    delete mqttClient;
  }
}

void MqttClient::begin() {
  using namespace std::placeholders;
  mqttClient->setServer(settings.mqttBroker.c_str(), settings.mqttBrokerPort);

  mqttClient->setCallback(std::bind(&MqttClient::onMessage, this, _1, _2, _3));

  reconnect();
}

static String stateMessage(const bool online) {
  return String(F("{\"chipId\":\"")) + String(ESP.getChipId(), HEX) +
         String(F(
             "\",\"firmware\":\"" QUOTE(FIRMWARE_VERSION) "\",\"state\":\"")) +
         String(online ? F("online") : F("offline")) + String(F("\"}"));
}

static String stateTopic(const String &devName) {
  return devName + F("/state");
}

bool MqttClient::connect() {
  if (0 == settings.mqttUser.length()) {
    return mqttClient->connect(settings.deviceName.c_str(),
                               stateTopic(settings.deviceName).c_str(), 0, true,
                               stateMessage(false).c_str());
  } else {
    return mqttClient->connect(
        settings.deviceName.c_str(), settings.mqttUser.c_str(),
        settings.mqttPassword.c_str(), stateTopic(settings.deviceName).c_str(),
        0, true, stateMessage(false).c_str());
  }
}

void MqttClient::reconnect() {
  if (lastConnectAttempt > 0 &&
      (millis() - lastConnectAttempt) < MQTT_CONNECTION_ATTEMPT_DELAY) {
    return;
  }

  if (!mqttClient->connected()) {
    if (connect()) {
      Logger.info.println(F("MQTT connected"));
      if (subsrcibe()) {
        mqttClient->publish(stateTopic(settings.deviceName).c_str(),
                            stateMessage(true).c_str(), true);
        Logger.info.println(F("Subscribed!"));
      } else {
        Logger.error.println(F("Cannot subsrcibe!"));
      }
    } else {
      Logger.error.println(F("Cannot connect!"));
    }
  }

  lastConnectAttempt = millis();
}

bool MqttClient::subsrcibe() {
  Logger.debug.println(F("...Subscribe to: "));
  Logger.debug.println((settings.mqttSendTopic + "+").c_str());

  return mqttClient->subscribe((settings.mqttSendTopic + "+").c_str());
}

void MqttClient::loop() {
  reconnect();
  mqttClient->loop();
}

void MqttClient::onMessage(char *topic, uint8_t *payload, unsigned int length) {
  PayloadString strPayload(payload, length);
  String strTopic(topic);

  Logger.debug.print(F("New MQTT message: "));
  Logger.debug.print(strTopic);
  Logger.debug.print(F(" .. "));
  Logger.debug.println(strPayload);

  if (strTopic.startsWith(settings.mqttSendTopic)) {
    if (onSendCallback) {
      onSendCallback(String(topic + settings.mqttSendTopic.length()),
                     strPayload);
    }
  }
}

void MqttClient::registerRfDataHandler(const MqttClient::HandlerCallback &cb) {
  onSendCallback = cb;
}

void MqttClient::publishCode(const String &protocol, const String &payload) {
  mqttClient->publish((settings.mqttReceiveTopic + protocol).c_str(),
                      payload.c_str(), settings.mqttRetain);
}
