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

#ifndef RFHANDLER_H
#define RFHANDLER_H

#include <functional>

#include <WString.h>

#include <Settings.h>

class ESPiLight;

class RfHandler {
 public:
  using ReceiveCallback = std::function<void(const String &, const String &)>;

  RfHandler(const Settings &settings, const ReceiveCallback &sendCb);
  ~RfHandler();
  void begin();
  void loop();

  void transmitCode(const String &protocol, const String &message);
  void setRawMode(bool mode) { rawMode = mode; }
  bool isRawModeEnabled() const { return rawMode; }
  void enableReceiver();
  void disableReceiver();
  void setEchoEnabled(bool enabled);
  void filterProtocols(const String &protocols);
  String availableProtocols() const;

 private:
  void rfCallback(const String &protocol, const String &message, int status,
                  size_t repeats, const String &deviceID);
  void rfRawCallback(const uint16_t *pulses, size_t length);

  int8_t recieverPin;
  bool recieverPinPullUp;
  ReceiveCallback onReceiveCallback;
  bool rawMode = false;

  ESPiLight *rf;
};

#endif  // RFHANDLER_H
