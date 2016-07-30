#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <Arduino.h>
#include <FastLED.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class LightPattern {
	public:
		byte pattern = 0;
		byte pattern_param1 = 1;
		unsigned long startTime = 0;

		void update(LightPattern newPattern) {
			this->pattern = newPattern.pattern;
			this->pattern_param1 = newPattern.pattern_param1;
			this->startTime = newPattern.startTime;
		}

		void printPattern() {
			info_print("pattern: ");
			info_print(this->pattern);
			info_print("      pattern_param1: ");
			info_print(this->pattern_param1);
			info_print("      startTime: ");
			info_print(this->startTime);
		}
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
		void setNextPattern(LightPattern newPattern);

		void chooseNewPattern(); // called from server
		void redrawLights(); // called from interrupt handler

	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		SingletonManager* singleMan = NULL;
		LightPattern currPattern;
		LightPattern nextPattern;

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
