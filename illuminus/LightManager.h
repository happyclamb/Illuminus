#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#define NUM_RGB_LEDS 6
#define COLOR_STEPS_IN_WHEEL 255

#include <Arduino.h>
#include <FastLED.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class LightPattern {
	public:
		LightPattern(){};
		LightPattern(byte init_pattern, byte init_param1, byte init_param2,
			byte init_param3, byte init_param4, byte init_param5, unsigned long init_startTime):
			pattern(init_pattern), pattern_param1(init_param1), pattern_param2(init_param2), pattern_param3(init_param3),
			pattern_param4(init_param4), pattern_param5(init_param5), startTime(init_startTime){};

		byte pattern = 0;
		byte pattern_param1 = 0;
		byte pattern_param2 = 0;
		byte pattern_param3 = 0;
		byte pattern_param4 = 0;
		byte pattern_param5 = 0;

		unsigned long startTime = 0;

		void update(LightPattern* newPattern);
		void printPattern(SingletonManager* singleMan, bool forcePrint = false);
};

enum LightPatternTimingOptions {
			PATTERN_TIMING_NONE,
			PATTERN_TIMING_STAGGER,
			PATTERN_TIMING_ALTERNATE,
			PATTERN_TIMING_SYNC };

class LightManager {
	public:
		LightManager(SingletonManager* _singleMan);

		bool getManualMode() { return this->manual_mode; }
		void setManualMode(bool newMode) { this->manual_mode = newMode; }

		unsigned long getPatternDuration() { return this->pattern_duration; }
		void setPatternDuration(unsigned long newDuration) { this->pattern_duration = newDuration; }

		void setBigLightBrightness(byte brightness);

		LightPattern* getNextPattern();
		void setNextPattern(LightPattern* newPattern, bool forcePrint = false);

		void chooseNewPattern(unsigned long nextPatternTimeOffset = 0); // called from server
		void redrawLights(); // called from interrupt handler

	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		SingletonManager* singleMan = NULL;
		bool manual_mode = false;
		LightPattern* currPattern;
		LightPattern* nextPattern;

		byte number_patterns_defined = 4;
		unsigned long pattern_duration = 30000;

		CRGB colorFromWheelPosition(byte wheelPos, float brightness=1.0);
		void colorFromWheelPosition(byte wheelPos, byte *r, byte *g, byte *b, float brightness=1.0);

		void checkForPatternUpdate();
		void noAddressPattern();
		void updateLEDArrayFromCurrentPattern();
		void debugPattern();
		void solidWheelColorChange(LightPatternTimingOptions timingType,
			int patternSpeed, int brightnessSpeed, bool allLaternLEDs);
		void comet();
};

#endif // __LIGHTMANAGER_H__
