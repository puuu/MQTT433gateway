/*
  MQTT433gateway - MQTT 433.92 MHz radio gateway utilizing ESPiLight
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2016 Puuu

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

#include "net-passwd.h"

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>

#include <ESPiLight.h>
#include <PubSubClient.h>

#include <Heartbeat.h>
#include <SHAauth.h>

#ifndef myMQTT_USERNAME
#define myMQTT_USERNAME nullptr
#define myMQTT_PASSWORD nullptr
#endif

const char *ssid = mySSID;
const char *password = myWIFIPASSWD;
const char *mqttBroker = myMQTT_BROCKER;
const char *mqttUser = myMQTT_USERNAME;
const char *mqttPassword = myMQTT_PASSWORD;

const int RECEIVER_PIN = 12;  // avoid 0, 2, 15, 16
const int TRANSMITTER_PIN = 4;
const int HEARTBEAD_LED_PIN = 0;

WiFiClient wifi;
PubSubClient mqtt(wifi);
Heartbeat beatLED(HEARTBEAD_LED_PIN);
ESPiLight rf(TRANSMITTER_PIN);
SHAauth otaAuth(myOTA_PASSWD);

const String mainTopic = String("rfESP_") + String(ESP.getChipId(), HEX);
const String globalTopic = "rf434";
bool logMode = false;
bool rawMode = false;
String otaURL = "";

void setupWifi() {
  delay(10);
  beatLED.on();
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);  // Explicitly set station mode
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    beatLED.off();
    delay(500);
    beatLED.off();
    delay(500);
    beatLED.on();
    Serial.print(F("."));
  }

  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());
}

void transmitt(const String &protocol, const char *message) {
  Serial.print(F("rf send "));
  Serial.print(message);
  Serial.print(F(" with protocol "));
  Serial.println(protocol);

  if (protocol == F("RAW")) {
    uint16_t rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen =
        rf.stringToPulseTrain(message, rawpulses, MAXPULSESTREAMLENGTH);
    if (rawlen > 0) {
      rf.sendPulseTrain(rawpulses, rawlen);
    }
  } else {
    rf.send(protocol, message);
  }
}

void mqttCallback(const char *topic_, const byte *payload_,
                  unsigned int length) {
  Serial.print(F("Message arrived ["));
  Serial.print(topic_);
  Serial.print(F("] "));
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload_[i]);
  }
  Serial.println();

  String topic = topic_;
  char payload[length + 1];
  strncpy(payload, (char *)payload_, length);
  payload[length] = '\0';

  String sendTopic = globalTopic + F("/send/");
  if (topic.startsWith(sendTopic)) {
    transmitt(topic.substring(sendTopic.length()), payload);
  }
  sendTopic = mainTopic + F("/send/");
  if (topic.startsWith(sendTopic)) {
    transmitt(topic.substring(sendTopic.length()), payload);
  }
  if (topic == (mainTopic + F("/set/raw"))) {
    Serial.println(F("Change raw mode."));
    if (payload[0] == '1') {
      rawMode = true;
    } else {
      rawMode = false;
    }
  }
  if (topic == (mainTopic + F("/set/log"))) {
    Serial.println(F("Change log mode."));
    if (payload[0] == '1') {
      logMode = true;
    } else {
      logMode = false;
    }
  }
  if (topic == (mainTopic + F("/ota/url"))) {
    otaURL = String(payload);
    mqtt.publish((mainTopic + F("/ota/nonce")).c_str(),
                 otaAuth.nonce().c_str());
  }
  if ((topic == (mainTopic + F("/ota/passwd"))) && (otaURL.length() > 7)) {
    if (otaAuth.verify(payload)) {
      beatLED.on();
      Serial.print(F("Start OTA update from: "));
      Serial.println(otaURL);
      rf.disableReceiver();
      t_httpUpdate_return ret = ESPhttpUpdate.update(otaURL);
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Serial.print(F("HTTP_UPDATE_FAILD Error ("));
          Serial.print(ESPhttpUpdate.getLastError());
          Serial.print(F("): "));
          Serial.println(ESPhttpUpdate.getLastErrorString());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
          break;
        case HTTP_UPDATE_OK:
          Serial.println(F("HTTP_UPDATE_OK"));  // may not called ESPhttpUpdate
                                                // reboot the ESP?
          ESP.restart();
          break;
      }
      rf.enableReceiver();
    } else {
      Serial.println(F("OTA authentication failed!"));
    }
  }
}

void rfCallback(const String &protocol, const String &message, int status,
                int repeats, const String &deviceID) {
  Serial.print(F("RF signal arrived ["));
  Serial.print(protocol);
  Serial.print(F("]/["));
  Serial.print(deviceID);
  Serial.print(F("] ("));
  Serial.print(status);
  Serial.print(F(") "));
  Serial.print(message);
  Serial.print(F(" ("));
  Serial.print(repeats);
  Serial.println(F(")"));

  if (status == VALID) {
    String topic = globalTopic + String(F("/recv/")) + protocol;
    if (deviceID != nullptr) {
      topic += String('/') + deviceID;
    }
    mqtt.publish(topic.c_str(), message.c_str(), true);
  }
  if (logMode) {
    String topic = mainTopic + String(F("/log/")) + String(status) +
                   String('/') + protocol;
    mqtt.publish(topic.c_str(), message.c_str());
  }
}

void rfRawCallback(const uint16_t *pulses, int length) {
  if (rawMode) {
    String data = rf.pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Serial.print(F("RAW RF signal ("));
      Serial.print(length);
      Serial.print(F("): "));
      Serial.print(data);
      Serial.println();

      mqtt.publish((mainTopic + F("/recvRaw")).c_str(), data.c_str());
    }
  }
}

void reconnect() {
  beatLED.on();
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (mqtt.connect(mainTopic.c_str(), mqttUser, mqttPassword,
                     mainTopic.c_str(), 0, true, "offline")) {
      Serial.println(F("connected"));
      mqtt.publish(mainTopic.c_str(), "online", true);
      mqtt.subscribe((mainTopic + F("/set/+")).c_str());
      mqtt.subscribe((mainTopic + F("/ota/+")).c_str());
      mqtt.subscribe((mainTopic + F("/send/+")).c_str());
      mqtt.subscribe((globalTopic + F("/send/+")).c_str());
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      beatLED.off();
      delay(500);
      beatLED.on();
      delay(4500);
    }
  }
  beatLED.off();
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  setupWifi();
  mqtt.setServer(mqttBroker, 1883);
  mqtt.setCallback(mqttCallback);
  pinMode(RECEIVER_PIN,
          INPUT_PULLUP);  // 5V protection with reverse diode needs pullup
  rf.setCallback(rfCallback);
  rf.setPulseTrainCallBack(rfRawCallback);
  rf.initReceiver(RECEIVER_PIN);
  Serial.println();
  Serial.print(F("Name: "));
  Serial.println(mainTopic);
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
  rf.loop();
  beatLED.loop();
}
