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

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <WString.h>
#include <WiFiClient.h>

#include <Settings.h>

#ifndef MQTT_CONNECTION_ATTEMPT_DELAY
#define MQTT_CONNECTION_ATTEMPT_DELAY 5000
#endif

class PubSubClient;

class MqttClient {
 public:
  using HandlerCallback =
      std::function<void(const String &topic_part, const String &payload)>;

  MqttClient(const Settings &settings, WiFiClient &client);

  void begin();

  bool connect();

  void reconnect();

  bool subsrcibe();

  void loop();

  void publishCode(const String &protocol, const String &payload);

  void registerRfDataHandler(const HandlerCallback &cb);

  ~MqttClient();

 private:
  void onMessage(char *topic, uint8_t *payload, unsigned int length);

  const Settings &settings;
  PubSubClient *mqttClient = nullptr;

  HandlerCallback onSendCallback;
  unsigned long lastConnectAttempt;
};

#endif  // MQTTCLIENT_H
