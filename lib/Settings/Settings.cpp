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

#include <FS.h>
#include <WString.h>

#include <ArduinoSimpleLogging.h>

#include "Settings.h"

namespace JsonKey {
char deviceName[] = "deviceName";
char configPassword[] = "configPassword";
char mqttBroker[] = "mqttBroker";
char mqttBrokerPort[] = "mqttBrokerPort";
char mqttUser[] = "mqttUser";
char mqttPassword[] = "mqttPassword";
char mqttRetain[] = "mqttRetain";
char mqttReceiveTopic[] = "mqttReceiveTopic";
char mqttSendTopic[] = "mqttSendTopic";
char rfEchoMessages[] = "rfEchoMessages";
char rfReceiverPin[] = "rfReceiverPin";
char rfTransmitterPin[] = "rfTransmitterPin";
char rfReceiverPinPullUp[] = "rfReceiverPinPullUp";
char rfProtocols[] = "rfProtocols";
char serialLogLevel[] = "serialLogLevel";
char webLogLevel[] = "webLogLevel";
char syslogLevel[] = "syslogLevel";
char syslogHost[] = "syslogHost";
char syslogPort[] = "syslogPort";
char ledPin[] = "ledPin";
char ledActiveHigh[] = "ledActiveHigh";
}  // namespace JsonKey

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

static void logInvalidWarning(const String &key) {
  Logger.warning.print(F("Setting "));
  Logger.warning.print(key);
  Logger.warning.println(F(" is not valid, will ignore it."));
}

template <typename TVal, typename TKey>
bool setIfPresent(JsonObject &obj, TKey key, TVal &var,
                  const std::function<bool(const TVal &)> &validator =
                      std::function<bool(const TVal &)>()) {
  if (obj.containsKey(key)) {
    TVal tmp = obj.get<TVal>(key);
    if (tmp != var) {
      if (!validator || validator(tmp)) {
        var = tmp;
        return true;
      } else {
        logInvalidWarning(key);
      }
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
  Logger.debug.println(F("Loading config file."));
  if (SPIFFS.exists(FPSTR(SETTINGS_FILE))) {
    File file = SPIFFS.open(FPSTR(SETTINGS_FILE), "r");
    if (!file) {
      Logger.error.println(F("Open settings file for read failed!"));
      return;
    }
    DynamicJsonBuffer jsonBuffer;
    JsonObject &parsedSettings = jsonBuffer.parseObject(file);

    file.close();

    applyJson(parsedSettings);
  }
}

void Settings::notifyAll() {
  // Fire for all
  onConfigChange(SettingTypeSet().set());
}

void Settings::save() {
  Logger.debug.println(F("Saving config file."));
  File file = SPIFFS.open(FPSTR(SETTINGS_FILE), "w");

  if (!file) {
    Logger.error.println(F("Open settings file for write failed!"));
  } else {
    serialize(file, false, true);
    file.close();
  }
}

Settings::~Settings() = default;

void Settings::doSerialize(JsonObject &root, bool sensible) const {
  root[JsonKey::deviceName] = this->deviceName;
  root[JsonKey::mqttBroker] = this->mqttBroker;
  root[JsonKey::mqttBrokerPort] = this->mqttBrokerPort;
  root[JsonKey::mqttUser] = this->mqttUser;
  root[JsonKey::mqttRetain] = this->mqttRetain;
  root[JsonKey::mqttReceiveTopic] = this->mqttReceiveTopic;
  root[JsonKey::mqttSendTopic] = this->mqttSendTopic;
  root[JsonKey::rfEchoMessages] = this->rfEchoMessages;
  root[JsonKey::rfReceiverPin] = this->rfReceiverPin;
  root[JsonKey::rfTransmitterPin] = this->rfTransmitterPin;
  root[JsonKey::rfReceiverPinPullUp] = this->rfReceiverPinPullUp;
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
  root[JsonKey::ledPin] = this->ledPin;
  root[JsonKey::ledActiveHigh] = this->ledActiveHigh;

  if (sensible) {
    root[JsonKey::configPassword] = this->configPassword;
    root[JsonKey::mqttPassword] = this->mqttPassword;
  }
}

void Settings::deserialize(const String &json) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &parsedSettings = jsonBuffer.parseObject(json);

  onConfigChange(applyJson(parsedSettings));
}

Settings::SettingTypeSet Settings::applyJson(JsonObject &parsedSettings) {
  Logger.debug.println(F("Applying config settings."));
  SettingTypeSet changed;

  if (!parsedSettings.success()) {
    Logger.warning.println(F("Parsing config data as JSON object failed!"));
    return changed;
  }

  changed.set(BASE, setIfPresent(parsedSettings, JsonKey::deviceName,
                                 deviceName, notEmpty()));
  bool pass_before = hasValidPassword();
  changed.set(WEB_CONFIG, setIfPresent(parsedSettings, JsonKey::configPassword,
                                       configPassword, notEmpty()));
  changed.set(
      MQTT, any({changed[BASE],
                 setIfPresent(parsedSettings, JsonKey::mqttBroker, mqttBroker,
                              notEmpty()),
                 setIfPresent(parsedSettings, JsonKey::mqttBrokerPort,
                              mqttBrokerPort, notZero<uint16_t>()),
                 setIfPresent(parsedSettings, JsonKey::mqttUser, mqttUser),
                 setIfPresent(parsedSettings, JsonKey::mqttPassword,
                              mqttPassword, notEmpty()),
                 setIfPresent(parsedSettings, JsonKey::mqttRetain, mqttRetain),
                 setIfPresent(parsedSettings, JsonKey::mqttReceiveTopic,
                              mqttReceiveTopic, notEmpty()),
                 setIfPresent(parsedSettings, JsonKey::mqttSendTopic,
                              mqttSendTopic, notEmpty())}));
  changed.set(RF_ECHO, (setIfPresent(parsedSettings, JsonKey::rfEchoMessages,
                                     rfEchoMessages)));
  changed.set(
      RF_CONFIG,
      any({setIfPresent(parsedSettings, JsonKey::rfReceiverPin, rfReceiverPin),
           setIfPresent(parsedSettings, JsonKey::rfTransmitterPin,
                        rfTransmitterPin),
           setIfPresent(parsedSettings, JsonKey::rfReceiverPinPullUp,
                        rfReceiverPinPullUp)}));
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
  changed.set(
      SYSLOG,
      any({setIfPresent(parsedSettings, JsonKey::syslogLevel, syslogLevel),
           setIfPresent(parsedSettings, JsonKey::syslogHost, syslogHost),
           setIfPresent(parsedSettings, JsonKey::syslogPort, syslogPort,
                        notZero<uint16_t>())}));
  changed.set(STATUSLED,
              any({setIfPresent(parsedSettings, JsonKey::ledPin, ledPin),
                   setIfPresent(parsedSettings, JsonKey::ledActiveHigh,
                                ledActiveHigh)}));

  if (hasValidPassword() != pass_before) {
    changed.set(MQTT);
    changed.set(RF_CONFIG);
  }

  return changed;
}

void Settings::reset() {
  if (SPIFFS.exists(FPSTR(SETTINGS_FILE))) {
    Logger.info.println(F("Remove config file."));
    SPIFFS.remove(FPSTR(SETTINGS_FILE));
  }
}

bool Settings::hasValidPassword() const {
  return (configPassword.length() > 7) &&
         (configPassword != FPSTR(DEFAULT_PASSWORD));
}
