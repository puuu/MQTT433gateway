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

#include <debug_helper.h>

#include "RfHandler.h"

// we need these because the ESPiLight cannot handle member function pointers.
static RfHandler *_instance = nullptr;

static void _rfCallback(const String &protocol, const String &message,
                        int status, int repeats, const String &deviceID) {
  _instance->rfCallback(protocol, message, status, repeats, deviceID);
}

static void _rfRawCallback(const uint16_t *pulses, int length) {
  _instance->rfRawCallback(pulses, length);
}

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
  Debug(F("rf send "));
  Debug(message);
  Debug(F(" with protocol "));
  DebugLn(protocol);

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
                           int status, int repeats, const String &deviceID) {
  Debug(F("RF signal arrived ["));
  Debug(protocol);
  Debug(F("]/["));
  Debug(deviceID);
  Debug(F("] ("));
  Debug(status);
  Debug(F(") "));
  Debug(message);
  Debug(F(" ("));
  Debug(repeats);
  DebugLn(F(")"));

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

void RfHandler::rfRawCallback(const uint16_t *pulses, int length) {
  if (rawMode) {
    String data = rf->pulseTrainToString(pulses, length);
    if (data.length() > 0) {
      Debug(F("RAW RF signal ("));
      Debug(length);
      Debug(F("): "));
      Debug(data);
      DebugLn();
      rawCb(data);
    }
  }
}

void RfHandler::begin() {
  if (0 < recieverPin) {
    using namespace std::placeholders;

    pinMode(RECEIVER_PIN,
            INPUT_PULLUP);  // 5V protection with reverse diode needs pullup
    _instance = this;
    rf->setCallback(_rfCallback);
    rf->setPulseTrainCallBack(_rfRawCallback);
    rf->initReceiver(RECEIVER_PIN);
  }
}

void RfHandler::enableReceiver() { rf->enableReceiver(); }

void RfHandler::disableReceiver() { rf->disableReceiver(); }

void RfHandler::setEchoEnabled(bool enabled) { rf->setEchoEnabled(enabled); }

void RfHandler::filterProtocols(const String &protocols) {
  rf->limitProtocols(protocols);
}

String RfHandler::availableProtocols() { return rf->availableProtocols(); }

void RfHandler::loop() { rf->loop(); }
