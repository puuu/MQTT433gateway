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

#define SETTINGS_FILE "/settings.json"

#define DEFAULT_PASSWORD "MQTT433gateway"
#define DEFAULT_NAME "rf434"

#define RECEIVER_PIN 12  // avoid 0, 2, 15, 16
#define TRANSMITTER_PIN 4

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION unknown
#endif

#define X_QUOTE(x) #x
#define QUOTE(x) X_QUOTE(x)

#include <bitset>
#include <forward_list>
#include <functional>

#include <Esp.h>
#include <Stream.h>
#include <WString.h>

#include <ArduinoJson.h>

enum SettingType {
  BASE,
  WEB_CONFIG,
  MQTT,
  RF_ECHO,
  RF_CONFIG,
  RF_PROTOCOL,
  LOGGING,
  SYSLOG,
  STATUSLED,
  _END
};

class Settings {
 public:
  using SettingTypeSet = std::bitset<SettingType::_END>;
  using SettingCallbackFn = std::function<void(const Settings &)>;

  Settings()
      : deviceName(DEFAULT_NAME),
        configPassword(DEFAULT_PASSWORD),
        mqttBroker(""),
        mqttBrokerPort(1883),
        mqttUser(""),
        mqttPassword(""),
        mqttRetain(true),
        mqttReceiveTopic(deviceName + ("/recv/")),
        mqttSendTopic(deviceName + ("/send/")),
        rfEchoMessages(false),
        rfReceiverPin(RECEIVER_PIN),
        rfTransmitterPin(TRANSMITTER_PIN),
        rfReceiverPinPullUp(true),
        rfProtocols(("[]")),
        serialLogLevel("debug"),
        webLogLevel(""),
        syslogLevel(""),
        syslogHost(""),
        syslogPort(514),
        ledPin(BUILTIN_LED),
        ledActiveHigh(false) {}
  ~Settings();
  void load();
  void save();
  void notifyAll();
  template <typename T>
  void serialize(T &target, bool pretty, bool sensible = true) const {
    DynamicJsonBuffer buffer;
    JsonObject &root = buffer.createObject();
    doSerialize(root, sensible);
    if (pretty) {
      root.prettyPrintTo(target);
    } else {
      root.printTo(target);
    }
  }
  void deserialize(const String &json);
  void reset();
  bool hasValidPassword();
  void registerChangeHandler(SettingType setting,
                             const SettingCallbackFn &callback);

  String deviceName;
  String configPassword;
  String mqttBroker;
  uint16_t mqttBrokerPort;
  String mqttUser;
  String mqttPassword;
  bool mqttRetain;
  String mqttReceiveTopic;
  String mqttSendTopic;
  bool rfEchoMessages;
  int8_t rfReceiverPin;
  int8_t rfTransmitterPin;
  bool rfReceiverPinPullUp;
  String rfProtocols;
  String serialLogLevel;
  String webLogLevel;
  String syslogLevel;
  String syslogHost;
  uint16_t syslogPort;
  uint8_t ledPin;
  bool ledActiveHigh;

 private:
  struct SettingListener {
    const SettingType type;
    const Settings::SettingCallbackFn callback;

    SettingListener(const SettingType &type,
                    const Settings::SettingCallbackFn &cb)
        : type(type), callback(cb) {}
  };

  void onConfigChange(SettingTypeSet typeSet) const;
  SettingTypeSet applyJson(JsonObject &parsedSettings);
  void doSerialize(JsonObject &obj, bool sensible = true) const;

  std::forward_list<SettingListener> listeners;
};

#endif  // MQTT433GATEWAY_SETTINGS_H
