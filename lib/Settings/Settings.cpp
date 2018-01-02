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
#include <algorithm>

#include "Settings.h"

#include <FS.h>

#include <ArduinoSimpleLogging.h>

namespace JsonKey {
const char PROGMEM deviceName[] = "deviceName";
const char PROGMEM mqttReceiveTopic[] = "mqttReceiveTopic";
const char PROGMEM mqttSendTopic[] = "mqttSendTopic";
const char PROGMEM mqttBroker[] = "mqttBroker";
const char PROGMEM mqttBrokerPort[] = "mqttBrokerPort";
const char PROGMEM mqttUser[] = "mqttUser";
const char PROGMEM mqttRetain[] = "mqttRetain";
const char PROGMEM rfReceiverPin[] = "rfReceiverPin";
const char PROGMEM rfTransmitterPin[] = "rfTransmitterPin";
const char PROGMEM rfEchoMessages[] = "rfEchoMessages";
const char PROGMEM rfProtocols[] = "rfProtocols";
const char PROGMEM serialLogLevel[] = "serialLogLevel";
const char PROGMEM webLogLevel[] = "webLogLevel";
const char PROGMEM syslogLevel[] = "syslogLevel";
const char PROGMEM syslogHost[] = "syslogHost";
const char PROGMEM syslogPort[] = "syslogPort";
const char PROGMEM mqttPassword[] = "mqttPassword";
const char PROGMEM configPassword[] = "configPassword";
}

static inline bool any(std::initializer_list<bool> items) {
  return std::any_of(items.begin(), items.end(),
                     [](bool item) { return item; });
}

std::function<bool(const String &)> notEmpty() {
  return [](const String &str) { return str.length() > 0; };
}

template <typename T>
std::function<bool(const T &)> notZero() {
  return [](const T &val) { return val != 0; };
}

template <typename TVal, typename TKey>
bool setIfPresent(JsonObject &obj, TKey key, TVal &var,
                  const std::function<bool(const TVal &)> &validator =
                      std::function<bool(const TVal &)>()) {
  if (obj.containsKey(key)) {
    TVal tmp = obj.get<TVal>(key);
    if (tmp != var && (!validator || validator(tmp))) {
      var = tmp;
      return true;
    }
  }
  return false;
}

void Settings::registerChangeHandler(SettingType setting,
                                     const SettingCallbackFn &callback) {
  listeners.emplace_front(setting, callback);
}

void Settings::onConfigChange(SettingTypeSet typeSet) const {
  for (const auto &listener : listeners) {
    if (typeSet[listener.type]) {
      listener.callback(*this);
    }
  }
}

void Settings::load() {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    File file = SPIFFS.open(SETTINGS_FILE, "r");
    if (!file) {
      Logger.error.println(F("Cannot open setings file!"));
      return;
    }
    DynamicJsonBuffer jsonBuffer;
    JsonObject &parsedSettings = jsonBuffer.parseObject(file);
    String settingsContents = file.readStringUntil(SETTINGS_TERMINATOR);

    file.close();

    applyJson(parsedSettings);
  }
}

void Settings::notifyAll() {
  // Fire for all
  onConfigChange(SettingTypeSet().set());
}

void Settings::save() {
  File file = SPIFFS.open(SETTINGS_FILE, "w");

  if (!file) {
    Logger.error.println(F("Opening settings file failed"));
  } else {
    serialize(file, false, true);
    file.close();
  }
}

Settings::~Settings() = default;

