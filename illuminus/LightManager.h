#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <Arduino.h>
#include <FastLED.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class LightPattern {
	public:
		byte pattern;
		byte pattern_param1;
		byte pattern_param2;
};

enum LightPatternTimingOptions {
			PATTERN_TIMING_NONE,
			PATTERN_TIMING_STAGGER,
			PATTERN_TIMING_ALTERNATE,
			PATTERN_TIMING_SYNC };

class LightManager {
	public:
		LightManager(SingletonManager* _singleMan);

		LightPattern getNextPattern();
		unsigned long getNextPatternStartTime();
		void setNextPattern(LightPattern newPattern, unsigned long startTime);
		void chooseNewPattern(); // called from server
		void redrawLights();

	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		SingletonManager* singleMan = NULL;
		LightPattern currPattern;
		LightPattern nextPattern;
		unsigned long nextPatternStartTime = 0;

		LightPattern getPattern();
		void setPattern(LightPattern newPattern);

		CRGB colorFromWheelPosition(byte wheelPos);
		void colorFromWheelPosition(byte wheelPos, byte *r, byte *g, byte *b);

		void checkForPatternUpdate();
		void noAddressPattern();
		void updateLEDArrayFromCurrentPattern();
		void debugPattern();
		void solidWheelColorChange(LightPatternTimingOptions timingType, bool allLaternLEDs);
		void comet();
};

#endif // __LIGHTMANAGER_H__
