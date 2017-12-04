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

#ifndef LED_h
#define LED_h

#include <Arduino.h>

class LED {
 public:
  LED(uint8_t pin, bool activeHigh = false);
  virtual void on();
  virtual void off();
  virtual void toggle();
  virtual void setState(bool state);
  virtual bool getState();
  virtual ~LED();

 protected:
  uint8_t _pin;
  bool _activeHigh;
};

class LEDOpenDrain : public LED {
 public:
  LEDOpenDrain(uint8_t pin);
  virtual void on();
  virtual void off();
  virtual bool getState();

 protected:
  bool _state;
};

#endif
