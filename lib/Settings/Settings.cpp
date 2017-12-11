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

#include "Settings.h"

#include <ArduinoJson.h>

#include <debug_helper.h>

static inline Settings::SettingTypeSet setFor(const SettingType type) {
  return Settings::SettingTypeSet().set(type);
}

static inline String maskSensible(const String &val, const bool sensible) {
  return (sensible ? val : F("xxx"));
}

template <typename TVal, typename TKey>
bool setIfPresent(JsonObject &obj, TKey key, TVal &var) {
  if (obj.containsKey(key)) {
    TVal tmp = obj.get<TVal>(key);
    if (tmp != var) {
      var = tmp;
      return true;
    }
  }
  return false;
}

struct SettingListener {
  const SettingType type;
  const Settings::SettingCallbackFn callback;
  SettingListener *const next;

  SettingListener(const SettingType &type,
                  const Settings::SettingCallbackFn &cb,
                  SettingListener *const next)
      : type(type), callback(cb), next(next) {}
};

void Settings::registerChangeHandler(SettingType setting,
                                     const SettingCallbackFn &callback) {
  listeners = new SettingListener(setting, callback, listeners);
}

void Settings::onConfigChange(SettingTypeSet typeSet) const {
  SettingListener *current = listeners;
  while (current != nullptr) {
    if (typeSet[current->type]) {
      current->callback(*this);
    }
    current = current->next;
  }
}

void Settings::load() {
  // ToDo load

  // Fire for all
  onConfigChange(SettingTypeSet().set());
}

Settings::~Settings() {
  SettingListener *current = listeners;
  while (current != nullptr) {
    SettingListener *tmp = current;
    current = current->next;
    delete tmp;
  }
}

void Settings::updateProtocols(const String &protocols) {
  this->rfProtocols = protocols;
  onConfigChange(setFor(RF_PROTOCOL));
}

void Settings::updateOtaUrl(const String &otaUrl) { this->otaUrl = otaUrl; }

void Settings::serialize(Stream &stream, bool pretty, bool sensible) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root[F("deviceName")] = this->deviceName;
  root[F("mqttReceiveTopic")] = this->mqttReceiveTopic;
  root[F("mqttLogTopic")] = this->mqttLogTopic;
  root[F("mqttRawRopic")] = this->mqttRawRopic;
  root[F("mqttSendTopic")] = this->mqttSendTopic;
  root[F("mqttConfigTopic")] = this->mqttConfigTopic;
  root[F("mqttOtaTopic")] = this->mqttOtaTopic;
  root[F("mqttBroker")] = this->mqttBroker;
  root[F("mqttBrokerPort")] = this->mqttBrokerPort;
  root[F("mqttUser")] = this->mqttUser;
  root[F("mqttPassword")] = maskSensible(this->mqttPassword, sensible);
  root[F("mqttRetain")] = this->mqttRetain;
  root[F("rfReceiverPin")] = this->rfReceiverPin;
  root[F("rfTransmitterPin")] = this->rfTransmitterPin;
  root[F("rfEchoMessages")] = this->rfEchoMessages;
  root[F("rfProtocols")] = this->rfProtocols;
  root[F("otaPassword")] = maskSensible(this->otaPassword, sensible);
  root[F("otaUrl")] = this->otaUrl;

  if (pretty) {
    root.prettyPrintTo(stream);
  } else {
    root.printTo(stream);
  }
}

void Settings::deserialize(const String &json, const bool fireCallbacks) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &parsedSettings = jsonBuffer.parseObject(json);

  if (!parsedSettings.success()) {
    DebugLn(F("Config parse failled!"));
    return;
  }

  SettingTypeSet changed;

  changed.set(BASE,
              (setIfPresent(parsedSettings, F("deviceName"), deviceName)));

  changed.set(
      MQTT,
      (setIfPresent(parsedSettings, F("mqttReceiveTopic"), mqttReceiveTopic) ||
       setIfPresent(parsedSettings, F("mqttLogTopic"), mqttLogTopic) ||
       setIfPresent(parsedSettings, F("mqttRawRopic"), mqttRawRopic) ||
       setIfPresent(parsedSettings, F("mqttSendTopic"), mqttSendTopic) ||
       setIfPresent(parsedSettings, F("mqttConfigTopic"), mqttConfigTopic) ||
       setIfPresent(parsedSettings, F("mqttOtaTopic"), mqttOtaTopic) ||
       setIfPresent(parsedSettings, F("mqttBroker"), mqttBroker) ||
       setIfPresent(parsedSettings, F("mqttBrokerPort"), mqttBrokerPort) ||
       setIfPresent(parsedSettings, F("mqttUser"), mqttUser) ||
       setIfPresent(parsedSettings, F("mqttPassword"), mqttPassword) ||
       setIfPresent(parsedSettings, F("mqttRetain"), mqttRetain)));

  changed.set(
      RF_CONFIG,
      (setIfPresent(parsedSettings, F("rfReceiverPin"), rfReceiverPin) ||
       setIfPresent(parsedSettings, F("rfTransmitterPin"), rfTransmitterPin)));

  changed.set(RF_ECHO, (setIfPresent(parsedSettings, F("rfEchoMessages"),
                                     rfEchoMessages)));

  changed.set(RF_PROTOCOL,
              (setIfPresent(parsedSettings, F("rfProtocols"), rfProtocols)));

  changed.set(OTA,
              (setIfPresent(parsedSettings, F("otaPassword"), otaPassword) ||
               setIfPresent(parsedSettings, F("otaUrl"), otaUrl)));

  if (fireCallbacks) {
    onConfigChange(changed);
  }
}

void Settings::reset() {}
