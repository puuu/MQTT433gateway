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

#ifndef WEBSOCKETLOGTARGET_H
#define WEBSOCKETLOGTARGET_H

#include <Print.h>

#include <LineBufferProxy.h>
#include <WebSocketsServer.h>

class WebSocketLogTarget : public LineBufferProxy<64> {
 public:
  explicit WebSocketLogTarget(const uint16_t port)
      : server(WebSocketsServer(port)) {}

  void loop() { server.loop(); }
  void begin();

  void handleEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

 protected:
  void flush(const char* data) override;

 private:
  WebSocketsServer server;
};

#endif  // WEBSOCKETLOGTARGET_H
