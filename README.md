[![Build Status](https://travis-ci.org/puuu/MQTT433gateway.svg?branch=master)](https://travis-ci.org/puuu/MQTT433gateway)

# MQTT433gateway

This Project implements a [MQTT](https://en.wikipedia.org/wiki/MQTT)
433.92MHz radio-frequency device gateway.  This is a IoT bridge to
couple popular RF devices like RC power-socket switches or weather
stations to internet capable devices like smartphone and tables.  In
this way they can be integrated into a home automation system.

The Software runs on a ESP8266 micro controller/WiFi chip and process
many 433.92MHz radio-protocols themselves.  The protocols are
processed utilizing the [ESPiLight](https://github.com/puuu/ESPiLight)
Arduino library which is a port of the [pilight](https://pilight.org/)
433.92MHz protocols to the Arduino platform.  The aim is to transmit,
receive and parse many 433.92MHz protocols directly on the device and
publish the results via MQTT.

A list of supported protocols can be found in the pilight wiki:
https://wiki.pilight.org/doku.php/protocols


## Software/Requirements

The software is using the [PlatformIO](http://platformio.org/) ecosystem. See their
[install instructions](http://docs.platformio.org/en/latest/installation.html) Or get
their [IDE](http://docs.platformio.org/en/latest/ide/pioide.html) to get the
software. More information can be found in their [documentation](http://docs.platformio.org/en/latest/).

1. Open a terminal and go to the MQTT433Gateway project folder.

2. Then initialize the project:
   ```
   platformio init
   ```

3. After that, decide for which board to create the firmware and give
   this as `--environment` to the platformio `run` command:
   ```
   platformio run --environment <board>
   ```
   The available boards are defined in `platformio.ini`. Currently, this are
   + `esp12e` for ESP8266-12e/f models,
   + `nodemcuv2` for NodeMCU boards,
   + `d1_mini`for D1 Mini boards,
   + `huzzah` for the Huzzah boards.

4. To flash the firmware to the board, connect the board to the PC via
   USB or a serial adapter. Make sure, it is the only device currently
   connected, as otherwise you might flash the wrong unit
   accidentaly. Some boards need to be set manually into a programming
   mode, please check the manual of the board to get more details about
   how to do that.  The `platformio run` has the upload target to
   initialize flashing:
   ```
   platformio run --environment <board> --target upload
   ```
   this will try to autodetect the serial port. If this is not
   successful, try
   ```
   platformio run --environment <board> --target upload --upload-port <path-to-serial-port>
   ```

Older versions of MQTT433gateway were developed with the Arduino
IDE. You can find the old sources in the departed
[`arduino`](../../tree/arduino) branch.


## Hardware

![MQTT433gateway box](hardware/MQTT433gateway_photo.jpg) ![MQTT433gateway box open](hardware/MQTT433gateway_photo_inside.jpg)

The Software is primary written for ESP8266 devices.  It is tested
with an
[Adafruit HUZZAH ESP8266](https://www.adafruit.com/product/2471), but
any other ESP8266 board should be fine too.  The circuitry can be
found in the [`hardware`](hardware/) folder.  The circuitry utilizes a separate 5V
voltage regulator.  This way it is possible to supply the
MQTT433gateway with up to 12V.  The 434MHz transmitter is directly
connected to the power-supply input, thus it can be driven with 12V
which enhances the transmitting range.  But powering with 12V also
means that the voltage regulator dissipates much power and a heat sink
is highly recommended.

For transmitting and receiving you need 434MHz-RF modules.  More
information can be found here:
- https://wiki.pilight.org/doku.php/receivers
- https://wiki.pilight.org/doku.php/senders
- https://github.com/sui77/rc-switch/wiki/List_TransmitterReceiverModules

If you are interested in a good receiving range, then I can not
recommend a WRL-10532 module.  I only achieved a range of about 5m
with it, after changing it against a RXB6 receiver, the range
increases to over 30m (including walls).

For the transmitter and receiver module you need separate antennas.
1/4 lambda (17cm) antennas are sufficient, but you may want to try
[coil loaded antennas](https://www.elektor.nl/Uploads/Forum/Posts/How-to-make-a-Air-Cooled-433MHz-antenna.pdf).


### LED status

The MQTT433gateway indicates its status by a LED connected to GPIO0:
- Connecting to WiFi: Flashing at 1 Hz
- Normal operation: Heartbeat pulsing
- MQTT connection problems: 4.5s on, 0.5s off
- OTA Update: on during flashing


## Configuration

The device is fully configurable through a web browser. If you boot it
for the first time, it will create a WiFi AccessPoint where you can
connect to. If you type in any url in your browser, you will get
directed to the wifi configuration of the device. You can select your
network here and fill in the credentials.

After that it will stop the access point and connect to your network.
There you can either discover it via mDNS or ask your router for the
address of the device. If you type this address in your browser, you'll
see the main configuration frontend. The default username is `admin`,
the default password is `MQTT433gateway`.

Please fill here your MQTT connection details. You can also change the
topics the device will subscribe and publish to. **Please note:** you
have to change the configuration password! For security reasons the
device will not start working before this password is changed.


## MQTT/Automation

The MQTT433gateway communicates via MQTT, therefore a MQTT broker is
needed, e.g., [Mosquitto](https://mosquitto.org/).  The MQTT433gateway
uses two root topics for communication: a common topic and a
device-only topic.

The common topic is `rf434` and is used for transmitted and received
RF messages.  The idea of utilizing a common topic is to run multiple
gateways in one network and thus to expand the radio range.  The
device-only topic is `rfESP_<ChipId>`, where `<ChipId>` is the
hexdecimal value of the ESP8266 chip ID.  E.g., if the chip ID is
`fd804`, then the topic results to `rfESP_fd804`.

After (re)connecting to the MQTT broker, the gateway sends the message
`online` to the `rfESP_<ChipId>` topic.  In addition, it registered
with the last will set to the message `offline` for the same topic.
This allows to check the status of the MQTT433gateway.

MQTT subscription is done to the following topics:
- `rfESP_<ChipId>/set`: configuration, like switching the logging or
  RAW mode
- `rfESP_<ChipId>/ota`: OTA updates
- `rfESP_<ChipId>/send/<protocol>` and `rf434/send/<protocol>`:
  message that should be transmitted by the RF transmitter

The messages to be transmitted must be valid pilight JSON messages.
`<protocol>` is the pilight protocol name.  Additionally, `<protocol>`
can be `RAW` to transmit a RAW signal similar as used with the
[pilight USB Nano](https://github.com/pilight/pilight-usb-nano/blob/master/pilight_usb_nano.c).

Received and decoded RF signals are published in the
`rf434/recv/<protocol>[/<id>]` topic as pilight JSON message.  To
avoid receiving errors, a message must be received at least twice
before it is published.  `<protocol>` is the pilight protocol name and
the optional `<id>` will be used if the pilight JSON message contains
an `id` attribute.


### Integration in Home Assistant

Here are some examples how to integrate the MQTT433gateway into
[Home Assistant](https://home-assistant.io).

Status of the MQTT433gateway:

```yaml
binary_sensor:
  - platform: mqtt
    state_topic: "rfESP_fd804"
    name: "rfESP_fd804"
    payload_on: "online"
    payload_off: "offline"
```

The message of a TCM 218943 weather station sensor looks like this:
`rf434/recv/tcm/108 {"id":108,"temperature":22.7,"humidity":50,"battery":1,"button":0}`.
A corresponding Home Assistant configuration is:

```yaml
sensor:
  - platform: mqtt
    state_topic: "rf434/recv/tcm/108"
    unit_of_measurement: "°C"
    name: "tcm_a_temp"
    value_template: '{{ value_json.temperature }}'
  - platform: mqtt
    state_topic: "rf434/recv/tcm/108"
    unit_of_measurement: "%"
    name: "tcm_a_humidity"
    value_template: '{{ value_json.humidity }}'

binary_sensor:
  - platform: mqtt
    state_topic: "rf434/recv/tcm/108"
    name: "tcm_a_battery"
    sensor_class: power
    payload_on: 1
    payload_off: 0
    value_template: '{{ value_json.battery }}'
```

Impulse RC socket switches, like many RC socket switches with DIP
switches:

```yaml
switch:
  - platform: mqtt
    name: "impuls_24_1"
    command_topic: "rf434/send/impuls"
    payload_on: '{"systemcode":24,"programcode":1,"on":1}'
    payload_off: '{"systemcode":24,"programcode":1,"off":1}'
  - platform: mqtt
    name: "impuls_24_2"
    command_topic: "rf434/send/impuls"
    payload_on: '{"systemcode":24,"programcode":2,"on":1}'
    payload_off: '{"systemcode":24,"programcode":2,"off":1}'
  - platform: mqtt
    name: "impuls_24_3"
    command_topic: "rf434/send/impuls"
    payload_on: '{"systemcode":24,"programcode":4,"on":1}'
    payload_off: '{"systemcode":24,"programcode":4,"off":1}'
```

Elro 800 based RC socket switches:

```yaml
switch:
  - platform: mqtt
    name: "elro800_13_A"
    command_topic: "rf434/send/elro_800_switch"
    payload_on: '{"systemcode":13,"unitcode":1,"on":1}'
    payload_off: '{"systemcode":13,"unitcode":1,"off":1}'
  - platform: mqtt
    name: "elro800_13_B"
    command_topic: "rf434/send/elro_800_switch"
    payload_on: '{"systemcode":13,"unitcode":2,"on":1}'
    payload_off: '{"systemcode":13,"unitcode":2,"off":1}'
  - platform: mqtt
    name: "elro800_13_C"
    command_topic: "rf434/send/elro_800_switch"
    payload_on: '{"systemcode":13,"unitcode":4,"on":1}'
    payload_off: '{"systemcode":13,"unitcode":4,"off":1}'
  - platform: mqtt
    name: "elro800_13_D"
    command_topic: "rf434/send/elro_800_switch"
    payload_on: '{"systemcode":13,"unitcode":8,"on":1}'
    payload_off: '{"systemcode":13,"unitcode":8,"off":1}'
```

Quigg GT9000 based RC socket switches, e.g., Tevion GT-9000,
SilverCrest 91210 or Intertek Unitec 48110:

```yaml
switch:
  - platform: mqtt
    name: "gt9000_590715_0"
    command_topic: "rf434/send/quigg_gt9000"
    payload_on: '{"id":590715,"unit":0,"on":1}'
    payload_off: '{"id":590715,"unit":0,"off":1}'
  - platform: mqtt
    name: "gt9000_590715_2"
    command_topic: "rf434/send/quigg_gt9000"
    payload_on: '{"id":590715,"unit":2,"on":1}'
    payload_off: '{"id":590715,"unit":2,"off":1}'
  - platform: mqtt
    name: "gt9000_590715_3"
    command_topic: "rf434/send/quigg_gt9000"
    payload_on: '{"id":590715,"unit":3,"on":1}'
    payload_off: '{"id":590715,"unit":3,"off":1}'
```


## OTA Update

Updates of the MQTT433gateway can be preformed by OTA (Over-the-air
programming).  To do this, the binary file must be provided by HTTP.
The URL and the authentication information are handled via MQTT.

All this is implemented in the `ota_update.py` Python script.  It will
start a HTTP server and handle the MQTT communication.  It requires
the Python paho-mqtt module:

```console
$ pip install paho-mqtt
```

You can use the script like:

```console
$ ./ota_update.py <ESPid> <passwd> <file> <broker> <thisip>
```

The parameters are:
- `ESPid`: The MQTT client name of the MQTT433gateway
  (`rfESP_<ChipId>`)
- `passwd`: OTA password as defined in `passwd-net.h`
- `file`: binary file (`<BUILD_DIR>/firmware.bin`, e.g. `.pioenvs/d1_mini/firmware.bin`)
- `broker`: address of the MQTT broker
- `thisip`: ip address of the computer running the script and that is
  access-able by the MQTT433gateway


## Debugging/RF-protocol analyzing

The MQTT433gateway outputs many messages over the RX/TX interface
which could be very helpful for debugging.  In addition it supports a
logging mode and a RAW mode.


### Logging mode

By publishing any message starting with `1` to the MQTT topic
`rfESP_<ChipId>/set/log` activates the logging mode.  Any other message
to this topic will deactivate this mode.  You can use the Mosquitto
tools to publish the message:

```console
$ mosquitto_pub -t rfESP_fd804/set/log -m '1'
```

The logging messages can be observed with `mosquitto_sub`:

```console
$ mosquitto_sub -v -t '#'
rfESP_fd804/log/0/tcm {"id":87,"temperature":18.8,"humidity":68,"battery":1,"button":0}
rf434/recv/tcm/87 {"id":87,"temperature":18.8,"humidity":68,"battery":1,"button":0}
rfESP_fd804/log/2/tcm {"id":87,"temperature":18.8,"humidity":68,"battery":1,"button":0}
rfESP_fd804/log/3/tcm {"id":87,"temperature":18.8,"humidity":68,"battery":1,"button":0}
```


### RAW mode

By publishing any message starting with `1` to the MQTT topic
`rfESP_<ChipId>/set/raw` activates the RAW mode.  Any other message to
this will deactivate this mode.  Be careful, this mode is very verbose
and will generate much network traffic.  You can use the Mosquitto
tools to publish the message:

```console
$ mosquitto_pub -t rfESP_fd804/set/raw -m '1'
```

The RAW messages can be observed with `mosquitto_sub`.  The messages
are similar as used with the
[pilight USB Nano](https://github.com/pilight/pilight-usb-nano/blob/master/pilight_usb_nano.c).

```console
$ mosquitto_sub -v -t '#'
rfESP_fd804/recvRaw c:012234134220145243231624216278;p:1360,578,396,1031,759,2591,1186,1532,5490@
rfESP_fd804/recvRaw c:010101010102020101020334010101010200350201010102020106;p:365,920,1959,188,622,1458,6537@
rfESP_fd804/recvRaw c:010202010110101010101010101010101010102020101020101010101020202010101020203;p:300,926,1960,8014@
rfESP_fd804/recvRaw c:0123040101010101010101010101010104040101040101010101040404010101040405;p:258,965,4878,801,1979,8039@
rfESP_fd804/recvRaw c:01020201020101010101010101010101010102020101020101010101020202010101020203;p:194,1035,1994,8037@
rfESP_fd804/recvRaw c:01020201020101010101013;p:228,1013,2026,5319@
rfESP_fd804/recvRaw c:011203030343031313031343434313030343435;p:875,1025,4918,225,2012,8076@
rf434/recv/tcm/104 {"id":104,"temperature":22.7,"humidity":50,"battery":1,"button":0}
rfESP_fd804/recvRaw c:01020201020101010101010101010101010102020101020101010101020202010101020203;p:204,1010,1976,8039@
```


## Protocol limitation

If you have the situation that there is a lot of noise in the air
and/or your device matches multiple protocols, you can limit the
available protocols. This is easily possible with a MQTT message. Just
send a JSON array with your needed protocols to
`rfESP_<ChipId>/set/protocols`. For example:

```console
$ mosquitto_pub -t rfESP_fd804/set/protocols -m '["elro_800_switch", "arctech_switch"]'
```


## Contributions

If you find any bug, please feel free to fill an issue.  Also, pull
request are welcome.

Please format the code using `clang-format` and the style configuration
`.clang-format` provided by this project.


## Acknowledgement

Big thanks goes to the pilight community, which implemented all the
434MHz protocols.  If you want to integrate more protocols, please
contribute directly to [pilight](https://pilight.org/).
