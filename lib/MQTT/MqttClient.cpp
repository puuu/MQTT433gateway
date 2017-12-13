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
  PayloadString(const uint8_t *data, unsigned int len) : String() {
    reserve(len + 1);
    copy(reinterpret_cast<const char *>(data), len);
    buffer[len] = '\0';
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

bool MqttClient::connect() {
  if (0 == settings.mqttUser.length()) {
    return mqttClient->connect(settings.deviceName.c_str(),
                               settings.deviceName.c_str(), 0, true, "offline");
  } else {
    return mqttClient->connect(settings.deviceName.c_str(),
                               settings.mqttUser.c_str(),
                               settings.mqttPassword.c_str(),
                               settings.deviceName.c_str(), 0, true, "offline");
  }
}

void MqttClient::reconnect() {
  if (lastConnectAttempt > 0 &&
      (millis() - lastConnectAttempt) < MQTT_CONNECTION_ATTEMPT_FREQUENCY) {
    return;
  }

  if (!mqttClient->connected()) {
    if (connect()) {
      Logger.info.println(F("MQTT connected"));
      if (subsrcibe()) {
        mqttClient->publish(settings.deviceName.c_str(), "online", true);
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
  Logger.debug.println((settings.mqttOtaTopic + "+").c_str());
  Logger.debug.println((settings.mqttConfigTopic + "+").c_str());
  Logger.debug.println((settings.mqttSendTopic + "+").c_str());

  return mqttClient->subscribe((settings.mqttOtaTopic + "+").c_str()) &&
         mqttClient->subscribe((settings.mqttConfigTopic + "+").c_str()) &&
         mqttClient->subscribe((settings.mqttSendTopic + "+").c_str());
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
    if (codeCallback) {
      codeCallback(String(topic + settings.mqttSendTopic.length()), strPayload);
    }
  }

  if (strTopic.startsWith(settings.mqttConfigTopic)) {
    String topicPart(topic + settings.mqttConfigTopic.length());

    Logger.debug.print(F("Config: "));
    Logger.debug.println(topicPart);

    for (const auto &setHandler : setHandlers) {
      if (topicPart == setHandler.path) {
        setHandler.cb(strPayload);
        return;
      }
    }
  }

  if (strTopic.startsWith(settings.mqttOtaTopic)) {
    String topicPart(topic + settings.mqttOtaTopic.length());
    if (otaCallback) {
      otaCallback(topicPart, strPayload);
    }
  }
}

void MqttClient::registerSetHandler(
    const String &url_part, const MqttClient::SettingHandlerCallback &cb) {
  setHandlers.emplace_front(url_part, cb);
}

void MqttClient::registerOtaHandler(const MqttClient::HandlerCallback &cb) {
  otaCallback = cb;
}

void MqttClient::registerRfDataHandler(const MqttClient::HandlerCallback &cb) {
  codeCallback = cb;
}

void MqttClient::publishCode(const String &protocol, const String &payload) {
  mqttClient->publish((settings.mqttReceiveTopic + protocol).c_str(),
                      payload.c_str(), settings.mqttRetain);
}

void MqttClient::publishLog(int status, const String &protocol,
                            const String &message) {
  mqttClient->publish(
      (settings.mqttLogTopic + String(status) + "/" + protocol).c_str(),
      message.c_str());
}

void MqttClient::publishRaw(const String &data) {
  mqttClient->publish(settings.mqttRawRopic.c_str(), data.c_str());
}

void MqttClient::publishOta(const String &topic, const String payload) {
  mqttClient->publish((settings.mqttOtaTopic + topic).c_str(), payload.c_str());
}
