#include "FastLED.h"
#include "geometry.h"



class Sun {
 public:
  Sun();
  void Update();

 private:
  Point location;
  CRGBPalette16 myPal;

  void Move();
};
