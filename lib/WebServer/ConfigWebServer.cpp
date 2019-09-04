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

#include <WString.h>
#ifndef ESP8266
#include <Update.h>
#endif

#include <ArduinoJson.h>
#include <ArduinoSimpleLogging.h>

#include <Version.h>

#include "../../dist/index.html.gz.h"
#include "ConfigWebServer.h"

const char PROGMEM PLAIN[] = "plain";
const char PROGMEM TEXT_PLAIN[] = "text/plain";
const char PROGMEM TEXT_HTML[] = "text/html";
const char PROGMEM APPLICATION_JSON[] = "application/json";

const char PROGMEM URL_ROOT[] = "/";
const char PROGMEM URL_SYSTEM[] = "/system";
const char PROGMEM URL_CONFIG[] = "/config";
const char PROGMEM URL_PROTOCOLS[] = "/protocols";
const char PROGMEM URL_DEBUG[] = "/debug";
const char PROGMEM URL_FIRMWARE[] = "/firmware";

void ConfigWebServer::begin() {
  server.on(FPSTR(URL_ROOT), authenticated([this]() {
              Logger.debug.println(F("Webserver: frontend request"));
              server.sendHeader(F("Content-Encoding"), F("gzip"));
              server.setContentLength(index_html_gz_len);
              server.send(200, FPSTR(TEXT_HTML), "");
              server.sendContent_P(index_html_gz, index_html_gz_len);
            }));

  server.on(FPSTR(URL_SYSTEM), HTTP_GET, authenticated([this]() {
              Logger.debug.println(F("Webserver: system GET"));
              server.send_P(200, TEXT_PLAIN, PSTR("POST your commands here"));
            }));

  server.on(
      FPSTR(URL_SYSTEM), HTTP_POST,
      authenticated(std::bind(&::ConfigWebServer::onSystemCommand, this)));

  server.on(FPSTR(URL_CONFIG), HTTP_GET, authenticated([&]() {
              Logger.debug.println(F("Webserver: config GET"));
              onConfigGet();
            }));

  server.on(FPSTR(URL_CONFIG), HTTP_PUT, authenticated([&]() {
              Logger.debug.println(F("Webserver: config PUT"));
              settings.deserialize(server.arg(FPSTR(PLAIN)));
              settings.save();
              onConfigGet();
            }));

  server.on(FPSTR(URL_PROTOCOLS), HTTP_GET, authenticated([this]() {
              Logger.debug.println(F("Webserver: protocols GET"));
              if (protocolProvider) {
                server.send(200, FPSTR(APPLICATION_JSON), protocolProvider());
              } else {
                Logger.warning.println(F("No protocolProvider avaiable."));
                server.send_P(200, APPLICATION_JSON, PSTR("[]"));
              }
            }));

  server.on(FPSTR(URL_DEBUG), HTTP_GET, authenticated([&]() {
              Logger.debug.println(F("Webserver: debug GET"));
              onDebugFlagGet();
            }));

  server.on(FPSTR(URL_DEBUG), HTTP_PUT,
            authenticated(std::bind(&::ConfigWebServer::onDebugFlagSet, this)));

  server.on(FPSTR(URL_FIRMWARE), HTTP_GET, authenticated([this]() {
              Logger.debug.println(F("Webserver: firmware GET"));
              server.send(200, FPSTR(APPLICATION_JSON), fwJsonVersion(true));
            }));

  server.on(
      FPSTR(URL_FIRMWARE), HTTP_POST,
      authenticated(std::bind(&::ConfigWebServer::onFirmwareFinish, this)),
      std::bind(&::ConfigWebServer::onFirmwareUpload, this));

  Logger.debug.println(F("Starting webserver and websocket server."));
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

void ConfigWebServer::onConfigGet() {
  String buff;
  settings.serialize(buff, true, false);
  server.send(200, FPSTR(APPLICATION_JSON), buff);
}

void ConfigWebServer::onSystemCommand() {
  Logger.debug.println(F("Webserver: system POST"));
  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error =
      deserializeJson(jsonDoc, server.arg(FPSTR(PLAIN)));

  if (error) {
    server.send_P(400, TEXT_PLAIN, PSTR("Cannot parse command!"));
    return;
  }

  const char* command = jsonDoc[F("command")];

  if (!command) {
    server.send_P(400, TEXT_PLAIN, PSTR("No command found!"));
    return;
  }

  for (const auto& systemCommandHandler : systemCommandHandlers) {
    if (systemCommandHandler.command == command) {
      server.send_P(200, TEXT_PLAIN, PSTR("Run command!"));
      systemCommandHandler.cb();
      return;
    }
  }

  server.send_P(400, TEXT_PLAIN, PSTR("Unknown command"));
}

void ConfigWebServer::onDebugFlagSet() {
  Logger.debug.println(F("Webserver: debug PUT"));
  DynamicJsonDocument jsonDoc(1024);
  DeserializationError error =
      deserializeJson(jsonDoc, server.arg(FPSTR(PLAIN)));

  if (!error) {
    for (const auto& debugFlagHandler : debugFlagHandlers) {
      JsonVariant value = jsonDoc[debugFlagHandler.name];
      if (!value.isNull()) {
        debugFlagHandler.setState(value.as<bool>());
        Logger.debug.print(F("Set debug flag "));
        Logger.debug.print(debugFlagHandler.name);
        Logger.debug.print(F(": "));
        Logger.debug.println(value.as<bool>());
      }
    }
  } else {
    Logger.error.println(F("Cannot parse debug flag as json object!"));
  }
  onDebugFlagGet();
}

void ConfigWebServer::onDebugFlagGet() {
  DynamicJsonDocument jsonDoc(1024);

  for (const auto& debugFlagHandler : debugFlagHandlers) {
    jsonDoc[debugFlagHandler.name] = debugFlagHandler.getState();
  }
  String result;
  serializeJson(jsonDoc, result);
  server.send(200, FPSTR(APPLICATION_JSON), result);
}

void ConfigWebServer::onFirmwareFinish() {
  server.sendHeader(F("Connection"), F("close"));

  Logger.info.println(F("Got an update. Rebooting..."));
  if (Update.hasError()) {
    server.send_P(
        200, TEXT_PLAIN,
        PSTR("Update failed. More information can be found on the serial "
             "console. \n\nDevice will reboot with old firmware. Please "
             "reconnect and try to flash again."));
  } else {
    server.sendHeader(F("Refresh"), F("20; URL=/"));
    server.send_P(200, TEXT_PLAIN,
                  PSTR("Update successful.\n\nDevice will reboot and try "
                       "to reconnect in 20 seconds."));
  }
  delay(500);
  ESP.restart();
}

void ConfigWebServer::onFirmwareUpload() {
  static bool authenticate = false;
  static bool error = false;
  HTTPUpload& upload = server.upload();
  wsLogTarget.loop();

  if (upload.status == UPLOAD_FILE_START) {
    authenticate = server.authenticate(ADMIN_USERNAME,
                                       this->settings.configPassword.c_str());
    error = false;
    if (!authenticate) {
      return;
    }
    Logger.info.println(F("Webserver: firmware upload started"));
    Serial.setDebugOutput(true);

    if (otaHook) {
      otaHook();
    }

    Logger.debug.print(F("Update: "));
    Logger.debug.println(upload.filename.c_str());
    Logger.debug.print(F("Free heap: "));
    Logger.debug.println(ESP.getFreeHeap());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {  // start with max available size
      Update.printError(Logger.info);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && authenticate && !error) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      error = true;
      Update.printError(Logger.info);
    }
  } else if (upload.status == UPLOAD_FILE_END && authenticate && !error) {
    if (Update.end(true)) {  // true to set the size to the current progress
      Logger.debug.print(F("Update Success: "));
      Logger.debug.println(upload.totalSize);
    } else {
      Update.printError(Logger.info);
    }
    Serial.setDebugOutput(false);
  } else if (upload.status == UPLOAD_FILE_ABORTED && authenticate) {
    Update.end();
    Logger.warning.print(F("Update was aborted!"));
  }
  delay(0);
}

void ConfigWebServer::loop() {
  wsLogTarget.loop();
  server.handleClient();
}

Print& ConfigWebServer::logTarget() { return wsLogTarget; }

WebServer::THandlerFunction ConfigWebServer::authenticated(
    const WebServer::THandlerFunction& handler) {
  return [=]() {
    if (!server.authenticate(ADMIN_USERNAME,
                             this->settings.configPassword.c_str())) {
      server.requestAuthentication(DIGEST_AUTH, nullptr,
                                   F("Authentication required!"));
      Logger.warning.println(F("Webserver: Authentication failed."));
    } else {
      handler();
    }
  };
}
