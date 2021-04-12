// Captive portal settings:
#define PORTAL_SSID "WiFi-Clock";
#define PORTAL_PSK  "ClockSetup";


#define LEDS_PER_SEG 2
#define LEDS_PER_DOT 1
#define NTPSERVER "uk.pool.ntp.org"           // NTP Server to use
#define NTPOFFSET 3600                        // Number of seconds offset from UTC (currently UTC+1hr = BST)
#define NTPUPDATEFREQ 6000000                 // Number of miliseconds between NTP updates (6000000 = 1hr)

#define LEDS_PER_DIGIT LEDS_PER_SEG * 7

#define NUM_LEDS     LEDS_PER_DIGIT * 4 + 2 * LEDS_PER_DOT       // 2 LEDs per Segment = 14 LEDs per digit, plus 2 for "Dots"
                                              
#define DATA_PIN D6                           // Change this if you are using another type of ESP board than a WeMos D1 Mini
#define MILLI_AMPS 1200                       // need to increase this for large number of LEDs and make sure to use external power.
#define COUNTDOWN_OUTPUT D5
#define LOG_LEVEL LOG_LEVEL_TRACE

// OTA Settings
#define ota_hostname "WiFi-Clock"
#define ota_password ""
#define ota_port 8266


// Settings
byte r_val = 0;
byte g_val = 255;
byte b_val = 0;
bool dotsOn = true;
byte brightness = 10;
float temperatureCorrection = -3.0;
byte temperatureSymbol = 12;                  // 12=Celcius, 13=Fahrenheit check 'numbers'
byte clockMode = 0;                           // Clock modes: 0=Clock, 1=Countdown, 2=Temperature, 3=Scoreboard
byte hourFormat = 24;                         // Change this to 12 if you want default 12 hours format instead of 24
byte showZero = 1;                            // Display leading 0, or set to 0 to have blank instead.
