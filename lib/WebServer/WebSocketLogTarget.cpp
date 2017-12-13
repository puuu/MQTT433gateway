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
      if (clientCount > 0) {
        clientCount--;
      }
      break;

    case WStype_CONNECTED:
      clientCount++;
      break;
    default:
      break;
  }
}

size_t WebSocketLogTarget::write(const uint8_t *buffer, size_t size) {
  if (clientCount) {
    server.broadcastTXT(buffer, size);
  }
  return size;
}

size_t WebSocketLogTarget::write(uint8_t byte) {
  if (clientCount) {
    server.broadcastTXT(&byte, 1);
  }
  return 1;
}
