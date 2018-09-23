#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <FastLED.h>		// http://fastled.io/docs/3.1/annotated.html

#include "SingletonManager.h"
class SingletonManager;

#define NUM_RGB_LEDS 6
#define COLOR_STEPS_IN_WHEEL 255

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
		void printlnPattern(SingletonManager* singleMan, OUTPUT_LOG_TYPES log_level);
};

class LightManager {
	public:
		LightManager(SingletonManager* _singleMan);

		bool getManualMode() { return this->manual_mode; }
		void setManualMode(bool newMode) { this->manual_mode = newMode; }

		unsigned long getPatternDuration() { return this->pattern_duration; }
		void setPatternDuration(unsigned long newDuration) { this->pattern_duration = newDuration; }

		void setBigLightBrightness(byte brightness);

		LightPattern* getCurrPattern() { return this->currPattern; }
		LightPattern* getNextPattern() { return this->nextPattern; }
		void setNextPattern(LightPattern* newPattern, OUTPUT_LOG_TYPES log_level);

		void chooseNewPattern(unsigned long nextPatternTimeOffset = 0); // called from server
		void redrawLights(); // called from interrupt handler

	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		SingletonManager* singleMan = NULL;
		bool manual_mode = false;
		LightPattern* currPattern;
		LightPattern* nextPattern;

		byte number_patterns_defined = 5;
		unsigned long pattern_duration = 59999; // closest prime to 1 min

		CRGB colorFromWheelPosition(byte wheelPos, float brightness=1.0);
		void colorFromWheelPosition(byte wheelPos, byte *r, byte *g, byte *b, float brightness=1.0);
		float cosFade(unsigned long currTime, byte brightnessSpeed);

		void checkForPatternUpdate();
		void initializingPattern(byte init_state);
		void updateLEDArrayFromCurrentPattern();

		void bananaGuard();
		void solidColor(byte wheelPos, byte brightness, byte sentyRequested);
		void solidWheelColorChange(LightPatternTimingOptions timingType,
			byte patternSpeed, byte brightnessSpeed, byte insideColors);
		void walkingLights(byte patternSpeed, byte brightnessSpeed, byte initialBackground);
};

#endif // __LIGHTMANAGER_H__
