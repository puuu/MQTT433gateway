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

#include "Settings.h"

struct SettingListener {
  const SettingType type;
  const Settings::SettingCallbackFn callback;
  SettingListener *const next;

  SettingListener(const SettingType &type,
                  const Settings::SettingCallbackFn &cb,
                  SettingListener *const next)
      : type(type), callback(cb), next(next) {}
};

void Settings::onChange(const SettingType setting,
                        const SettingCallbackFn &callback) {
  listeners = new SettingListener(setting, callback, listeners);
}

void Settings::fireChange(const SettingType type) const {
  SettingListener *current = listeners;
  while (current != nullptr) {
    if (type == current->type) {
      current->callback(*this);
    }
    current = current->next;
  }
}

void Settings::load() {
  // ToDo load

  fireChange(MQTT);
  fireChange(RF_ECHO);
  fireChange(RF_PROTOCOL);
  fireChange(OTA);
}

Settings::~Settings() {
  SettingListener *current = listeners;
  while (current != nullptr) {
    SettingListener *tmp = current;
    current = current->next;
    delete tmp;
  }
}

void Settings::updateProtocols(const String &protocols) {
  this->rfProtocols = protocols;
  fireChange(RF_PROTOCOL);
}

void Settings::updateOtaUrl(const String &otaUrl) { this->otaUrl = otaUrl; }
