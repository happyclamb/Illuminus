#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <Arduino.h>
#include <FastLED.h>

#include "IlluminusDefs.h"
#include "RadioManager.h"

class LightManager
{
	public:
		LightManager(RadioManager& _radioMan);
		void init();
		byte getPattern();
		void setPattern(byte newPattern);
		byte getPatternParam();
		void setPatternParam(byte newPatternParam1);
		void chooseNewPattern(); // called from server

		void redrawLights();

	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		RadioManager& radioMan;
		byte pattern;
		byte pattern_param1;

		CRGB colorFromWheelPosition(byte wheelPos);
		void colorFromWheelPosition(byte wheelPos, byte *r, byte *g, byte *b);

		void updateLEDArrayFromCurrentPattern();
		void debugPattern();
		void syncFade();
		void staggeredFade();
};

#endif // __LIGHTMANAGER_H__
