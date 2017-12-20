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
#include <ArduinoSimpleLogging.h>
#include <WiFiUdp.h>

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
              const RfHandler* handler(getRfHandler());
              if (handler) {
                server.send(200, APPLICATION_JSON,
                            handler->availableProtocols());
              } else {
                server.send(200, APPLICATION_JSON, F("[]"));
              }
            }));

  server.on("/debug", HTTP_GET, authenticated([this]() {
              const RfHandler* handler(getRfHandler());

              if (!handler) {
                server.send(200, APPLICATION_JSON, F("{}"));
                return;
              }

              DynamicJsonBuffer buff;
              JsonObject& root = buff.createObject();
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

              if (parsed.containsKey(F("protocolRaw"))) {
                handler->setRawMode(parsed.get<bool>(F("protocolRaw")));
              }

              server.send(200, APPLICATION_JSON, F("true"));
            }));

  server.on("/firmware", HTTP_GET, authenticated([this]() {
              server.send(200, APPLICATION_JSON, F("{\"version\": \"TODO\"}"));
            }));

  server.on(
      "/firmware", HTTP_POST, authenticated([this]() {
        server.sendHeader("Connection", "close");

        Logger.info.println(F("Got an update. Reboot."));
        if (Update.hasError()) {
          server.send_P(
              200, TEXT_PLAIN,
              PSTR("Update failed. You may need to reflash the device."));
        } else {
          server.sendHeader("Refresh", F("20; URL=/"));
          server.send_P(200, TEXT_PLAIN,
                        PSTR("Update successful.\n\nDevice will reboot any try "
                             "to reconnect in 20 seconds."));
        }
        delay(500);
        ESP.restart();
      }),
      authenticated([this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();

          RfHandler* handler = getRfHandler();
          if (handler) {
            handler->disableReceiver();
          }

          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace =
              (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace)) {  // start with max available size
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
          if (Update.write(upload.buf, upload.currentSize) !=
              upload.currentSize) {
            Update.printError(Serial);
          }
        } else if (upload.status == UPLOAD_FILE_END) {
          if (Update.end(
                  true)) {  // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n",
                          upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
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
  password = settings.configPassword;
}

Print& ConfigWebServer::logTarget() { return wsLogTarget; }

ESP8266WebServer::THandlerFunction ConfigWebServer::authenticated(
    const ESP8266WebServer::THandlerFunction& handler) {
  return [=]() {
    if (!server.authenticate(ADMIN_USERNAME, this->password.c_str())) {
      server.requestAuthentication();
    } else {
      handler();
    }
  };
}
