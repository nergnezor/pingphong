// #include <bitswap.h>
// #include <chipsets.h>
// #include <color.h>
// #include <colorpalettes.h>
// #include <colorutils.h>
// #include <controller.h>
// #include <cpp_compat.h>
// #include <dmx.h>
// #include <FastLED.h>
// #include <fastled_config.h>
// #include <fastled_delay.h>
// #include <fastled_progmem.h>
// #include <fastpin.h>
// #include <fastspi.h>
// #include <fastspi_bitbang.h>
// #include <fastspi_dma.h>
// #include <fastspi_nop.h>
// #include <fastspi_ref.h>
// #include <fastspi_types.h>
// #include <hsv2rgb.h>
// #include <led_sysdefs.h>
// #include <lib8tion.h>
// #include <noise.h>
// #include <pixelset.h>
// #include <pixeltypes.h>
// #include <platforms.h>
// #include <power_mgt.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <math.h>
#include <SPI.h> //SPI
#define FASTLED_ESP8266_RAW_PIN_ORDER
// #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define FASTLED_ESP8266_D1_PIN_ORDER
#include "FastLED.h"

#include "geometry.h"

CRGB leds[NUM_LEDS];
Bulb bulbs[NUM_LEDS];

#define CS 15
#define ESP8266_LED 2
const char *ssid = "jazik";
const char *password = "R0j4rr4lf";
const char *host = "OTA-LEDS";

int led_pin = ESP8266_LED;
#define N_DIMMERS 0
int dimmer_pin[] = {14, 5, 15};
void setup()
{
  Serial.begin(115200);

  for (int i = 0; i < 50; ++i)
  {
    SPI.transfer(10);
    SPI.transfer(0);
    SPI.transfer(0);
  }
  /* switch on led */
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    Serial.println("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(led_pin, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(led_pin, 990);

  for (int i = 0; i < N_DIMMERS; i++)
  {
    pinMode(dimmer_pin[i], OUTPUT);
    analogWrite(dimmer_pin[i], 50);
  }

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
    for (int i = 0; i < N_DIMMERS; i++)
      analogWrite(dimmer_pin[i], 0);
    analogWrite(led_pin, 0);
  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
    for (int i = 0; i < 30; i++)
    {
      analogWrite(led_pin, (i * 100) % 1001);
      delay(50);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) { ESP.restart(); });

  /* setup the OTA server */
  ArduinoOTA.begin();
  Serial.println("Ready");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip.toString());
  SPI.begin();
  uint8_t buf[] = {255, 20, 0};

  for (int i = 0; i < 50; ++i)
  {
    SPI.transfer(buf[0]);
    SPI.transfer(buf[1]);
    SPI.transfer(buf[2]);
  }
  getBulbs(bulbs);
  RainInit();
  Serial.println("Starting fastLED");
  FastLED.addLeds<WS2801, 13, 14>(leds, NUM_LEDS);
  FastLED.show();
}
int limit = 0;
class Eye
{
public:
  Bulb Eye_target;
  Eye()
  {
    Eye_target.location.r = 1;
    Eye_target.location.y = 0.8;
  }
  int Update()
  {
    Move();
    Bulb *target = &Eye_target;
    int closestIndex = -1;
    uint minDist = UINT32_MAX;
    for (int i = 0; i < NUM_LEDS; ++i)
    {
      float distance = MIN(1, getDistance(target->location, bulbs[i].location));
      leds[i] = CHSV(30 + 100 * distance, 255 - 150 * distance, Max(0, 100 - 120 * distance));
    }
    return minDist;
  }

private:
  int counter = 0;
  float yLoc = 0.0001;
  void Move()
  {
    Eye_target.location.phi += 0.01;
    // Eye_target.location.r += 0.001;
    if (Eye_target.location.r > 1.2)
    {
      Eye_target.location.r = 0;
    }
    if (Eye_target.location.phi > 2 * M_PI)
    {
      Eye_target.location.phi -= 2 * M_PI;
    }
    uint8 theta = UINT8_MAX * Eye_target.location.phi / (2 * M_PI);
    Eye_target.location.x = (cos8(theta) / (float)UINT8_MAX) * Eye_target.location.r;
    Eye_target.location.z = (sin8(theta) / (float)UINT8_MAX) * Eye_target.location.r;
  }
};

void VerticalRainbow()
{
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    // uint8 hue = 127 * ((1 + bulbs[i].y) - yLoc);

    // leds[i] = CHSV(hue, 255, 30);
    // leds[i] = CHSV(i*4, 255, 30);
    leds[i] = CHSV((255 * bulbs[i].location.phi) / (2 * M_PI), 255, 30);
  }
}

Eye eye;
#define N_RAINDROPS_MAX 10
#define N_DROPBULBS_MAX 5
struct WaterBulb;
struct BulbBelow
{
  WaterBulb *bulb;
  int index;
  float distance;
  int amount;
};
struct WaterBulb
{
  BulbBelow below[N_DROPBULBS_MAX];
  int amount;
};
// struct Rain
// {
//   Raindrop drops[N_RAINDROPS_MAX];
//   int count;
// };
// struct Rain rain;

static WaterBulb waterBulbs[NUM_LEDS] = {0};

void Rain()
{
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    // Serial.println(i);
    if (waterBulbs[i].amount > 0)
    {
          Serial.println(i);
          Serial.println(waterBulbs[i].amount);
      // leds[i] = CHSV(100, 255, 30);
      for (int k = 0; k < N_DROPBULBS_MAX; ++k)
      {
        BulbBelow *below = &waterBulbs[i].below[k];
        if (below->distance > 0)
        {
          // below->amount = Min(255, below->amount + 1);
          below->bulb->amount = Min(255, below->bulb->amount + 1);
          // leds[below->index] = CHSV(255 * below->distance, 255, 255);
          Serial.println(below->index);
          Serial.println(below->bulb->amount);
        }
      }
      // waterBulbs[i].amount = Max(0, waterBulbs[i].amount - 1);
    }
          Serial.println();
  }
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    leds[i] = CHSV(0, 255, waterBulbs[i].amount);
  }
}

void RainInit()
{
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    Bulb *closest[N_DROPBULBS_MAX];
    if (bulbs[i].location.y < 0.1)
    {
      waterBulbs[i].amount = 255;
    }
    Serial.println(i);
    for (int j = 0; j < NUM_LEDS; ++j)
    {
      if (i == j || bulbs[i].location.y < bulbs[j].location.y)
        continue;
      float distance = MIN(1, getDistance(bulbs[i].location, bulbs[j].location));
      // Serial.println(distance);
      for (int k = 0; k < N_DROPBULBS_MAX; ++k)
      {
        BulbBelow *below = &waterBulbs[i].below[k];
        // Serial.println(below->distance);
        if (below->distance == 0 || distance < below->distance)
        {
          if (distance < below->distance)
          {
            for (int l = N_DROPBULBS_MAX - 1; l > k; --l)
            {
              waterBulbs[i].below[l] = waterBulbs[i].below[l - 1];
            }
          }
          below->bulb = &waterBulbs[j];
          below->index = j;
          below->distance = distance;
          break;
        }
      }
      // Serial.println(j);
    }
    Serial.println();
  }
  Rain();
}
void loop()
{
  ArduinoOTA.handle();

  // eye.Update();
  Rain();
  FastLED.show();
  delay(1000);
}
