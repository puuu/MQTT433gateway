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

#include "WebSocketLogTarget.h"

void WebSocketLogTarget::begin() {
  using namespace std::placeholders;

  server.onEvent(
      std::bind(&WebSocketLogTarget::handleEvent, this, _1, _2, _3, _4));
  server.begin();
}

void WebSocketLogTarget::handleEvent(uint8_t num, WStype_t type,
                                     uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      break;
    case WStype_CONNECTED: {
      String reply(F("*** Connection established ***\n"));
      server.sendTXT(num, reply);
    } break;
    case WStype_TEXT:
      // We do not expect any data except the __PING__ requests from the
      // frontend. So we can save the cycles to check the message and just send
      // the response.
      server.sendTXT(num, "__PONG__");
      break;
    default:
      break;
  }
}

void WebSocketLogTarget::flush(const char *data) {
  server.broadcastTXT(data, size());
}
