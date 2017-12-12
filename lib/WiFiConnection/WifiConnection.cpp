/*
  WifiConnection - Helper for connecting to a wifi network
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

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <WiFiManager.h>

#include <ArduinoSimpleLogging.h>

#include "WifiConnection.h"

WiFiManager wifiManager;

bool setupWifiStatic(const char *ssid, const char *passwd,
                     std::function<void()> const &waitCb) {
  delay(10);
  waitCb();
  // We start by connecting to a WiFi network
  Logger.info.println();
  Logger.info.print(F("Connecting to "));
  Logger.info.println(ssid);

  WiFi.mode(WIFI_STA);  // Explicitly set station mode
  WiFi.begin(ssid, passwd);

  uint16_t counter = 0;

  while (WiFi.status() != WL_CONNECTED) {
    waitCb();
    delay(500);
    waitCb();
    delay(500);
    waitCb();
    Logger.info.print(F("."));
    if (counter > 180) {
      Logger.error.println();
      Logger.error.println(F("Could not connect to wifi in 180 seconds!"));
      return false;
    }
    ++counter;
  }

  Logger.info.println();
  Logger.info.println(F("WiFi connected"));
  Logger.info.println(F("IP address: "));
  Logger.info.println(WiFi.localIP());
  waitCb();
  return true;
}

bool connectWifi(const char *ssid, const char *passwd,
                 const std::function<void()> &waitCb) {
  if (ssid != nullptr) {
    return setupWifiStatic(ssid, passwd, waitCb);
  } else {
    String portal_ssid("RfEsp_" + String(ESP.getChipId(), HEX));
    wifiManager.setConfigPortalTimeout(180);
    return wifiManager.autoConnect(portal_ssid.c_str(), CAPTIVE_PW);
  }
}

void resetWifiConfig() { wifiManager.resetSettings(); }