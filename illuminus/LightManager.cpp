#include "LightManager.h"

#include <Arduino.h>
#include <FastLED.h>		// http://fastled.io/docs/3.1/annotated.html

#include "IlluminusDefs.h"
#include "SingletonManager.h"

LightManager::LightManager(SingletonManager* _singleMan):
	singleMan(_singleMan) {

	currPattern.pattern = 0;
	currPattern.pattern_param1 = 0;
	currPattern.pattern_param2 = 0;

	nextPattern.pattern = 0;
	nextPattern.pattern_param1 = 0;
	nextPattern.pattern_param2 = 0;

	// Init RTG
	FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);
	for(int i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = CRGB(0,0,0);
	FastLED.show();

	// Turn on the big LED at 100%
	pinMode(BIG_LED_PIN, OUTPUT);
	byte bigLEDBrightness = 255 * 1.00;
	analogWrite(BIG_LED_PIN, bigLEDBrightness);

	singleMan->setLightMan(this);
}

LightPattern LightManager::getPattern() {
	return this->currPattern;
}

void LightManager::setPattern(LightPattern newPattern) {
	this->currPattern.pattern = newPattern.pattern;
	this->currPattern.pattern_param1 = newPattern.pattern_param1;
	this->currPattern.pattern_param2 = newPattern.pattern_param2;
}

LightPattern LightManager::getNextPattern() {
	return(this->nextPattern);
}

unsigned long LightManager::getNextPatternStartTime() {
	return(this->nextPatternStartTime);
}

void LightManager::setNextPattern(LightPattern newPattern, unsigned long startTime) {
	this->nextPattern.pattern = newPattern.pattern;
	this->nextPattern.pattern_param1 = newPattern.pattern_param1;
	this->nextPattern.pattern_param2 = newPattern.pattern_param2;

	this->nextPatternStartTime = startTime;
}

void LightManager::chooseNewPattern() {
	static unsigned long lastPatternChangeTime = 0;

	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
	if(currTime > lastPatternChangeTime + FORCE_PATTERN_CHANGE)
	{
		this->nextPattern.pattern = random(0,LIGHT_PATTERNS_DEFINED);
		this->nextPattern.pattern_param1 = this->currPattern.pattern_param1;
		this->nextPattern.pattern_param2 = this->currPattern.pattern_param2;
		nextPatternStartTime = singleMan->radioMan()->getAdjustedMillis() + PATTERN_CHANGE_DELAY;

		lastPatternChangeTime = currTime;
	}
	else
	{
		// check to see if the pattern_param should be updated...
		#define TimeBetweenDebugPatternColorChange 1000
		unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

		static unsigned long colorChoice = 0;
		if(currTime > colorChoice + TimeBetweenDebugPatternColorChange) {
			this->nextPattern.pattern_param1++;
			nextPatternStartTime = singleMan->radioMan()->getAdjustedMillis() + PATTERN_CHANGE_DELAY;
		}

	}
}

//*********
//*********  ColorManipulation Utils *******
//*********
CRGB LightManager::colorFromWheelPosition(byte wheelPos)
{
	byte r=0;
	byte g=0;
	byte b=0;
	colorFromWheelPosition(wheelPos, &r, &g, &b);
	return CRGB(r, g, b);
}
void LightManager::colorFromWheelPosition(byte wheelPos, byte *r, byte *g, byte *b)
{
	if (wheelPos < 85) {
		// 0-->85
		*r = wheelPos * 3;
		*g = 255 - wheelPos * 3;
		*b = 0;
	} else if (wheelPos < 170) {
		// 85-->170
		wheelPos -= 85;
		*r = 255 - wheelPos * 3;
		*g = 0;
		*b = wheelPos * 3;
	} else {
		// 170-->255
		wheelPos -= 170;
		*r = 0;
		*g = wheelPos * 3;
		*b = 255 - wheelPos * 3;
	}
}

//*********
//*********  Everything from here forward runs on interrupt !! *******
//*********
void LightManager::redrawLights() {
	// If there is no address gently pulse blue light
	if(singleMan->addrMan()->hasAddress() == false) {
		noAddressPattern();
	} else {
		checkForPatternUpdate();
		updateLEDArrayFromCurrentPattern();
	}

	FastLED.show();
}

