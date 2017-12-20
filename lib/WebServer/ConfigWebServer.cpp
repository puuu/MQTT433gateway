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

// It won't compile if clang-format reorders these includes.
// clang-format off
#include <WString.h>
#include <ArduinoJson.h>
// clang-format on

#include <StringStream.h>

#include "../../dist/index.html.gz.h"
#include "ConfigWebServer.h"

const char TEXT_PLAIN[] PROGMEM = "text/plain";
const char APPLICATION_JSON[] = "application/json";

void ConfigWebServer::begin(Settings& settings) {
  updateSettings(settings);

  server.on("/", authenticated([this]() {
              server.sendHeader(F("Content-Encoding"), "gzip");
              server.sendHeader(F("Content-Length"), String(index_html_gz_len));
              server.setContentLength(CONTENT_LENGTH_UNKNOWN);
              server.send(200, F("text/html"), "");
              server.setContentLength(index_html_gz_len);
              server.sendContent_P(index_html_gz, index_html_gz_len);
              server.client().stop();
            }));

  server.on("/system", HTTP_GET, authenticated([this]() {
              server.send_P(200, TEXT_PLAIN, PSTR("POST your commands here"));
            }));

  server.on(
      "/system", HTTP_POST,
      authenticated(std::bind(&::ConfigWebServer::onSystemCommand, this)));

  server.on("/config", HTTP_GET, authenticated([&]() {
              String buff;
              StringStream stream(buff);
              settings.serialize(stream, true, false);
              server.send(200, APPLICATION_JSON, buff);
            }));

  server.on("/config", HTTP_PUT, authenticated([&]() {
              settings.deserialize(server.arg("plain"), true);
              settings.save();
              server.send(200, APPLICATION_JSON, F("true"));
            }));

  server.on("/protocols", HTTP_GET, authenticated([this]() {
              const RfHandler* handler(rfHandlerProvider());
              if (handler) {
                server.send(200, APPLICATION_JSON,
                            handler->availableProtocols());
              } else {
                server.send(200, APPLICATION_JSON, F("[]"));
              }
            }));

  server.on("/debug", HTTP_GET, authenticated([this]() {
              const RfHandler* handler(rfHandlerProvider());

              if (!handler) {
                server.send(200, APPLICATION_JSON, F("{}"));
                return;
              }

              DynamicJsonBuffer buff;
              JsonObject& root = buff.createObject();
              root[F("protocolLog")] = handler->isLogModeEnabled();
              root[F("protocolRaw")] = handler->isRawModeEnabled();

              String result;
              root.printTo(result);

              server.send(200, APPLICATION_JSON, result);
            }));

  server.on("/debug", HTTP_PUT, authenticated([this]() {
              RfHandler* handler(rfHandlerProvider());

              if (!handler) {
                server.send(500, APPLICATION_JSON, F("false"));
                return;
              }

              DynamicJsonBuffer buff;
              JsonObject& parsed = buff.parse(server.arg("plain"));

              if (!parsed.success()) {
                server.send(500, APPLICATION_JSON, "false");
                return;
              }

              if (parsed.containsKey(F("protocolLog"))) {
                handler->setLogMode(parsed.get<bool>(F("protocolLog")));
              }
              if (parsed.containsKey(F("protocolRaw"))) {
                handler->setRawMode(parsed.get<bool>(F("protocolRaw")));
              }
            }));

  wsLogTarget.begin();
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

  for (const auto& systemCommandHandler : systemCommandHandlers) {
    if (systemCommandHandler.command == request["command"]) {
      server.send_P(200, TEXT_PLAIN, PSTR("Run command!"));
      systemCommandHandler.cb();
      return;
    }
  }

  server.send_P(400, TEXT_PLAIN, PSTR("Unknown command"));
}

void ConfigWebServer::handleClient() {
  wsLogTarget.loop();
  server.handleClient();
}

void ConfigWebServer::updateSettings(const Settings& settings) {
  user = settings.configUser;
  password = settings.configPassword;
}

Print& ConfigWebServer::logTarget() { return wsLogTarget; }

ESP8266WebServer::THandlerFunction ConfigWebServer::authenticated(
    const ESP8266WebServer::THandlerFunction& handler) {
  return [=]() {
    if (!server.authenticate(this->user.c_str(), this->password.c_str())) {
      server.requestAuthentication();
    } else {
      handler();
    }
  };
}
