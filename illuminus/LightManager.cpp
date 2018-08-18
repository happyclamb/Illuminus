#include "LightManager.h"

#include <Arduino.h>
#include <FastLED.h>		// http://fastled.io/docs/3.1/annotated.html

#include "IlluminusDefs.h"
#include "OutputManager.h"
#include "SingletonManager.h"

void LightPattern::update(LightPattern* newPattern) {
	this->pattern = newPattern->pattern;

	this->pattern_param1 = newPattern->pattern_param1;
	this->pattern_param2 = newPattern->pattern_param2;
	this->pattern_param3 = newPattern->pattern_param3;
	this->pattern_param4 = newPattern->pattern_param4;
	this->pattern_param5 = newPattern->pattern_param5;

	this->startTime = newPattern->startTime;
}

void LightPattern::printPattern(SingletonManager* singleMan, OUTPUT_LOG_TYPES log_level)  {
	if (singleMan->outputMan()->isLogLevelEnabled(log_level)) {
		singleMan->outputMan()->print(log_level, F("pattern: "));
		singleMan->outputMan()->print(log_level, this->pattern);
		singleMan->outputMan()->print(log_level, F("      pattern_param1: "));
		singleMan->outputMan()->print(log_level, this->pattern_param1);
		singleMan->outputMan()->print(log_level, F("      pattern_param2: "));
		singleMan->outputMan()->print(log_level, this->pattern_param2);
		singleMan->outputMan()->print(log_level, F("      pattern_param3: "));
		singleMan->outputMan()->print(log_level, this->pattern_param3);
		singleMan->outputMan()->print(log_level, F("      pattern_param4: "));
		singleMan->outputMan()->print(log_level, this->pattern_param4);
		singleMan->outputMan()->print(log_level, F("      pattern_param5: "));
		singleMan->outputMan()->print(log_level, this->pattern_param5);
		singleMan->outputMan()->print(log_level, F("      startTime: "));
		singleMan->outputMan()->print(log_level, this->startTime);
	}
}

LightManager::LightManager(SingletonManager* _singleMan):
	singleMan(_singleMan),
	currPattern(new LightPattern()),
	nextPattern(new LightPattern())
{

	// Init RTG
	FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);
	for(int i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = CRGB(0,0,0);
	FastLED.show();

	// Turn on the big LED at 100%
	pinMode(BIG_LED_PIN, OUTPUT);
	setBigLightBrightness(255);
	singleMan->setLightMan(this);
}

void LightManager::setBigLightBrightness(byte brightness) {
	singleMan->outputMan()->print(LOG_INFO, F("setBigLightBrightness:  "));
	singleMan->outputMan()->println(LOG_INFO, brightness);
	analogWrite(BIG_LED_PIN, brightness);
}

// The currently selected NextPattern
LightPattern* LightManager::getNextPattern() {
	return(this->nextPattern);
}

// NextPattern is passed to Sentries via radio messages
void LightManager::setNextPattern(LightPattern* newPattern, OUTPUT_LOG_TYPES log_level) {
	singleMan->outputMan()->println(log_level, F("Setting Next Pattern"));
	newPattern->printPattern(singleMan, log_level);

	this->nextPattern->update(newPattern);
}

void LightManager::chooseNewPattern(unsigned long nextPatternTimeOffset /*= 0*/) {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

	if((this->manual_mode == false) &&
		(this->currPattern->startTime >= this->nextPattern->startTime))
	{
		singleMan->outputMan()->print(LOG_DEBUG, F("ChooseNewPattern >"));
		this->nextPattern->pattern = random(1, this->number_patterns_defined+1);
		this->nextPattern->pattern_param1 = random(1, 4)*20;
		this->nextPattern->pattern_param2 = random(0, 3)*5;
		this->nextPattern->pattern_param3 = random(1, 4);
		this->nextPattern->pattern_param4 = random(1, 4);
		this->nextPattern->pattern_param5 = random(1, 4);
		this->nextPattern->startTime = currTime +
			(nextPatternTimeOffset > 0 ? nextPatternTimeOffset : this->getPatternDuration());

		this->nextPattern->printPattern(singleMan, LOG_DEBUG);
	}
}

//*********
//*********  ColorManipulation Utils *******
//*********
CRGB LightManager::colorFromWheelPosition(byte wheelPos, float brightness)
{
	byte r=0;
	byte g=0;
	byte b=0;
	colorFromWheelPosition(wheelPos, &r, &g, &b, brightness);
	return CRGB(r, g, b);
}
void LightManager::colorFromWheelPosition(byte wheelPos,
	byte *r, byte *g, byte *b, float brightness)
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

	*r = (brightness * (float)*r);
	*g = (brightness * (float)*g);
	*b = (brightness * (float)*b);
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
	if((currTime > this->nextPattern->startTime) &&
		(this->nextPattern->startTime != this->currPattern->startTime))
	{
		this->currPattern->update(this->nextPattern);
	}
}

