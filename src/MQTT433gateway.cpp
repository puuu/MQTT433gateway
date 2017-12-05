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

#include "config.h"

#include <ESP8266httpUpdate.h>

#include <ESPiLight.h>
#include <PubSubClient.h>

#include <Heartbeat.h>
#include <SHAauth.h>
#include <WifiConnection.h>
#include <debug_helper.h>

#ifndef myMQTT_USERNAME
#define myMQTT_USERNAME nullptr
#define myMQTT_PASSWORD nullptr
#endif

#ifndef mySSID
#define mySSID nullptr
#define myWIFIPASSWD nullptr
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

void transmitt(const String &protocol, const char *message) {
  Debug(F("rf send "));
  Debug(message);
  Debug(F(" with protocol "));
  DebugLn(protocol);

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
  Debug(F("Message arrived ["));
  Debug(topic_);
  Debug(F("] "));
  for (unsigned int i = 0; i < length; i++) {
    Debug((char)payload_[i]);
  }
  DebugLn();

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
  if (topic == (mainTopic + F("/set/protocols"))) {
    DebugLn(F("Change available protocols."));
    rf.limitProtocols(payload);
  }
  if (topic == (mainTopic + F("/set/raw"))) {
    DebugLn(F("Change raw mode."));
    rawMode = payload[0] == '1';
  }
  if (topic == (mainTopic + F("/set/log"))) {
    DebugLn(F("Change log mode."));
    logMode = payload[0] == '1';
  }
  if (topic == (mainTopic + F("/ota/url"))) {
    otaURL = String(payload);
    mqtt.publish((mainTopic + F("/ota/nonce")).c_str(),
                 otaAuth.nonce().c_str());
  }
  if ((topic == (mainTopic + F("/ota/passwd"))) && (otaURL.length() > 7)) {
    if (otaAuth.verify(payload)) {
      beatLED.on();
      Debug(F("Start OTA update from: "));
      DebugLn(otaURL);
      rf.disableReceiver();
      t_httpUpdate_return ret = ESPhttpUpdate.update(otaURL);
      switch (ret) {
        case HTTP_UPDATE_FAILED:
          Debug(F("HTTP_UPDATE_FAILD Error ("));
          Debug(ESPhttpUpdate.getLastError());
          Debug(F("): "));
          DebugLn(ESPhttpUpdate.getLastErrorString());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          DebugLn(F("HTTP_UPDATE_NO_UPDATES"));
          break;
        case HTTP_UPDATE_OK:
          DebugLn(F("HTTP_UPDATE_OK"));  // may not called ESPhttpUpdate
                                         // reboot the ESP?
          ESP.restart();
          break;
      }
      rf.enableReceiver();
    } else {
      DebugLn(F("OTA authentication failed!"));
    }
  }
}

void rfCallback(const String &protocol, const String &message, int status,
                int repeats, const String &deviceID) {
  Debug(F("RF signal arrived ["));
  Debug(protocol);
  Debug(F("]/["));
  Debug(deviceID);
  Debug(F("] ("));
  Debug(status);
  Debug(F(") "));
  Debug(message);
  Debug(F(" ("));
  Debug(repeats);
  DebugLn(F(")"));

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

void rfRawCallback(const uint16_t *pulses, size_t length) {
  if (rawMode) {
    String data = rf.pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Debug(F("RAW RF signal ("));
      Debug(length);
      Debug(F("): "));
      Debug(data);
      DebugLn();

      mqtt.publish((mainTopic + F("/recvRaw")).c_str(), data.c_str());
    }
  }
}

void reconnect() {
  beatLED.on();
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Debug(F("Attempting MQTT connection..."));
    // Attempt to connect
    if (mqtt.connect(mainTopic.c_str(), mqttUser, mqttPassword,
                     mainTopic.c_str(), 0, true, "offline")) {
      DebugLn(F("connected"));
      mqtt.publish(mainTopic.c_str(), "online", true);
      mqtt.subscribe((mainTopic + F("/set/+")).c_str());
      mqtt.subscribe((mainTopic + F("/ota/+")).c_str());
      mqtt.subscribe((mainTopic + F("/send/+")).c_str());
      mqtt.subscribe((globalTopic + F("/send/+")).c_str());
    } else {
      Debug(F("failed, rc="));
      Debug(mqtt.state());
      DebugLn(F(" try again in 5 seconds"));
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

  if (!connectWifi(ssid, password, []() { beatLED.loop(); })) {
    DebugLn(F("Try connectiing again after reboot"));
    ESP.restart();
  }

  mqtt.setServer(mqttBroker, 1883);
  mqtt.setCallback(mqttCallback);
  pinMode(RECEIVER_PIN,
          INPUT_PULLUP);  // 5V protection with reverse diode needs pullup
  rf.setCallback(rfCallback);
  rf.setPulseTrainCallBack(rfRawCallback);
  rf.initReceiver(RECEIVER_PIN);
  DebugLn();
  Debug(F("Name: "));
  DebugLn(mainTopic);
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
  rf.loop();
  beatLED.loop();
}
