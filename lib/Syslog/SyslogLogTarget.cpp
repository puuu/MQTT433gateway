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

#include <algorithm>

#include "SyslogLogTarget.h"

size_t SyslogLogTarget::write(uint8_t byte) {
  buffer[position] = byte;
  position++;

  if (position >= SYSLOG_BUFFSIZE || byte == '\n') {
    send();
  }

  return 1;
}

size_t SyslogLogTarget::write(const uint8_t *incomming, size_t size) {
  size_t start = 0;

  while (size > 0) {
    size_t fit = std::min(size, SYSLOG_BUFFSIZE - position);

    bool newline = false;
    for (size_t i = 0; i < fit; ++i) {
      if (incomming[i + start] == '\n') {
        newline = true;
        fit = i + 1;
        break;
      }
    }

    memcpy(buffer + position, incomming + start, fit);
    position += fit;

    if (position >= SYSLOG_BUFFSIZE || newline) {
      send();
    }

    start += fit;
    size -= fit;
  }
}

void SyslogLogTarget::send() {
  buffer[position] = '\0';
  syslog.log(buffer);
  position = 0;
}

void SyslogLogTarget::begin(const String &name, const String &server,
                            uint16_t port) {
  syslog.server(server.c_str(), port);
  syslog.deviceHostname(name.c_str());
  syslog.appName("rfESP");
}
