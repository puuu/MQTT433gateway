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

#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H

#include <functional>

#define CAPTIVE_PW "rfESP_password"

/**
 * Connect to a network static or via teh connection manager.
 *
 * If the ssid is nullptr, the WiFiManager will open a SoftAP and start an
 * captive portal where the user can configure
 * the wifi to use.
 *
 * @param ssid The ssid in static mode.
 * @param passwd  The password for static mode.
 * @param waitCb  A callback that gets called peridically to indicate that the
 * device is alive.
 * @return true if connected successful.
 */
bool connectWifi(const std::function<void()> &waitCb);

void resetWifiConfig();

#endif  // WIFICONNECTION_H
