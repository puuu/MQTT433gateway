/*
  SHAauth - SHA1 digest authentication
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

#include "SHAauth.h"
#include <Hash.h>

//#define DEBUG

SHAauth::SHAauth(const String &password, unsigned long validMillis) {
  _passHash = sha1(password);
  _validMillis = validMillis;
  _timestamb = 0;
  _nonceHash = "";
};

String SHAauth::nonce(void) {
  _nonceHash = sha1(String(micros()));
  _timestamb = millis();
  return _nonceHash;
}

boolean SHAauth::verify(const String &answer) {
  if ((_nonceHash.length() > 0) && ((millis() - _timestamb) <= _validMillis)){
    int pos = answer.indexOf(' ');
    if ((pos > 1) && (answer.length() > pos)) {
      String cnonce = answer.substring(0, pos);
      String response = answer.substring(pos+1);
      String result = sha1(_passHash + ":" + _nonceHash + ":" + cnonce);
#ifdef DEBUG
      Serial.print("SHAauth::verify: ");
      Serial.print(cnonce);
      Serial.print(" ");
      Serial.print(response);
      Serial.print(" ");
      Serial.println(result);
#endif //DEBUG
      return response == result;
    }
  }
  return false;
};
