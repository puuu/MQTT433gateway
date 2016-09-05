#!/usr/bin/env python

"""MQTT433gateway OTA updater

Project home: https://github.com/puuu/MQTT433gateway/
"""

import hashlib
import time
import http.server
import socketserver

import paho.mqtt.client as mqtt

HTTPPORT = 8000

def handlemqtt(espid, passwd, url, broker):
    """Send url to espid by MQTT and handle digest authentication"""
    client = mqtt.Client()

    def on_connect(client, userdata, flags, rc):
        print("Connected with result code "+str(rc))
        client.subscribe(espid+"/ota/nonce")
        topic = espid+"/ota/url"
        payload = url
        print("Sending:", topic, payload)
        client.publish(topic, payload)
        return

    def on_message(client, userdata, msg):
        print(msg.topic+" "+msg.payload.decode())
        topic = espid+"/ota/passwd"
        payload = auth(passwd, msg.payload.decode())
        print("Sending:", topic, payload)
        client.publish(topic, payload)
        client.disconnect()
        return

    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(broker, 1883, 60)
    client.loop_forever()
    return


def auth(passwd, nonce_digest):
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


def main(espid, passwd, filename, broker, httpip):
    """main function:
    - start http server and serve image
    - handle MQTT and authentication
    """
    httphandler = http.server.SimpleHTTPRequestHandler
    socketserver.TCPServer.allow_reuse_address = True
    httpd = socketserver.TCPServer((httpip, HTTPPORT), httphandler)

    url = "http://"+httpip+":"+str(HTTPPORT)+"/"+filename

    print("Start MQTT. broker: ", broker)
    handlemqtt(espid, passwd, url, broker)

    print("Serving at port", HTTPPORT)
    httpd.handle_request()
    httpd.server_close()
    return

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 6:
        print("Usage:", sys.argv[0], "<ESPid> <passwd> <file> <broker> <thisip>")
        exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