void Settings::doSerialize(JsonObject &root, bool sensible) const {
  root[JsonKey::deviceName] = this->deviceName;
  root[JsonKey::mqttReceiveTopic] = this->mqttReceiveTopic;
  root[JsonKey::mqttSendTopic] = this->mqttSendTopic;
  root[JsonKey::mqttBroker] = this->mqttBroker;
  root[JsonKey::mqttBrokerPort] = this->mqttBrokerPort;
  root[JsonKey::mqttUser] = this->mqttUser;
  root[JsonKey::mqttRetain] = this->mqttRetain;
  root[JsonKey::rfReceiverPin] = this->rfReceiverPin;
  root[JsonKey::rfTransmitterPin] = this->rfTransmitterPin;
  root[JsonKey::rfEchoMessages] = this->rfEchoMessages;

  {
    DynamicJsonBuffer protoBuffer;
    JsonArray &parsedProtocols = protoBuffer.parseArray(this->rfProtocols);
    JsonArray &protos = root.createNestedArray(JsonKey::rfProtocols);
    for (const auto proto : parsedProtocols) {
      protos.add(proto.as<String>());
    }
  }

  root[JsonKey::serialLogLevel] = this->serialLogLevel;
  root[JsonKey::webLogLevel] = this->webLogLevel;
  root[JsonKey::syslogLevel] = this->syslogLevel;
  root[JsonKey::syslogHost] = this->syslogHost;
  root[JsonKey::syslogPort] = this->syslogPort;

  if (sensible) {
    root[JsonKey::mqttPassword] = this->mqttPassword;
    root[JsonKey::configPassword] = this->configPassword;
  }
}

void Settings::deserialize(const String &json) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &parsedSettings = jsonBuffer.parseObject(json);

  onConfigChange(applyJson(parsedSettings));
}

Settings::SettingTypeSet Settings::applyJson(JsonObject &parsedSettings) {
  SettingTypeSet changed;

  if (!parsedSettings.success()) {
    Logger.warning.println(F("Config parse failed!"));
    return changed;
  }

  changed.set(BASE, setIfPresent(parsedSettings, JsonKey::deviceName,
                                 deviceName, notEmpty()));

  changed.set(
      MQTT,
      any({setIfPresent(parsedSettings, JsonKey::mqttReceiveTopic,
                        mqttReceiveTopic),
           setIfPresent(parsedSettings, JsonKey::mqttSendTopic, mqttSendTopic),
           setIfPresent(parsedSettings, JsonKey::mqttBroker, mqttBroker,
                        notEmpty()),
           setIfPresent(parsedSettings, JsonKey::mqttBrokerPort, mqttBrokerPort,
                        notZero<uint16_t>()),
           setIfPresent(parsedSettings, JsonKey::mqttUser, mqttUser),
           setIfPresent(parsedSettings, JsonKey::mqttPassword, mqttPassword,
                        notEmpty()),
           setIfPresent(parsedSettings, JsonKey::mqttRetain, mqttRetain)}));

  changed.set(
      RF_CONFIG,
      any({setIfPresent(parsedSettings, JsonKey::rfReceiverPin, rfReceiverPin),
           setIfPresent(parsedSettings, JsonKey::rfTransmitterPin,
                        rfTransmitterPin)}));

  changed.set(RF_ECHO, (setIfPresent(parsedSettings, JsonKey::rfEchoMessages,
                                     rfEchoMessages)));

  if (parsedSettings.containsKey(JsonKey::rfProtocols)) {
    String buff;
    parsedSettings[JsonKey::rfProtocols].printTo(buff);
    if (buff != rfProtocols) {
      rfProtocols = buff;
      changed.set(RF_PROTOCOL, true);
    }
  }

  changed.set(
      LOGGING,
      any({setIfPresent(parsedSettings, JsonKey::serialLogLevel,
                        serialLogLevel),
           setIfPresent(parsedSettings, JsonKey::webLogLevel, webLogLevel)}));

  changed.set(WEB_CONFIG, setIfPresent(parsedSettings, JsonKey::configPassword,
                                       configPassword, notEmpty()));

  changed.set(
      SYSLOG,
      any({setIfPresent(parsedSettings, JsonKey::syslogLevel, syslogLevel),
           setIfPresent(parsedSettings, JsonKey::syslogHost, syslogHost),
           setIfPresent(parsedSettings, JsonKey::syslogPort, syslogPort)}));

  return changed;
}

void Settings::reset() {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    SPIFFS.remove(SETTINGS_FILE);
  }
}
