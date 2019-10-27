#include "rain.h"
#include "geometry.h"

#define N_RAINDROPS_MAX 10
#define N_DROPBULBS_MAX 10

#define N_RAINDROPS_MAX 10
#define N_DROPBULBS_MAX 10
struct WaterBulb;
struct BulbBelow {
  WaterBulb *bulb;
  int index;
  float distance;
  int amount;
};
struct WaterBulb {
  BulbBelow below[N_DROPBULBS_MAX];
  int belowCount;
  int amount;
  bool wasFull;
};
static WaterBulb waterBulbs[NUM_LEDS];
extern CRGB leds[NUM_LEDS];
extern Bulb bulbs[NUM_LEDS];

void Rain::Update() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (bulbs[i].location.y < 0.01) {
      // if (waterBulbs[i].amount == 0)
      waterBulbs[i].amount = 100;
      // waterBulbs[i].amount = Min(255, waterBulbs[i].amount + 1);
      waterBulbs[i].wasFull = true;
    }
    if (waterBulbs[i].amount > 0) {
      if (waterBulbs[i].amount >= 255) {
        waterBulbs[i].wasFull = true;
      }
      if (waterBulbs[i].wasFull) {
        for (int k = 0; k < waterBulbs[i].belowCount; ++k) {
          BulbBelow *below = &waterBulbs[i].below[k];
          if (below == 0) break;
          float dy = bulbs[below->index].location.y - bulbs[i].location.y;
          // int leakage = dy * 20;
          int leakage = dy * 20 + below->distance / 1;
          leakage *= 4;
          // Serial.println(leakage);
          waterBulbs[below->index].amount =
              Min(255, waterBulbs[below->index].amount + leakage);
          waterBulbs[i].amount = Max(0, waterBulbs[i].amount - leakage);
        }
        if (waterBulbs[i].amount <= 0) {
          waterBulbs[i].wasFull = false;
          waterBulbs[i].amount = 0;
        }
      }
    }
  }
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CHSV((150 - waterBulbs[i].amount / 20), 200, waterBulbs[i].amount);
  }
}

Rain::Rain() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (bulbs[i].location.y < 0.01) {
      waterBulbs[i].amount = 255;
      waterBulbs[i].wasFull = true;
    }
    for (int j = 0; j < NUM_LEDS; j++) {
      if (i == j || bulbs[j].location.y < bulbs[i].location.y) continue;
      float distance =
          Min(1, getDistance(bulbs[i].location, bulbs[j].location));
      // Serial.println(distance);
      if (distance > 0.5) continue;
      for (int k = 0; k < N_DROPBULBS_MAX; ++k) {
        BulbBelow *below = &waterBulbs[i].below[k];
        if (k >= waterBulbs[i].belowCount || distance < below->distance) {
          if (distance < below->distance) {
            for (int l = N_DROPBULBS_MAX - 1; l > k; --l) {
              waterBulbs[i].below[l] = waterBulbs[i].below[l - 1];
            }
          }
          below->bulb = &waterBulbs[j];
          below->index = j;
          below->distance = distance;
          ++waterBulbs[i].belowCount;
          break;
        }
      }
      // Serial.println(waterBulbs[i].belowCount);
    }
  }
  Update();
}

