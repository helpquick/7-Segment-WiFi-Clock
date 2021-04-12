/*
 * 7-Segment WiFi Clock
 * based on https://github.com/leonvandenbeukel/7-Segment-Digital-Clock-V2
 * 
 */

#include <Wire.h>
#include <RtcDS3231.h>                        // Include RTC library by Makuna: https://github.com/Makuna/Rtc
#include <ESP8266WiFi.h>
#include <LittleFS.h>                         // See https://github.com/earlephilhower/arduino-esp8266littlefs-plugin for tools to upload files.
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <FastLED.h>
#include <NTPClient.h>                        // Note, there is a bug in v3.2.0 available in the Arduino libs, see readme for simple fix
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoLog.h>
#include "config.h"


#define countof(a) (sizeof(a) / sizeof(a[0]))


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,NTPSERVER,NTPOFFSET,NTPUPDATEFREQ);

RtcDS3231<TwoWire> Rtc(Wire);

ESP8266WebServer server(80);
AutoConnectConfig acConfig;

AutoConnect Portal(server);                   // AutoConnect captive portal to setup WiFi

CRGB LEDs[NUM_LEDS];

CRGB scoreboardColorLeft = CRGB::Green;
CRGB scoreboardColorRight = CRGB::Red;
CRGB countdownColor = CRGB::Green;


unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;
byte scoreboardLeft = 0;
byte scoreboardRight = 0;
CRGB alternateColor = CRGB::Black; 
unsigned long prevTime = 0;



/*

   - 5 -
  |     |
  4     6
  |     |
   - 7 -
  |     |
  3     1
  |     |
   - 2 -

*/
// bitmap for LEDs 
byte numbers[] = { 
//  7654321
  0b0111111,  // [0] 0
  0b0100001,  // [1] 1
  0b1110110,  // [2] 2
  0b1110011,  // [3] 3
  0b1101001,  // [4] 4
  0b1011011,  // [5] 5
  0b1011111,  // [6] 6
  0b0110001,  // [7] 7
  0b1111111,  // [8] 8
  0b1111011,  // [9] 9
  0b0000000,  // [10] off
  0b1111000,  // [11] degrees symbol
  0b0011110,  // [12] C(elsius)
  0b1011100,  // [13] F(ahrenheit)
};


