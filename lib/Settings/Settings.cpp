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
const char PROGMEM deviceName[] = "deviceName";
const char PROGMEM configPassword[] = "configPassword";
const char PROGMEM mqttBroker[] = "mqttBroker";
const char PROGMEM mqttBrokerPort[] = "mqttBrokerPort";
const char PROGMEM mqttUser[] = "mqttUser";
const char PROGMEM mqttPassword[] = "mqttPassword";
const char PROGMEM mqttRetain[] = "mqttRetain";
const char PROGMEM mqttReceiveTopic[] = "mqttReceiveTopic";
const char PROGMEM mqttSendTopic[] = "mqttSendTopic";
const char PROGMEM rfEchoMessages[] = "rfEchoMessages";
const char PROGMEM rfReceiverPin[] = "rfReceiverPin";
const char PROGMEM rfTransmitterPin[] = "rfTransmitterPin";
const char PROGMEM rfReceiverPinPullUp[] = "rfReceiverPinPullUp";
const char PROGMEM rfProtocols[] = "rfProtocols";
const char PROGMEM serialLogLevel[] = "serialLogLevel";
const char PROGMEM webLogLevel[] = "webLogLevel";
const char PROGMEM syslogLevel[] = "syslogLevel";
const char PROGMEM syslogHost[] = "syslogHost";
const char PROGMEM syslogPort[] = "syslogPort";
const char PROGMEM ledPin[] = "ledPin";
const char PROGMEM ledActiveHigh[] = "ledActiveHigh";
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
  if (SPIFFS.exists(SETTINGS_FILE)) {
    File file = SPIFFS.open(SETTINGS_FILE, "r");
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
  File file = SPIFFS.open(SETTINGS_FILE, "w");

  if (!file) {
    Logger.error.println(F("Open settings file for write failed!"));
  } else {
    serialize(file, false, true);
    file.close();
  }
}

Settings::~Settings() = default;

void Settings::doSerialize(JsonObject &root, bool sensible) const {
  root[FPSTR(JsonKey::deviceName)] = this->deviceName;
  root[FPSTR(JsonKey::mqttBroker)] = this->mqttBroker;
  root[FPSTR(JsonKey::mqttBrokerPort)] = this->mqttBrokerPort;
  root[FPSTR(JsonKey::mqttUser)] = this->mqttUser;
  root[FPSTR(JsonKey::mqttRetain)] = this->mqttRetain;
  root[FPSTR(JsonKey::mqttReceiveTopic)] = this->mqttReceiveTopic;
  root[FPSTR(JsonKey::mqttSendTopic)] = this->mqttSendTopic;
  root[FPSTR(JsonKey::rfEchoMessages)] = this->rfEchoMessages;
  root[FPSTR(JsonKey::rfReceiverPin)] = this->rfReceiverPin;
  root[FPSTR(JsonKey::rfTransmitterPin)] = this->rfTransmitterPin;
  root[FPSTR(JsonKey::rfReceiverPinPullUp)] = this->rfReceiverPinPullUp;
  {
    DynamicJsonBuffer protoBuffer;
    JsonArray &parsedProtocols = protoBuffer.parseArray(this->rfProtocols);
    JsonArray &protos = root.createNestedArray(FPSTR(JsonKey::rfProtocols));
    for (const auto proto : parsedProtocols) {
      protos.add(proto.as<String>());
    }
  }
  root[FPSTR(JsonKey::serialLogLevel)] = this->serialLogLevel;
  root[FPSTR(JsonKey::webLogLevel)] = this->webLogLevel;
  root[FPSTR(JsonKey::syslogLevel)] = this->syslogLevel;
  root[FPSTR(JsonKey::syslogHost)] = this->syslogHost;
  root[FPSTR(JsonKey::syslogPort)] = this->syslogPort;
  root[FPSTR(JsonKey::ledPin)] = this->ledPin;
  root[FPSTR(JsonKey::ledActiveHigh)] = this->ledActiveHigh;

  if (sensible) {
    root[FPSTR(JsonKey::configPassword)] = this->configPassword;
    root[FPSTR(JsonKey::mqttPassword)] = this->mqttPassword;
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

  changed.set(BASE, setIfPresent(parsedSettings, FPSTR(JsonKey::deviceName),
                                 deviceName, notEmpty()));
  bool pass_before = hasValidPassword();
  changed.set(WEB_CONFIG,
              setIfPresent(parsedSettings, FPSTR(JsonKey::configPassword),
                           configPassword, notEmpty()));
  changed.set(
      MQTT,
      any({setIfPresent(parsedSettings, FPSTR(JsonKey::mqttBroker), mqttBroker,
                        notEmpty()),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttBrokerPort),
                        mqttBrokerPort, notZero<uint16_t>()),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttUser), mqttUser),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttPassword),
                        mqttPassword, notEmpty()),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttRetain), mqttRetain),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttReceiveTopic),
                        mqttReceiveTopic),
           setIfPresent(parsedSettings, FPSTR(JsonKey::mqttSendTopic),
                        mqttSendTopic)}));
  changed.set(RF_ECHO,
              (setIfPresent(parsedSettings, FPSTR(JsonKey::rfEchoMessages),
                            rfEchoMessages)));
  changed.set(
      RF_CONFIG,
      any({setIfPresent(parsedSettings, FPSTR(JsonKey::rfReceiverPin),
                        rfReceiverPin),
           setIfPresent(parsedSettings, FPSTR(JsonKey::rfTransmitterPin),
                        rfTransmitterPin),
           setIfPresent(parsedSettings, FPSTR(JsonKey::rfReceiverPinPullUp),
                        rfReceiverPinPullUp)}));
  if (parsedSettings.containsKey(FPSTR(JsonKey::rfProtocols))) {
    String buff;
    parsedSettings[FPSTR(JsonKey::rfProtocols)].printTo(buff);
    if (buff != rfProtocols) {
      rfProtocols = buff;
      changed.set(RF_PROTOCOL, true);
    }
  }
  changed.set(LOGGING,
              any({setIfPresent(parsedSettings, FPSTR(JsonKey::serialLogLevel),
                                serialLogLevel),
                   setIfPresent(parsedSettings, FPSTR(JsonKey::webLogLevel),
                                webLogLevel)}));
  changed.set(
      SYSLOG,
      any({setIfPresent(parsedSettings, FPSTR(JsonKey::syslogLevel),
                        syslogLevel),
           setIfPresent(parsedSettings, FPSTR(JsonKey::syslogHost), syslogHost),
           setIfPresent(parsedSettings, FPSTR(JsonKey::syslogPort),
                        syslogPort)}));
  changed.set(STATUSLED,
              any({setIfPresent(parsedSettings, FPSTR(JsonKey::ledPin), ledPin),
                   setIfPresent(parsedSettings, FPSTR(JsonKey::ledActiveHigh),
                                ledActiveHigh)}));

  if (hasValidPassword() != pass_before) {
    changed.set(MQTT);
    changed.set(RF_CONFIG);
  }

  return changed;
}

void Settings::reset() {
  if (SPIFFS.exists(SETTINGS_FILE)) {
    Logger.info.println(F("Remove config file."));
    SPIFFS.remove(SETTINGS_FILE);
  }
}

bool Settings::hasValidPassword() {
  return (configPassword.length() > 7) &&
         (configPassword != F(DEFAULT_PASSWORD));
}
