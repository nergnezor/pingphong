#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>             //SPI

#define CS 15
#define ESP8266_LED 2
const char* ssid = "jazik";
const char* password = "R0j4rr4lf";
const char* host = "OTA-LEDS";

int led_pin = 13;
#define N_DIMMERS 3
int dimmer_pin[] = {14, 5, 15};

void setup() {
   Serial.begin(115200);

   /* switch on led */
   pinMode(led_pin, OUTPUT);
   digitalWrite(led_pin, LOW);

   Serial.println("Booting");
   WiFi.mode(WIFI_STA);

   WiFi.begin(ssid, password);

   while (WiFi.waitForConnectResult() != WL_CONNECTED){
     WiFi.begin(ssid, password);
     Serial.println("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(led_pin, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(led_pin,990);

  for (int i=0; i<N_DIMMERS; i++)
  {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i],50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
                        for(int i=0; i<N_DIMMERS;i++)
                          analogWrite(dimmer_pin[i], 0);
                          analogWrite(led_pin,0);
                    });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
                          for (int i=0;i<30;i++)
                          {
                            analogWrite(led_pin,(i*100) % 1001);
                            delay(50);
                          }
                        });

   ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

   /* setup the OTA server */
   ArduinoOTA.begin();
   Serial.println("Ready!");
  SPI.begin();
  uint8_t buf[] = {255,10,0};

  for (int i = 0; i < 50; ++i)
  {
    SPI.transfer(buf[0]);
    SPI.transfer(buf[1]);
    SPI.transfer(buf[2]);
  }
}

void loop() {
  ArduinoOTA.handle();
}
