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

#ifndef CONFIGWEBSERVER_H
#define CONFIGWEBSERVER_H

#define ADMIN_USERNAME "admin"

#include <algorithm>
#include <forward_list>

#include <ESP8266WebServer.h>

#include <RfHandler.h>
#include <Settings.h>

#include "WebSocketLogTarget.h"

class ConfigWebServer {
 public:
  using SystemCommandCb = std::function<void()>;
  using RfHandlerProviderCb = std::function<RfHandler*()>;

  ConfigWebServer() : server(80), wsLogTarget(81) {}

  void begin(Settings& settings);
  void updateSettings(const Settings& settings);

  void handleClient();

  void registerSystemCommandHandler(const String& command,
                                    const SystemCommandCb& cb);

  void registerRfHandlerProvider(const RfHandlerProviderCb& cb) {
    rfHandlerProvider = cb;
  }

  Print& logTarget();

 private:
  struct SystemCommandHandler {
    const String command;
    const ConfigWebServer::SystemCommandCb cb;

    SystemCommandHandler(const String& command,
                         const ConfigWebServer::SystemCommandCb& cb)
        : command(command), cb(cb) {}
  };

  ESP8266WebServer::THandlerFunction authenticated(
      const ESP8266WebServer::THandlerFunction& handler);
  void onSystemCommand();
  RfHandler* getRfHandler() {
    if (rfHandlerProvider) {
      return rfHandlerProvider();
    }
    return nullptr;
  }

  ESP8266WebServer server;
  WebSocketLogTarget wsLogTarget;
  std::forward_list<SystemCommandHandler> systemCommandHandlers;
  RfHandlerProviderCb rfHandlerProvider;

  String password;
};

#endif  // CONFIGWEBSERVER_H