void LightManager::noAddressPattern() {
	int fadeIndex = ( millis() % (300*7) ) / 7;

	if(fadeIndex > 150)
		fadeIndex = 150 - (fadeIndex-150);

	for(int i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = CRGB(0,0,fadeIndex);
}

void LightManager::checkForPatternUpdate() {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
	if(currTime > nextPatternStartTime) {
		this->currPattern.pattern = this->nextPattern.pattern;
		this->currPattern.pattern_param1 = this->nextPattern.pattern_param1;
		this->currPattern.pattern_param2 = this->nextPattern.pattern_param2;
	}
}

void LightManager::updateLEDArrayFromCurrentPattern()
{
	switch(this->currPattern.pattern) {
		case 0: debugPattern(); break;
		case 1: solidWheelColorChange(PATTERN_TIMING_NONE, true); break;
		case 2: solidWheelColorChange(PATTERN_TIMING_STAGGER, true); break;
		case 3: solidWheelColorChange(PATTERN_TIMING_SYNC, true); break;
		case 4: solidWheelColorChange(PATTERN_TIMING_NONE, false); break;
		case 5: solidWheelColorChange(PATTERN_TIMING_STAGGER, false); break;
		case 6: solidWheelColorChange(PATTERN_TIMING_SYNC, false); break;
		case 7: comet(); break;
	}
}

void LightManager::debugPattern() {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

	// Over 2000ms break into 200ms sections (10 total) segments
	byte litIndex = (currTime%(1200))/200;

	CRGB paramColor;
	switch(this->currPattern.pattern_param1%3) {
		case 0: paramColor = CRGB(75,0,0); break;
		case 1: paramColor = CRGB(0,75,0); break;
		case 2: paramColor = CRGB(0,0,75); break;
	}

	for(int i=0; i<NUM_RGB_LEDS; i++) {
		if(litIndex == i)
			ledstrip[i] = paramColor;
		else
			ledstrip[i] = CRGB(0,0,0);
	}
}

// Treat all 6 LEDs as 1 solid colour, default pattern look is to have all
//  the lanterns showing exactly the same pattern PATTERN_TIMING_SYNC
//	if (allLaternLEDs) then make all 6 LEDs the same
// MATH: cycle through all 255 wheel positions over ~3sec
// == 3000/255 == 11.7 ... make a step 12ms == 12*255 == 3060
void LightManager::solidWheelColorChange(LightPatternTimingOptions timingType, bool allLaternLEDs) {
	unsigned long currTime = 0;
	if(timingType == PATTERN_TIMING_NONE)
		currTime = millis();
	else
		currTime = singleMan->radioMan()->getAdjustedMillis();

	// Over 255position*12ms broken into segements
	byte wheelPos = (currTime%(COLOR_STEPS_IN_WHEEL*COLOR_TIME_BETWEEN_WHEEL_STEPS))/COLOR_TIME_BETWEEN_WHEEL_STEPS;

	// Now handle the stagger between sentries.
	byte thisSentryOffset = 0;
	if(timingType == PATTERN_TIMING_STAGGER) {
		byte wheelSentryPositionOffsetAmount = COLOR_STEPS_IN_WHEEL / singleMan->addrMan()->getLanternCount();
		thisSentryOffset = singleMan->addrMan()->getAddress() * wheelSentryPositionOffsetAmount;
	}

	//	CRGB wheelColor = colorFromWheelPosition(wheelPos+thisSentryOffset);
	byte baseWheel = wheelPos+thisSentryOffset;
	byte offsetForLanternLeds = COLOR_STEPS_IN_WHEEL / NUM_RGB_LEDS;
	for(int i=0; i<NUM_RGB_LEDS; i++) {
		byte nextStep = baseWheel;
		if(allLaternLEDs == false)
			nextStep += (offsetForLanternLeds/2) + (i*offsetForLanternLeds);
		ledstrip[i] = colorFromWheelPosition(nextStep);
	}
}

#define COMET_SPEED 750
void LightManager::comet()
{
	// Want to dim each light from 255->0   over (~255ms*2)
	// Light moves at about 255ms / light. == MOVE_SPEED
	// 8 lights * 255 + 1 segent of all black + 2 segment final fade ==  (NUMBER_SENTRIES + 3) * MOVE_SPEED

	byte numberOfSteps = singleMan->addrMan()->getLanternCount() + 3; // 1 blank extra and 2 fades
	unsigned long totalPatternTime = numberOfSteps * COMET_SPEED;
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
	byte currentPatternSegment = (currTime % totalPatternTime)/COMET_SPEED;

	long timeIntoASegment = (currTime % totalPatternTime) - (currentPatternSegment * COMET_SPEED);

	byte numberOfColorDecreaseSteps = 85;
	byte brightnessLevel = ((COMET_SPEED-timeIntoASegment)*numberOfColorDecreaseSteps)/COMET_SPEED;

	byte address = singleMan->addrMan()->getAddress();
	if(currentPatternSegment == (address+1))
		brightnessLevel = brightnessLevel + 170;
	else if(currentPatternSegment == (address+2))
		brightnessLevel = brightnessLevel + 85;
	else if(currentPatternSegment == (address+3))
		brightnessLevel = brightnessLevel;
	else
		brightnessLevel = 0;

	for(int i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = CRGB(brightnessLevel,brightnessLevel,brightnessLevel);
}
