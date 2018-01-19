/*
  HeartbeatFlashing - Library for flashing LED
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

#include <Arduino.h>

#include "HeartbeatFlashing.h"


void _flash_tick(LED* led) { led->toggle(); }

HeartbeatFlashing::HeartbeatFlashing(LED& led, int interval)
    : Heartbeat(led, interval) {
  _flashing = false;
}

HeartbeatFlashing::HeartbeatFlashing(int pin, int interval)
    : Heartbeat(pin, interval) {
  _flashing = false;
}

HeartbeatFlashing::~HeartbeatFlashing() { off(); }

void HeartbeatFlashing::off() {
  if (_flashing) {
    _ticker.detach();
    _flashing = false;
  }
  Heartbeat::off();
}

void HeartbeatFlashing::flash(unsigned int onMilliseconds) {
  on();
  _flashing = true;
  _ticker.attach_ms(onMilliseconds, _flash_tick, &_led);
}
