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

#ifndef SYSLOGLOGTARGET_H
#define SYSLOGLOGTARGET_H

#define SYSLOG_BUFFSIZE 100

#include <Print.h>
#include <Syslog.h>
#include <WiFiUdp.h>

class SyslogLogTarget : public Print {
 public:
  SyslogLogTarget() : udp(), syslog(udp), position(0) {}

  void begin(const String& name, const String& server, uint16_t port);

  size_t write(uint8_t byte) override;
  size_t write(const uint8_t* buffer, size_t size) override;

 private:
  void send();

  WiFiUDP udp;
  Syslog syslog;
  char buffer[SYSLOG_BUFFSIZE + 1];
  size_t position;
};

#endif  // SYSLOGLOGTARGET_H
