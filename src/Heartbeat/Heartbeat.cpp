/*
  Heartbeat - Library for flashing LED
  Project home: https://github.com/puuu/MQTT433gateway/

  The MIT License (MIT)

  Copyright (c) 2016 Puuu

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

#include "Heartbeat.h"

Heartbeat::Heartbeat(int pin, int interval) {
  pinMode(pin, OUTPUT);
  _pin = pin;
  _interval = interval;
  off();
}

void Heartbeat::on() {
  _tick = 1;
  _last = millis();
  _state = true;
  digitalWrite(_pin, !_state); //active low
}

void Heartbeat::off() {
  _tick = 0;
  _last = millis();
  _state = false;
  digitalWrite(_pin, !_state); //active low
}

void Heartbeat::loop() {
  unsigned long now = millis();
  if( (_last - now) >= _interval ) {
    beatStep();
    _last = now;
  };
}

void Heartbeat::beatStep() {
  if (_tick <= 3) {
    _state = ! _state;
    digitalWrite(_pin, !_state); //active low
  }
  _tick = (_tick + 1) % 10;
}
