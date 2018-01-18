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

#include <ESP8266WebServer.h>
#include <WString.h>

#include <ArduinoJson.h>
#include <ArduinoSimpleLogging.h>

#include "../../dist/index.html.gz.h"
#include "ConfigWebServer.h"

const char TEXT_PLAIN[] PROGMEM = "text/plain";
const char APPLICATION_JSON[] = "application/json";

void ConfigWebServer::begin(Settings& settings) {
  updateSettings(settings);

  server.on(F("/"), authenticated([this]() {
              server.sendHeader(F("Content-Encoding"), "gzip");
              server.setContentLength(index_html_gz_len);
              server.send(200, F("text/html"), "");
              server.sendContent_P(index_html_gz, index_html_gz_len);
            }));

  server.on(F("/system"), HTTP_GET, authenticated([this]() {
              server.send_P(200, TEXT_PLAIN, PSTR("POST your commands here"));
            }));

  server.on(
      F("/system"), HTTP_POST,
      authenticated(std::bind(&::ConfigWebServer::onSystemCommand, this)));

  server.on(F("/config"), HTTP_GET, authenticated([&]() {
              String buff;
              settings.serialize(buff, true, false);
              server.send(200, APPLICATION_JSON, buff);
            }));

  server.on(F("/config"), HTTP_PUT, authenticated([&]() {
              settings.deserialize(server.arg("plain"));
              settings.save();
              server.send(200, APPLICATION_JSON, F("true"));
            }));

  server.on("/protocols", HTTP_GET, authenticated([this]() {
              if (protocolProvider) {
                server.send(200, APPLICATION_JSON, protocolProvider());
              } else {
                server.send(200, APPLICATION_JSON, F("[]"));
              }
            }));

  server.on("/debug", HTTP_GET,
            authenticated(std::bind(&::ConfigWebServer::onDebugFlagGet, this)));

  server.on("/debug", HTTP_PUT,
            authenticated(std::bind(&::ConfigWebServer::onDebugFlagSet, this)));

  server.on(F("/firmware"), HTTP_GET, authenticated([this]() {
              server.send_P(
                  200, APPLICATION_JSON,
                  PSTR("{\"version\": \"" QUOTE(FIRMWARE_VERSION) "\"}"));
            }));

  server.on(
      F("/firmware"), HTTP_POST, authenticated([this]() {
        server.sendHeader(F("Connection"), F("close"));

        Logger.info.println(F("Got an update. Reboot."));
        if (Update.hasError()) {
          server.send_P(
              200, TEXT_PLAIN,
              PSTR("Update failed. You may need to reflash the device."));
        } else {
          server.sendHeader(F("Refresh"), F("20; URL=/"));
          server.send_P(200, TEXT_PLAIN,
                        PSTR("Update successful.\n\nDevice will reboot and try "
                             "to reconnect in 20 seconds."));
        }
        delay(500);
        ESP.restart();
      }),
      authenticated([this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
          Serial.setDebugOutput(true);

          if (otaHook) {
            otaHook();
          }

          Serial.print(F("Update: "));
          Serial.println(upload.filename.c_str());
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
            Serial.print(F("Update Success: "));
            Serial.print(upload.totalSize);
            Serial.print(F("\nRebooting...\n"));
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

void ConfigWebServer::registerDebugFlagHandler(
    const String& state, const ConfigWebServer::DebugFlagGetCb& getState,
    const ConfigWebServer::DebugFlagSetCb& setState) {
  debugFlagHandlers.emplace_front(state, getState, setState);
}

void ConfigWebServer::onSystemCommand() {
  DynamicJsonBuffer buffer;
  JsonObject& request = buffer.parse(server.arg(F("plain")));

  if (!request.success()) {
    server.send_P(400, TEXT_PLAIN, PSTR("Cannot parse command!"));
    return;
  }

  if (!request.containsKey(F("command"))) {
    server.send_P(400, TEXT_PLAIN, PSTR("No command found!"));
    return;
  }

  for (const auto& systemCommandHandler : systemCommandHandlers) {
    if (systemCommandHandler.command == request[F("command")]) {
      server.send_P(200, TEXT_PLAIN, PSTR("Run command!"));
      systemCommandHandler.cb();
      return;
    }
  }

  server.send_P(400, TEXT_PLAIN, PSTR("Unknown command"));
}

void ConfigWebServer::onDebugFlagSet() {
  DynamicJsonBuffer buffer;
  JsonObject& request = buffer.parse(server.arg(F("plain")));

  if (!request.success()) {
    server.send_P(400, TEXT_PLAIN, PSTR("Cannot parse json object!"));
    return;
  }

  for (const auto& debugFlagHandler : debugFlagHandlers) {
    if (request.containsKey(debugFlagHandler.name)) {
      debugFlagHandler.setState(request[debugFlagHandler.name].as<bool>());
      Logger.debug.print(F("Set debug flag "));
      Logger.debug.print(debugFlagHandler.name);
      Logger.debug.print(F(": "));
      Logger.debug.println(request[debugFlagHandler.name].as<bool>());
    }
  }
  onDebugFlagGet();
}

void ConfigWebServer::onDebugFlagGet() {
  DynamicJsonBuffer buffer;
  JsonObject& root = buffer.createObject();

  for (const auto& debugFlagHandler : debugFlagHandlers) {
    root[debugFlagHandler.name] = debugFlagHandler.getState();
  }
  String result;
  root.printTo(result);
  server.send(200, APPLICATION_JSON, result);
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
      server.sendHeader(F("WWW-Authenticate"),
                        F("Basic realm=\"Login Required\""));
      server.send_P(401, TEXT_PLAIN, PSTR("Authentication required!"));
    } else {
      handler();
    }
  };
}
