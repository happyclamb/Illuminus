#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <Arduino.h>
#include <FastLED.h>

#include "RadioManager.h"

#define NUM_RGB_LEDS 6

class LightManager
{
  public:
    LightManager(RadioManager& _radioMan);
    void init();
    void updateLights();
  private:
    CRGB ledstrip[NUM_RGB_LEDS];
    RadioManager& radioMan;
};

#endif // __LIGHTMANAGER_H__
