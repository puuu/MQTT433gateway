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

#include <ESPiLight.h>

#include <ArduinoSimpleLogging.h>

#include "RfHandler.h"

RfHandler::RfHandler(const Settings &settings,
                     const RfHandler::SendCallback &sendCb,
                     const RfHandler::LogCallback &logCb,
                     const RfHandler::RawCallback &rawCb)
    : rf(new ESPiLight(settings.rfTransmitterPin)),
      recieverPin(settings.rfReceiverPin),
      sendCb(sendCb),
      logCb(logCb),
      rawCb(rawCb) {
  rf->setEchoEnabled(settings.rfEchoMessages);
}

void RfHandler::transmitCode(const String &protocol, const String &message) {
  Logger.debug.print(F("rf send "));
  Logger.debug.print(message);
  Logger.debug.print(F(" with protocol "));
  Logger.debug.println(protocol);

  if (protocol == F("RAW")) {
    uint16_t rawpulses[MAXPULSESTREAMLENGTH];
    int rawlen =
        rf->stringToPulseTrain(message, rawpulses, MAXPULSESTREAMLENGTH);
    if (rawlen > 0) {
      rf->sendPulseTrain(rawpulses, rawlen);
    }
  } else {
    rf->send(protocol, message);
  }
}

void RfHandler::rfCallback(const String &protocol, const String &message,
                           int status, size_t repeats, const String &deviceID) {
  Logger.debug.print(F("RF signal arrived ["));
  Logger.debug.print(protocol);
  Logger.debug.print(F("]/["));
  Logger.debug.print(deviceID);
  Logger.debug.print(F("] ("));
  Logger.debug.print(status);
  Logger.debug.print(F(") "));
  Logger.debug.print(message);
  Logger.debug.print(F(" ("));
  Logger.debug.print(repeats);
  Logger.debug.println(F(")"));

  if (status == VALID) {
    if (deviceID != nullptr) {
      sendCb(protocol + "/" + deviceID, message);
    }
    sendCb(protocol, message);
  }
  if (logMode) {
    logCb(status, protocol, message);
  }
}

void RfHandler::rfRawCallback(const uint16_t *pulses, size_t length) {
  if (rawMode) {
    String data = rf->pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Logger.debug.print(F("RAW RF signal ("));
      Logger.debug.print(length);
      Logger.debug.print(F("): "));
      Logger.debug.print(data);
      Logger.debug.println();
      rawCb(data);
    }
  }
}

void RfHandler::begin() {
  if (0 < recieverPin) {
    using namespace std::placeholders;

    pinMode(RECEIVER_PIN,
            INPUT_PULLUP);  // 5V protection with reverse diode needs pullup
    rf->setCallback(
        std::bind(&RfHandler::rfCallback, this, _1, _2, _3, _4, _5));
    rf->setPulseTrainCallBack(
        std::bind(&RfHandler::rfRawCallback, this, _1, _2));
    rf->initReceiver(RECEIVER_PIN);
  }
}

void RfHandler::enableReceiver() { rf->enableReceiver(); }

void RfHandler::disableReceiver() { rf->disableReceiver(); }

void RfHandler::setEchoEnabled(bool enabled) { rf->setEchoEnabled(enabled); }

void RfHandler::filterProtocols(const String &protocols) {
  rf->limitProtocols(protocols);
}

String RfHandler::availableProtocols() const {
  return rf->availableProtocols();
}

void RfHandler::loop() { rf->loop(); }
