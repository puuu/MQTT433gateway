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

#include <PubSubClient.h>
#include <ESPiLight.h>

#include "src/SHAauth/SHAauth.h"
#include "src/Heartbeat/Heartbeat.h"

const char* ssid = mySSID;
const char* password = myWIFIPASSWD;
const char* mqtt_server = myMQTT_BROCKER;

#define RECEIVER_PIN 12 //avoid 0, 2, 15, 16
#define TRANSMITTER_PIN 4

WiFiClient wifi;
PubSubClient mqtt(wifi);
Heartbeat beatLED(0);
ESPiLight rf(TRANSMITTER_PIN);
SHAauth otaAuth(myOTA_PASSWD);

const String hostName = String("rfESP_")+String(ESP.getChipId(), HEX);;
const String rfTopic = "rf434";
boolean logMode = false;
boolean rawMode = false;
String otaURL = "";

void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqttCallback);
  pinMode(RECEIVER_PIN, INPUT_PULLUP); //5V protection with reverse diode needs pullup
  rf.setCallback(rfCallback);
  rf.setPulseTrainCallBack(rfRawCallback);
  rf.initReceiver(RECEIVER_PIN);
  Serial.println("");
  Serial.print("Hostname: ");
  Serial.println(hostName);
}

void setup_wifi() {
  delay(10);
  beatLED.on();
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    beatLED.off();
    delay(500);
    beatLED.off();
    delay(500);
    beatLED.on();
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void transmitt(const String &protocol, const char *message) {
  Serial.print("rf send ");
  Serial.print(message);
  Serial.print(" with protocol ");
  Serial.println(protocol);

  if(protocol == "RAW") {
    int rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen = rf.stringToPulseTrain(message, rawpulses, MAXPULSESTREAMLENGTH);
    if(rawlen > 0) {
      rf.sendPulseTrain(rawpulses, rawlen);
    }
  } else {
    rf.send(protocol, message);
  }
}

void mqttCallback(const char* topic_, const byte* payload_, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic_);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload_[i]);
  }
  Serial.println();

  String topic = topic_;
  char payload[length+1];
  strncpy(payload, (char *)payload_, length);
  payload[length]='\0';

  String sendTopic = rfTopic + "/send/";
  if(topic.startsWith(sendTopic)) {
    transmitt(topic.substring(sendTopic.length()), payload);
  }
  sendTopic = hostName + "/send/";
  if(topic.startsWith(sendTopic)) {
    transmitt(topic.substring(sendTopic.length()), payload);
  }
  if(topic == (hostName + "/set/raw")) {
    Serial.println("Change raw mode.");
    if (payload[0] == '1') {
      rawMode = true;
    }
    else {
      rawMode = false;
    }
  }
  if(topic == (hostName + "/set/log")) {
    Serial.println("Change log mode.");
    if (payload[0] == '1') {
      logMode = true;
    }
    else {
      logMode = false;
    }
  }
  if(topic == (hostName + "/ota/url")) {
    otaURL = String(payload);
    mqtt.publish((hostName+ "/ota/nonce").c_str(), otaAuth.nonce().c_str());
  }
  if((topic == (hostName + "/ota/passwd")) && (otaURL.length() > 7)) {
    if(otaAuth.verify(payload)) {
      beatLED.on();
      Serial.print("Start OTA update from: ");
      Serial.println(otaURL);
      rf.disableReceiver();
      t_httpUpdate_return ret = ESPhttpUpdate.update(otaURL);
      switch(ret) {
        case HTTP_UPDATE_FAILED:
          Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;
        case HTTP_UPDATE_OK:
          Serial.println("HTTP_UPDATE_OK"); // may not called ESPhttpUpdate reboot the ESP?
          ESP.restart();
          break;
        }
        rf.enableReceiver();
    } else {
      Serial.println("OTA authentication failed!");
    }
  }
}

void rfCallback(const String &protocol, const String &message, int status, int repeats, const String &deviceID) {
  Serial.print("RF signal arrived [");
  Serial.print(protocol);
  Serial.print("]/[");
  Serial.print(deviceID);
  Serial.print("] (");
  Serial.print(status);
  Serial.print(") ");
  Serial.print(message);
  Serial.print(" (");
  Serial.print(repeats);
  Serial.println(")");

  if(status==VALID) {
    String topic = rfTopic + String("/recv/") + protocol;
    if (deviceID != NULL) {
      topic += String('/') + deviceID;
    }
    mqtt.publish(topic.c_str(), message.c_str(), true);
  }
  if(logMode) {
    String topic = hostName + String("/log/") + String(status) + String('/') + protocol;
    mqtt.publish(topic.c_str(), message.c_str());
  }
}

void rfRawCallback(const int* pulses, int length) {
  if(rawMode) {
    String data = rf.pulseTrainToString(pulses, length);
    if(data.length()>0) {
      Serial.print("RAW RF signal (");
      Serial.print(length);
      Serial.print("): ");
      Serial.print(data);
      Serial.println();

      mqtt.publish((hostName + "/recvRaw").c_str(), data.c_str());
    }
  }
}

void reconnect() {
  beatLED.on();
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(hostName.c_str(), hostName.c_str(), 0, true, "offline")) {
      Serial.println("connected");
      mqtt.publish(hostName.c_str(), "online", true);
      mqtt.subscribe((hostName + "/set/+").c_str());
      mqtt.subscribe((hostName + "/ota/+").c_str());
      mqtt.subscribe((hostName + "/send/+").c_str());
      mqtt.subscribe((rfTopic + "/send/+").c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      beatLED.off();
      delay(500);
      beatLED.on();
      delay(4500);
    }
  }
  beatLED.off();
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
  rf.loop();
  beatLED.loop();
}
