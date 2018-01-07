#pragma once
#include <stdint.h>
#define Abs(a) ((a) < 0 ? -(a) : (a))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))

#define NUM_LEDS 50

enum Axis
{
  X,
  Y,
  Z,
  R,
  PHI
};

struct Point
{
    float x;
    float y;
    float z;
    float r;
    float phi;
};

struct Bulb
{
    Point location;
    uint8_t index;
};


float getDistance(Point p1, Point p2);
void getBulbs(Bulb bulbs[]);
