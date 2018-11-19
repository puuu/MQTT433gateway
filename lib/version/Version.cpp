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

#include <Esp.h>

#include "Version.h"

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION unknown
#endif

#ifndef FW_BUILD_WITH
#define FW_BUILD_WITH \
  {}
#endif

#define X_QUOTE(x...) #x
#define QUOTE(x) X_QUOTE(x)

static const char PROGMEM fw_version[] = QUOTE(FIRMWARE_VERSION);
static const char PROGMEM fw_build_with[] = QUOTE(FW_BUILD_WITH);

String fwVersion() { return String(FPSTR(fw_version)); }

String chipId() { return String(ESP.getChipId(), HEX); }

String fwJsonVersion(bool withBuildIn) {
  return String(F("{\"version\":\"")) + fwVersion() + F("\",\"chipId\":\"") +
         chipId() + F("\"") +
         (withBuildIn ? (String(F(",\"build_with\":")) + FPSTR(fw_build_with))
                      : "") +
         F("}");
}