void LightManager::updateLEDArrayFromCurrentPattern()
{
	switch(this->currPattern->pattern) {
		case 0:
			debugPattern();
			break;
		case 1:
			solidWheelColorChange(PATTERN_TIMING_SYNC, currPattern->pattern_param1,
				currPattern->pattern_param2, currPattern->pattern_param3 > 2 ? true : false);
			break;
		case 2:
			solidWheelColorChange(PATTERN_TIMING_STAGGER, currPattern->pattern_param1,
				currPattern->pattern_param2, currPattern->pattern_param3 > 2 ? true : false);
			break;
		case 3:
			solidWheelColorChange(PATTERN_TIMING_ALTERNATE, currPattern->pattern_param1,
				currPattern->pattern_param2, currPattern->pattern_param3 > 2 ? true : false);
			break;
		case 4:
			solidWheelColorChange(PATTERN_TIMING_NONE, currPattern->pattern_param1,
				currPattern->pattern_param2, currPattern->pattern_param3 > 2 ? true : false);
			break;
		case 5:
			comet();
			break;
		case 10:
			solidColor(currPattern->pattern_param1);
			break;
	}
}

void LightManager::debugPattern() {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

	// Over 2000ms break into 200ms sections (10 total) segments
	byte litIndex = (currTime%(1200))/200;

	// 3 segements at 1200s each
	byte colorIndex = (currTime%(3600))/1200;

	CRGB paramColor;
	switch(colorIndex) {
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

void LightManager::solidColor(byte wheelPos) {
	CRGB newColor = colorFromWheelPosition(wheelPos);
	for(int i=0; i<NUM_RGB_LEDS; i++) {
		ledstrip[i] = newColor;
	}
}

// Treat all 6 LEDs as 1 solid colour, default pattern look is to have all
//  the lanterns showing exactly the same pattern PATTERN_TIMING_SYNC
//	if (allLaternLEDs) then make all 6 LEDs the same
// MATH: cycle through all 255 wheel positions over ~3sec
// == 3000/255 == 11.7 ... make a step 12ms == 12*255 == 3060
void LightManager::solidWheelColorChange(LightPatternTimingOptions timingType,
	int patternSpeed, int brightnessSpeed, bool allLaternLEDs) {
	unsigned long currTime = 0;
	if(timingType == PATTERN_TIMING_NONE)
		currTime = millis();
	else
		currTime = singleMan->radioMan()->getAdjustedMillis();

	/*	brightnessFade */
	float cosBright = 1.0;
	if(brightnessSpeed > 0) {
			/*		cos(rad)		Calculates the cos of an angle (in radians). The result will be between -1 and 1.
			cos(0) == 1
			cos(3.14) == -1
			cos(6.28) == 1
			*/
		int totalBrightSteps = 314;
		int brightnessAngleIndex = (currTime%(totalBrightSteps*brightnessSpeed))/brightnessSpeed;
		cosBright = cos(brightnessAngleIndex/50.0) + 1.0;

		// Now the range is from 0.0 -> 2.0
		// Lets scale it
		//	0.33 -> 1.0    cosBright/3.0 + 0.33
		//	0.25 -> 1.0    cosBright/2.6 + 0.25
		//	0.20 -> 1.0    cosBright/2.5 + 0.20
		cosBright = cosBright/2.5 + 0.20;
	}

	byte colorTimeBetweenSteps = patternSpeed;
	byte wheelPos = (currTime%(COLOR_STEPS_IN_WHEEL*colorTimeBetweenSteps))/colorTimeBetweenSteps;

	// Now handle the stagger between sentries.
	byte thisSentryOffset = 0;
	if(timingType == PATTERN_TIMING_STAGGER) {
		byte wheelSentryPositionOffsetAmount = COLOR_STEPS_IN_WHEEL  / singleMan->healthMan()->totalSentries() ;
		thisSentryOffset = singleMan->addrMan()->getAddress() * wheelSentryPositionOffsetAmount;
	} else if(timingType == PATTERN_TIMING_ALTERNATE) {
		if(singleMan->addrMan()->getAddress()%2 == 0)
			thisSentryOffset = 128;
	}

	byte baseWheel = wheelPos+thisSentryOffset;
	byte offsetForLanternLeds = COLOR_STEPS_IN_WHEEL / NUM_RGB_LEDS;
	for(int i=0; i<NUM_RGB_LEDS; i++) {
		byte nextStep = baseWheel;
		if(allLaternLEDs == false)
			nextStep += (offsetForLanternLeds/2) + (i*offsetForLanternLeds);

		ledstrip[i] = colorFromWheelPosition(nextStep, cosBright);
	}
}

#define COMET_SPEED 750
void LightManager::comet()
{
	// Want to dim each light from 255->0   over (~255ms*2)
	// Light moves at about 255ms / light. == MOVE_SPEED
	// 8 lights * 255 + 1 segent of all black + 2 segment final fade ==  (NUMBER_SENTRIES + 3) * MOVE_SPEED

	byte numberOfSteps = singleMan->healthMan()->totalSentries() + 3; // 1 blank extra and 2 fades
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
