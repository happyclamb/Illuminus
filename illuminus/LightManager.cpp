#include "LightManager.h"

#include "Utils.h"
#include "PinDefns.h"

#include "RadioManager.h"
#include <FastLED.h>

LightManager::LightManager(RadioManager& _radioMan)
	:radioMan(_radioMan) {
}

void LightManager::init() {
	// Init RTG
  FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);
  for(int i=0; i<NUM_RGB_LEDS; i++)
      ledstrip[i] = CRGB(0,0,0);
  FastLED.show();

  // Turn on the big LED at 50%
  pinMode(BIG_LED_PIN, OUTPUT);
  int bigLEDBrightness = 255 * 0.5;
  analogWrite(BIG_LED_PIN, bigLEDBrightness);
}

void LightManager::updateLights() {
  unsigned long currTime = radioMan.getAdjustedMillis();

  // Over 2000ms break into '10' segments
  int litIndex = (currTime%2000)/200;

  // If the index is higher than 5, minus the offset
  // ie; 7-> 7-5*2 == 4 ... 3 should be lit
  if (litIndex > 5)
    litIndex -= (litIndex-5)*2;

  for(int i=0; i<NUM_RGB_LEDS; i++)
  {
    if(litIndex == i)
      ledstrip[i] = CRGB(0,25,25);
    else
      ledstrip[i] = CRGB(0,0,0);
  }
  FastLED.show();
}
