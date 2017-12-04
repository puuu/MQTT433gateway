#!/usr/bin/env python

"""SHAauth - SHA1 digest authentication.

Python implementation for the Arduino SHA1auth authentication class.

Project home: https://github.com/puuu/MQTT433gateway/
"""

import hashlib
import time


def answer(passwd, nonce_digest):
    """Calculate digest response to passwd and nonce_digest"""
    passwd_digest = hashlib.sha1(passwd.encode()).hexdigest()

    cnonce = str(time.time()).encode()
    cnonce_digest = hashlib.sha1(cnonce).hexdigest()

    response = hashlib.sha1(passwd_digest.encode())
    response.update(b":")
    response.update(nonce_digest.encode())
    response.update(b":")
    response.update(cnonce_digest.encode())

    return "{} {}".format(cnonce_digest, response.hexdigest())


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: {} <passwd> <nonce>".format(sys.argv[0]))
        exit(1)
    print(answer(sys.argv[1], sys.argv[2]))
