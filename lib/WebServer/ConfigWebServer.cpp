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

#include <ArduinoJson.h>

#include "ConfigWebServer.h"

const char TEXT_PLAIN[] PROGMEM = "text/plain";
const char APPLICATION_JSON[] = "application/json";

void ConfigWebServer::begin(const Settings& settings) {
  server.on("/",
            [this]() { server.send(200, TEXT_PLAIN, F("Hello from rfESP")); });

  server.on("/", [this]() {
    server.send(200, TEXT_PLAIN, F("POST your commands here"));
  });

  server.on("/system", HTTP_POST,
            std::bind(&::ConfigWebServer::onSystemCommand, this));

  server.begin();
}

void ConfigWebServer::registerSystemCommandHandler(
    const String& command, const ConfigWebServer::SystemCommandCb& cb) {
  systemCommandHandlers.emplace_front(command, cb);
}

void ConfigWebServer::onSystemCommand() {
  DynamicJsonBuffer buffer;
  JsonObject& request = buffer.parse(server.arg("plain"));

  if (!request.success()) {
    server.send_P(400, TEXT_PLAIN, PSTR("Cannot parse command!"));
    return;
  }

  if (!request.containsKey("command")) {
    server.send_P(400, TEXT_PLAIN, PSTR("No command found!"));
    return;
  }

  for (auto cur = systemCommandHandlers.begin();
       cur != systemCommandHandlers.end(); ++cur) {
    if (cur->command == request["command"]) {
      server.send_P(200, TEXT_PLAIN, PSTR("Run command!"));
      cur->cb();
      return;
    }
  }

  server.send_P(400, TEXT_PLAIN, PSTR("Unknown command"));
}

void ConfigWebServer::handleClient() { server.handleClient(); }

void ConfigWebServer::updateSettings(const Settings& settings) {}
