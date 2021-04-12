# 7-Segment WiFi Clock

Based on the 7-Segment-Digital-Clock-V2 by [Leon van den Beukel](https://github.com/leonvandenbeukel)

This version of the 7 segment digital clock uses a configurable number of WS2812B LEDs per segment.

WiFi settings are configured using a Captive Portal. If no existing WiFi connection can be established, the clock will start a temporary access point "ClockSetupAP", the password for this is "ClockSetup".

NTP server and time offset are currently configured in the "config.h" file. I am working on adding these settings to the Web interface.

Web interface can be accessed in browser using <http://wifi-clock/>

OTA Update has been implemented, so code can be updated without needing to be plugged into a PC.

NTPClient library has a small bug, on the update() fuction, this should normally return false, so a small change can be made to fix this in <arduino>/Libraries/NTPClient/

## Dependencies

ESP8266 boards added to Arduino IDE, please see <https://github.com/esp8266/Arduino>

### Libraries

* Rtc by Makuna
* AutoConnect
  * ArduinoJson
  * PageBuilder
* FastLED
* ArduinoLog
* NTPClient \*

*LittleFS upload tool*, please see <https://github.com/earlephilhower/arduino-esp8266littlefs-plugin> for details.

\* NTPClient v 3.2.0 as provided by the Arduino Library manager has a small bug related to the `update` function.

To fix this, edit Documents\\Arduino\\Libraries\\NTPClient\\NTPClient.cpp, find the `update()` function around line 95, and change the final `return true;` on line 101 with `return false;`

### Hardware

My build was based on the Wemos D1 Mini, other ESP8266 boards may work too.

* ESP8266 based board (Tested with Wemos D1 mini)
* DS3231 I2C RTC Module
* Addressable LED strips as supported by the FastLED Library: <https://github.com/FastLED/FastLED#supported-led-chipsets> (Tested with WS2812B)
* 5V power supply powerfull enough for all LEDs

### Assembly

Soldering skills are required to connect all segments together.

I constructed my clock on some spare plywood, which worked for my needs. If you are looking to 3D print a clock, the orgingal page (<https://github.com/leonvandenbeukel/7-Segment-Digital-Clock-V2>) has cad files, etc. for 3-LED based setup.

There is also a youtube video linked from this page with construction details.

### Arduino code

Upload the .ino file to your ESP board. You also need to upload the html and javascript files with the ESP8266 LittleFS tool. Check this link for more info:

<https://github.com/earlephilhower/arduino-esp8266littlefs-plugin>

Before uploading the files to your ESP board you have to gzip them with the command:

`gzip -r ./data/`

Afterwards if you want to change something to the html files, just unzip with:

`gunzip -r ./data/`