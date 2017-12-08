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

#ifndef MQTT433GATEWAY_SETTINGS_H
#define MQTT433GATEWAY_SETTINGS_H

#include <bitset>

#include <Esp.h>
#include <WString.h>
#include <functional>

enum SettingType { MQTT, RF_PROTOCOL, RF_ECHO, OTA, _END };

class SettingListener;

class Settings {
 public:
  Settings(const char *defaultBroker, const char *defaultUser,
           const char *defaultPasswd)
      : deviceName(String(F("rfESP_")) + String(ESP.getChipId(), HEX)),
        mqttReceiveTopic(deviceName + F("/recv/")),
        mqttLogTopic(deviceName + F("/log/")),
        mqttRawRopic(deviceName + F("/raw/")),
        mqttSendTopic(deviceName + F("/send/")),
        mqttConfigTopic(deviceName + F("/set/")),
        mqttOtaTopic(deviceName + F("/ota/")),
        mqttBroker(defaultBroker),
        mqttBrokerPort(1883),
        mqttUser(defaultUser),
        mqttPassword(defaultPasswd),
        mqttRetain(true),
        rfEchoMessages(false),
        rfProtocols(F("[]")),
        otaPassword() {}

  using SettingTypeSet = std::bitset<SettingType::_END>;
  using SettingCallbackFn = std::function<void(const Settings &)>;

  void onChange(SettingType setting, const SettingCallbackFn &callback);

  String deviceName;

  String mqttReceiveTopic;
  String mqttLogTopic;
  String mqttRawRopic;

  String mqttSendTopic;
  String mqttConfigTopic;
  String mqttOtaTopic;

  String mqttBroker;
  uint16_t mqttBrokerPort;
  String mqttUser;
  String mqttPassword;
  bool mqttRetain;

  bool rfEchoMessages;
  String rfProtocols;

  String otaPassword;
  String otaUrl;

  void load();

  void updateProtocols(const String &protocols);
  void updateOtaUrl(const String &otaUrl);

  ~Settings();

 private:
  void fireChange(SettingTypeSet typeSet) const;

  SettingListener *listeners = nullptr;
};

#endif  // MQTT433GATEWAY_SETTINGS_H
