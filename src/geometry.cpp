#include "geometry.h"
// #include <math.h>
#include "FastLED.h"


uint8 ledIndexes[] = {34, 4, 38, 5, 20,
                      28, 11, 33, 45, 14, //10
                      44, 15, 1, 39, 21,
                      32, 27, 10, 49, 6, //20
                      43, 35, 3, 37, 22,
                      19, 29, 16, 48, 40, //30
                      0, 30, 12, 9, 46,
                      13, 31, 7, 2, 36, //40
                      23, 42, 26, 18, 47,
                      17, 24, 41, 8, 25};

float getDistance(Point p1, Point p2)
{
  float sum = 0;
  float *coord1 = &p1.x;
  float *coord2 = &p2.x;
  for (int axis = X; axis <= Z; ++axis)
  {
    sum += pow(Abs(*coord1 - *coord2), 2);
    ++coord1;
    ++coord2;
  }
  float r = sqrt(sum);
  return r;
}

void getBulbs(Bulb bulbs[NUM_LEDS])
{
  float offset = 2. / NUM_LEDS;
  float increment = M_PI * (3. - sqrt(5.));
  for (int i = 0; i < NUM_LEDS; ++i)
  {
    float y = ((i * offset) - 1) + (offset / 2);
    float r = sqrt(1 - pow(y, 2));
    float phi = (i % NUM_LEDS) * increment;
    while (phi > 2 * M_PI)
    {
      phi -= 2 * M_PI;
    }
    uint8 theta = UINT8_MAX * phi / (2 * M_PI);
    bulbs[ledIndexes[i]] = {
        {
            cos8(theta) / (float)UINT8_MAX * r,
            (y + 1) / 2,
            sin8(theta) / (float)UINT8_MAX * r,
            r,
            phi,
        },
        ledIndexes[i]};
    // Serial.println(bulbs[ledIndexes[i]].location.x);
    // Serial.println(bulbs[ledIndexes[i]].location.y);
    // Serial.println(bulbs[ledIndexes[i]].location.z);
  }
}