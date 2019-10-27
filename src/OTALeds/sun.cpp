#include "sun.h"
extern CRGB leds[NUM_LEDS];
extern Bulb bulbs[NUM_LEDS];

  DEFINE_GRADIENT_PALETTE(heatmap_gp){
    0,   255, 200, 100,  //
    40,  0,   0,   100,  //
    120, 200, 50,  0,    //
    255, 100, 0,   0,    //
                         // 255, 0,   0,   0,    //
};
DEFINE_GRADIENT_PALETTE(rio_gp){
    0,   0,   255, 0,    // green
    60,  0,   0,   255,  // blue
    130, 128, 0,   255,  // purple
    255, 255, 0,   200,  // pink    
};

void Sun::Update()   {
    Move();
    for (int i = 0; i < NUM_LEDS; ++i) {
      float distance = getDistance(location, bulbs[i].location) / 1.2;
      uint8_t heatindex = distance * 255;
      leds[i] = ColorFromPalette(myPal, heatindex);
    }
  }

  Sun::Sun() {
    location.x = 0.5;
    location.z = 0.5;
    location.r = 1;
    myPal = heatmap_gp;
  }

  void Sun::Move() {
    location.y += -0.005;
    location.phi += -0.1;
    if (location.phi > 2 * PI) {
      location.phi -= 2 * PI;
    }
    uint8 theta = UINT8_MAX * location.phi / (2 * PI);
    location.x = (cos8(theta) / (float)UINT8_MAX) * location.r;
    location.z = (sin8(theta) / (float)UINT8_MAX) * location.r;
  }