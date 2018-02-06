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
#include <ArduinoSimpleLogging.h>

#include "RfHandler.h"

RfHandler::RfHandler(const Settings &settings)
    : settings(settings), rf(settings.rfTransmitterPin) {
  rf.setErrorOutput(Logger.error);
}

RfHandler::~RfHandler() { rf.initReceiver(-1); }

void RfHandler::transmitCode(const String &protocol, const String &message) {
  int result = 0;

  Logger.info.print(F("transmit rf signal "));
  Logger.info.print(message);
  Logger.info.print(F(" with protocol "));
  Logger.info.println(protocol);

  if (protocol == F("RAW")) {
    uint16_t rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen =
        rf.stringToPulseTrain(message, rawpulses, MAXPULSESTREAMLENGTH);
    if (rawlen > 0) {
      rf.sendPulseTrain(rawpulses, rawlen);
      result = rawlen;
    } else {
      result = -9999;
    }
  } else {
    result = rf.send(protocol, message);
  }

  if (result > 0) {
    Logger.debug.print(F("transmitted pulse train with "));
    Logger.debug.print(result);
    Logger.debug.println(F(" pulses"));
  } else {
    Logger.error.print(F("transmitting failed: "));
    switch (result) {
      case ESPiLight::ERROR_UNAVAILABLE_PROTOCOL:
        Logger.error.println(F("protocol is not avaiable"));
        break;
      case ESPiLight::ERROR_INVALID_PILIGHT_MSG:
        Logger.error.println(F("message is invalid"));
        break;
      case ESPiLight::ERROR_INVALID_JSON:
        Logger.error.println(F("message is not a proper json object"));
        break;
      case ESPiLight::ERROR_NO_OUTPUT_PIN:
        Logger.error.println(F("no transmitter pin"));
        break;
      case -9999:
        Logger.error.println(F("invalid pulse train message"));
        break;
    }
  }
}

void RfHandler::onRfCode(const String &protocol, const String &message,
                         int status, size_t repeats, const String &deviceID) {
  if (!onReceiveCallback) return;

  if (status == VALID) {
    Logger.info.print(F("rf signal received: "));
    Logger.info.print(message);
    Logger.info.print(F(" with protocol "));
    Logger.info.print(protocol);
    if (deviceID != nullptr) {
      Logger.info.print(F(" deviceID="));
      Logger.info.println(deviceID);
      onReceiveCallback(protocol + "/" + deviceID, message);
    } else {
      Logger.info.println(deviceID);
      onReceiveCallback(protocol, message);
    }
  } else {
    Logger.debug.print(F("rf signal received: "));
    Logger.debug.print(message);
    Logger.debug.print(F(" protocol= "));
    Logger.debug.print(protocol);
    Logger.debug.print(F(" status="));
    Logger.debug.print(status);
    Logger.debug.print(F(" repeats="));
    Logger.debug.print(repeats);
    Logger.debug.print(F(" deviceID="));
    Logger.debug.println(deviceID);
  }
}

void RfHandler::onRfRaw(const uint16_t *pulses, size_t length) {
  if (rawMode) {
    String data = rf.pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Logger.info.print(F("RAW RF signal ("));
      Logger.info.print(length);
      Logger.info.print(F("): "));
      Logger.info.println(data);
    }
  }
}

void RfHandler::begin() {
  if (0 < settings.rfReceiverPin) {
    using namespace std::placeholders;
    if (settings.rfReceiverPinPullUp) {
      // 5V protection with reverse diode needs pullup
      pinMode(settings.rfReceiverPin, INPUT_PULLUP);
    }
    rf.setCallback(std::bind(&RfHandler::onRfCode, this, _1, _2, _3, _4, _5));
    rf.setPulseTrainCallBack(std::bind(&RfHandler::onRfRaw, this, _1, _2));
    rf.initReceiver(settings.rfReceiverPin);
  }
}

void RfHandler::enableReceiver() { rf.enableReceiver(); }

void RfHandler::disableReceiver() { rf.disableReceiver(); }

void RfHandler::setEchoEnabled(bool enabled) { rf.setEchoEnabled(enabled); }

void RfHandler::filterProtocols(const String &protocols) {
  rf.limitProtocols(protocols);
}

String RfHandler::availableProtocols() {
  return ESPiLight::availableProtocols();
}

void RfHandler::loop() { rf.loop(); }

void RfHandler::registerReceiveHandler(const ReceiveCb &cb) {
  onReceiveCallback = cb;
}