void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY); // Only need to send from ESP to PC for debugging output.
  delay(50);
  
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "WiFi Clock starting up..." CR));

  acConfig.apid = PORTAL_SSID;
  acConfig.psk  = PORTAL_PSK;
  
  acConfig.autoReconnect = true;
  acConfig.reconnectInterval = 6;
  
  Portal.config(acConfig);
  
  Portal.onConnect([](IPAddress& ipaddr){
    Log.notice(F("WiFi connected with %s, IP:%s" CR), WiFi.SSID().c_str(), ipaddr.toString().c_str());
    if (WiFi.getMode() & WIFI_AP) {
      WiFi.softAPdisconnect(true);
      WiFi.enableAP(false);
      Log.trace(F("SoftAP:%s shut down" CR), WiFi.softAPSSID().c_str());
      timeClient.begin();
    }
  });

  pinMode(COUNTDOWN_OUTPUT, OUTPUT);

  // RTC DS3231 Setup
  Rtc.Begin();    
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  byte rtcRetry=10;
  while (!Rtc.IsDateTimeValid() & rtcRetry-- > 0 ) {
      if (Rtc.LastError() != 0) {
          // we have a communications error see https://www.arduino.cc/en/Reference/WireEndTransmission for what the number means
          Log.notice(F("RTC communications error = %d" CR), Rtc.LastError());
          
          delay(500);
      } else {
          // Common Causes:
          //    1) first time you ran and the device wasn't running yet
          //    2) the battery on the device is low or even missing
          Log.notice(F("RTC lost confidence in the DateTime!" CR));
          // following line sets the RTC to the date & time this sketch was compiled
          // it will also reset the valid flag internally unless the Rtc device is
          // having an issue
          Rtc.SetDateTime(compiled);    // will be set by NTP later
          rtcRetry = 0;
      }
  }
    
  delay(200);
  
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);  
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Handlers
  server.on("/",HTTP_GET,[]() {           // With AutoConnect, URLs starting at the root served from LittlFS or SPIFFS seem to block the portal access
    server.sendHeader("Location",String("/clock/index.html"), true);
    server.send(302, "text/plain", ""); 
  });
  
  server.on("/color", HTTP_POST, []() {    
    r_val = server.arg("r").toInt();
    g_val = server.arg("g").toInt();
    b_val = server.arg("b").toInt();
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/setdate", HTTP_POST, []() { 
    // Sample input: date = "Dec 06 2009", time = "12:34:56"
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    String datearg = server.arg("date");
    String timearg = server.arg("time");
    char d[12];
    char t[9];
    datearg.toCharArray(d, 12);
    timearg.toCharArray(t, 9);
    RtcDateTime compiled = RtcDateTime(d, t);
    Rtc.SetDateTime(compiled);   
    clockMode = 0;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/brightness", HTTP_POST, []() {    
    brightness = server.arg("brightness").toInt();    
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/countdown", HTTP_POST, []() {    
    countdownMilliSeconds = server.arg("ms").toInt();     
    byte cd_r_val = server.arg("r").toInt();
    byte cd_g_val = server.arg("g").toInt();
    byte cd_b_val = server.arg("b").toInt();
    digitalWrite(COUNTDOWN_OUTPUT, LOW);
    countdownColor = CRGB(cd_r_val, cd_g_val, cd_b_val); 
    endCountDownMillis = millis() + countdownMilliSeconds;
    allBlank(); 
    clockMode = 1;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/temperature", HTTP_POST, []() {   
    temperatureCorrection = server.arg("correction").toInt();
    temperatureSymbol = server.arg("symbol").toInt();
    clockMode = 2;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  server.on("/scoreboard", HTTP_POST, []() {   
    scoreboardLeft = server.arg("left").toInt();
    scoreboardRight = server.arg("right").toInt();
    scoreboardColorLeft = CRGB(server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
    scoreboardColorRight = CRGB(server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
    clockMode = 3;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  server.on("/hourformat", HTTP_POST, []() {   
    hourFormat = server.arg("hourformat").toInt();
    clockMode = 0;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 

  server.on("/clock", HTTP_POST, []() {       
    clockMode = 0;     
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  
  
  // Before uploading the files with the "ESP8266 LittleFS Data Upload" tool, zip the files with the command "gzip -r ./data/" (on Windows I do this with a Git Bash)
  // *.gz files are automatically unpacked and served from your ESP (so you don't need to create a handler for each file).
  server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico");
  server.serveStatic("/clock/", LittleFS, "/", "max-age=86400");
 
  
  Portal.begin();
  
  LittleFS.begin();
  Log.trace(F("LittleFS contents:" CR));
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Log.trace(F("FS File: %s, size: %s" CR), fileName.c_str(), String(fileSize).c_str());
  }
  
  digitalWrite(COUNTDOWN_OUTPUT, LOW);
  MDNS.begin(ota_hostname);
  MDNS.addService("http","tcp",80);
  setOTA();
}

void setOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA, lock other functions" CR));
    

  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F(CR "OTA done" CR));

  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %d%%" CR), uint8_t (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {

    Log.error(F("Error[%u]: "), error);
    if (error == OTA_AUTH_ERROR)
      Log.error(F("Auth Failed" CR));
    else if (error == OTA_BEGIN_ERROR)
      Log.error(F("Begin Failed" CR));
    else if (error == OTA_CONNECT_ERROR)
      Log.error(F("Connect Failed" CR));
    else if (error == OTA_RECEIVE_ERROR)
      Log.error(F("Receive Failed" CR));
    else if (error == OTA_END_ERROR)
      Log.error(F("End Failed" CR));
  });
  ArduinoOTA.begin();
}

void displayNumber(byte number, byte segment, CRGB color) {
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = LEDS_PER_DIGIT;
      break;
    case 2:
      startindex = LEDS_PER_DIGIT * 2 + LEDS_PER_DOT * 2;
      break;
    case 3:
      startindex = LEDS_PER_DIGIT * 3 + LEDS_PER_DOT * 2;
      break;    
  }

  for (byte i=0; i<7; i++){                // 7 segments
    for (byte j=0; j<LEDS_PER_SEG; j++) {             // LEDs per segment
      yield();
      LEDs[i * LEDS_PER_SEG + j + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
    }
  } 

}

void allBlank() {
  for (int i=0; i<NUM_LEDS; i++) {
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
}

void updateClock() {  
  RtcDateTime now = Rtc.GetDateTime();
  
  printDateTime(now);    // print date and time to serial

  int hour = now.Hour();
  int mins = now.Minute();
  int secs = now.Second();

  if (hourFormat == 12 && hour > 12)
    hour = hour - 12;
  
  byte h1 = hour / 10;
  byte h2 = hour % 10;
  byte m1 = mins / 10;
  byte m2 = mins % 10;  
  byte s1 = secs / 10;
  byte s2 = secs % 10;
  
  CRGB color = CRGB(r_val, g_val, b_val);

  if (h1 > 0 || showZero == 1 ) {
    displayNumber(h1,3,color);
  } else { 
    displayNumber(10,3,color);  // Blank
  }
  
  displayNumber(h2,2,color);
  displayNumber(m1,1,color);
  displayNumber(m2,0,color); 

  displayDots(color);  
}

void updateCountdown() {

  if (countdownMilliSeconds == 0 && endCountDownMillis == 0) 
    return;
    
  unsigned long restMillis = endCountDownMillis - millis();
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  
  Log.trace(F("%d %d %d %d | %d %d" CR),restMillis, hours, minutes, seconds, remMinutes, remSeconds);
  
  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;

  CRGB color = countdownColor;
  if (restMillis <= 60000) {
    color = CRGB::Red;
  }

  if (hours > 0) {
    // hh:mm
    displayNumber(h1,3,color); 
    displayNumber(h2,2,color);
    displayNumber(m1,1,color);
    displayNumber(m2,0,color);  
  } else {
    // mm:ss   
    displayNumber(m1,3,color);
    displayNumber(m2,2,color);
    displayNumber(s1,1,color);
    displayNumber(s2,0,color);  
  }

  displayDots(color);  

  if (hours <= 0 && remMinutes <= 0 && remSeconds <= 0) {
    Log.trace(F("Countdown timer ended." CR));
    //endCountdown();
    countdownMilliSeconds = 0;
    endCountDownMillis = 0;
    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
    return;
  }  
}

void endCountdown() {
  allBlank();
  for (int i=0; i<NUM_LEDS; i++) {
    if (i>0)
      LEDs[i-1] = CRGB::Black;
    
    LEDs[i] = CRGB::Red;
    FastLED.show();
    delay(25);
  }  
}

void displayDots(CRGB color) {
  if (dotsOn) {
    LEDs[LEDS_PER_DIGIT * 2] = color;
    LEDs[LEDS_PER_DIGIT * 2 + 1] = color;
  } else {
    hideDots();
  }
  dotsOn = !dotsOn;  
}

void hideDots() {
  LEDs[LEDS_PER_DIGIT * 2] = CRGB::Black;
  LEDs[LEDS_PER_DIGIT * 2 + 1] = CRGB::Black;
}

void updateTemperature() {
  RtcTemperature temp = Rtc.GetTemperature();
  float ftemp = temp.AsFloatDegC();
  float ctemp = ftemp + temperatureCorrection;
  Log.trace(F("Sensor temp: %F Corrected: %F" CR),ftemp,ctemp);
  
  if (temperatureSymbol == 13)
    ctemp = (ctemp * 1.8000) + 32;

  byte t1 = int(ctemp) / 10;
  byte t2 = int(ctemp) % 10;
  CRGB color = CRGB(r_val, g_val, b_val);
  displayNumber(t1,3,color);
  displayNumber(t2,2,color);
  displayNumber(11,1,color);
  displayNumber(temperatureSymbol,0,color);
  hideDots();
}

void updateScoreboard() {
  byte sl1 = scoreboardLeft / 10;
  byte sl2 = scoreboardLeft % 10;
  byte sr1 = scoreboardRight / 10;
  byte sr2 = scoreboardRight % 10;

  displayNumber(sl1,3,scoreboardColorLeft);
  displayNumber(sl2,2,scoreboardColorLeft);
  displayNumber(sr1,1,scoreboardColorRight);
  displayNumber(sr2,0,scoreboardColorRight);
  hideDots();
}

void printDateTime(const RtcDateTime& dt)
{
  Log.trace(F("%04d-%02d-%02d %02d:%02d:%02d" CR),
    dt.Year(),
    dt.Month(),
    dt.Day(),
    dt.Hour(),
    dt.Minute(),
    dt.Second()
  );
}


void updateRtcFromNtp(){
  unsigned long NtpEpoch = timeClient.getEpochTime();              // Epoch will eventually exceed 32bit signed int, so needs to be unsigned long
  RtcDateTime ntpTime = RtcDateTime(long (NtpEpoch - 946684800)); // 946684800 is Epoch at 1/1/2000 for Rtc update.
  Rtc.SetDateTime(ntpTime);
  Log.notice(F("RTC updated from NTP" CR));
}



void loop(){

  Portal.handleClient(); // and web requests
  
  if(timeClient.update()) {
    updateRtcFromNtp();
  }
  
  unsigned long currentMillis = millis();  
  if (currentMillis - prevTime >= 1000) {
    prevTime = currentMillis;
  switch (clockMode) {
    case 0:
      updateClock();
      break;
    case 1:
      updateCountdown();
      break;
    case 2:
      updateTemperature();
      break;
    case 3:
      updateScoreboard();
      break;
  }
  

    FastLED.setBrightness(brightness);
    FastLED.show();

    ArduinoOTA.handle();
  }   
}
