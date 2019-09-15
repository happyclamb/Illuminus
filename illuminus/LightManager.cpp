#include "LightManager.h"


void LightPattern::update(LightPattern* newPattern) {
	this->pattern = newPattern->pattern;

	this->pattern_param1 = newPattern->pattern_param1;
	this->pattern_param2 = newPattern->pattern_param2;
	this->pattern_param3 = newPattern->pattern_param3;
	this->pattern_param4 = newPattern->pattern_param4;
	this->pattern_param5 = newPattern->pattern_param5;

	this->startTime = newPattern->startTime;
}


void LightPattern::printlnPattern(SingletonManager* singleMan, OUTPUT_LOG_TYPES log_level)  {
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
		singleMan->outputMan()->println(log_level, this->startTime);
	}
}


LightManager::LightManager(SingletonManager* _singleMan):
	singleMan(_singleMan),
	currPattern(new LightPattern()),
	nextPattern(new LightPattern())
{

	// Init RTG
	FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);
	for(byte i=0; i<NUM_RGB_LEDS; i++)
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


// NextPattern is passed to Sentries via radio messages
void LightManager::setNextPattern(LightPattern* newPattern, OUTPUT_LOG_TYPES log_level) {
	singleMan->outputMan()->println(log_level, F("Setting Next Pattern"));
	newPattern->printlnPattern(singleMan, log_level);

	this->nextPattern->update(newPattern);
}


void LightManager::chooseNewPattern(unsigned long nextPatternTimeOffset /*= 0*/) {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

	if((singleMan->inputMan()->isInteractiveMode() == false) &&
		(this->currPattern->startTime >= this->nextPattern->startTime))
	{
		// Increment through all the patterns, on loop roll to 1 (as 0 is 'waiting' pattern)
		this->nextPattern->pattern = this->currPattern->pattern + 1;
		if(this->nextPattern->pattern > this->number_patterns_defined)
			this->nextPattern->pattern = 1;

		this->nextPattern->pattern_param1 = random(1, 4)*42;
		this->nextPattern->pattern_param2 = random(0, 3)*21;
		this->nextPattern->pattern_param3 = random(0, 4);
		this->nextPattern->pattern_param4 = 255;
		this->nextPattern->pattern_param5 = random(1, 4);
		this->nextPattern->startTime = currTime +
			(nextPatternTimeOffset > 0 ? nextPatternTimeOffset : this->getPatternDuration());

		singleMan->outputMan()->print(LOG_DEBUG, F("NewPattern >"));
		this->nextPattern->printlnPattern(singleMan, LOG_DEBUG);
	} else if(singleMan->inputMan()->isInteractiveMode() == true &&
		singleMan->inputMan()->hasUnhandledInput()) {

		// next pattern is based on existing pattern
		this->nextPattern->update(this->currPattern);

		// startTime is as soon as possible
		this->nextPattern->startTime = currTime;

		// Allow forward
		bool button_1_pressed = singleMan->inputMan()->isButtonPressed(1);
		if (button_1_pressed) {
			this->nextPattern->pattern = this->nextPattern->pattern + (button_1_pressed ? 1 : 0);
			if(this->nextPattern->pattern > this->number_patterns_defined)
				this->nextPattern->pattern = 1;
		}

		// And backwards
		bool button_0_pressed = singleMan->inputMan()->isButtonPressed(0);
		if (button_0_pressed) {
			this->nextPattern->pattern = this->nextPattern->pattern - (button_0_pressed ? 1 : 0);
			if(this->nextPattern->pattern == 0 || this->nextPattern->pattern > this->number_patterns_defined)
				this->nextPattern->pattern = this->number_patterns_defined;
		}

		// Bananananna Guard
		if (singleMan->inputMan()->isButtonPressed(3)) {
			this->nextPattern->pattern = 0;
		}

		// Such Randoms
		if (singleMan->inputMan()->isButtonPressed(4)) {
			byte currPattern = this->nextPattern->pattern;
			while (currPattern == this->nextPattern->pattern) {
				this->nextPattern->pattern = random(0, this->number_patterns_defined+1);
			}
		}

		// Pattern Speed
		this->nextPattern->pattern_param1 = map(singleMan->inputMan()->getAnalog(0), 0, 255, 200, 20);

		// BrightnessFade speed
		this->nextPattern->pattern_param2 = singleMan->inputMan()->getAnalog(1);
		if (this->nextPattern->pattern_param2 > 0) {
			// Need to reverse the values except if it's zero as that means stop
			this->nextPattern->pattern_param2 = map(this->nextPattern->pattern_param2, 1, 255, 70, 1);
		}

		// And sync brightness
		bool button_2_pressed = singleMan->inputMan()->isButtonPressed(2);
		this->nextPattern->pattern_param3 = this->nextPattern->pattern_param3 + (button_2_pressed ? 1 : 0);
		if(this->nextPattern->pattern_param3 == 4)
			this->nextPattern->pattern_param3 = 0;

		// Brightness Scale
		this->nextPattern->pattern_param4 = singleMan->inputMan()->getAnalog(2);

		// This is only used by pattern 9 - just leave as random for now
		this->nextPattern->pattern_param5 = random(1, 4);

		singleMan->outputMan()->print(LOG_DEBUG, F("NewPattern >"));
		this->nextPattern->printlnPattern(singleMan, LOG_DEBUG);
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

/*  Halloween version
Green:
	red:   25->110
	green: 220 -> 155
	blue:  0
Orange:
	red:   170->255
	green: 85 -> 0
	blue:  0
Purple:
	red:   155->70
	green: 0
	blue:  100->185
void LightManager::colorFromWheelPosition(byte wheelPos,
	byte *r, byte *g, byte *b, float brightness)
{
	if (wheelPos < 85) {
		// 0-->85
		*r = 25 + wheelPos;
		*g = 220 - wheelPos;
		*b = 0;
	} else if (wheelPos < 170) {
		// 85-->170
		wheelPos -= 85;
		*r = 170 + wheelPos;
		*g = 85 - wheelPos;
		*b = 0;
	} else {
		// 170-->255
		wheelPos -= 170;
		*r = 155 - wheelPos;
		*g = 0;
		*b = 100 + wheelPos;
	}

	*r = (brightness * (float)*r);
	*g = (brightness * (float)*g);
	*b = (brightness * (float)*b);
}
*/

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


float LightManager::cosFade(float scaledBrightness, byte brightnessSpeed, bool syncedBrightness) {
	unsigned long currTime;
	if(syncedBrightness)
		currTime = singleMan->radioMan()->getAdjustedMillis();
	else
		currTime = millis();

	float cosBright = 1.0;
	if(brightnessSpeed > 0) {
		/*		cos(rad)		Calculates the cos of an angle (in radians). The result will be between -1 and 1.
		cos(0) == 1
		cos(3.14) == -1
		cos(6.28) == 1
		*/
		int totalBrightSteps = 314;
		int brightnessAngleIndex = (currTime%(totalBrightSteps*(int)brightnessSpeed))/(int)brightnessSpeed;
		cosBright = cos(brightnessAngleIndex/50.0) + 1.0;

		// Now the range is from 0.0 -> 2.0
		// Lets scale it
		//	0.33 -> 1.0    cosBright/3.0 + 0.33
		//	0.25 -> 1.0    cosBright/2.6 + 0.25
		//	0.20 -> 1.0    cosBright/2.5 + 0.20
		cosBright = cosBright/2.5 + 0.20;
	}

	return cosBright*scaledBrightness;
}


//*********
//*********  Everything from here forward runs on interrupt !! *******
//*********
void LightManager::redrawLights() {

	if(singleMan->addrMan()->hasAddress() == false) {
		initializingPattern(0);
	}
	else if (singleMan->radioMan()->getMillisOffset() == 0
		&& singleMan->healthMan()->getServerAddress() != singleMan->addrMan()->getAddress())
	{
		initializingPattern(1);
	}
	else
	{
		checkForPatternUpdate();
		updateLEDArrayFromCurrentPattern();
	}

	FastLED.show();
}


void LightManager::initializingPattern(byte init_state) {
	int fadeIndex = ( millis() % (300*7) ) / 7;

	if(fadeIndex > 150)
		fadeIndex = 150 - (fadeIndex-150);

	CRGB fadeColor;
	// If there is no address gently pulse blue light
	switch (init_state) {
		case 0: fadeColor = CRGB(0,0,fadeIndex); break;
		case 1: fadeColor = CRGB(0,fadeIndex,0); break;
	}

	for(byte i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = fadeColor;
}


void LightManager::checkForPatternUpdate() {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
	if((currTime > this->nextPattern->startTime) &&
		(this->nextPattern->startTime != this->currPattern->startTime))
	{
		this->currPattern->update(this->nextPattern);
	}
}


void LightManager::updateLEDArrayFromCurrentPattern() {

	float initBrightness = ((float)map(currPattern->pattern_param4, 0, 255, 200, 1000)) / 1000;

	switch(this->currPattern->pattern) {
		case 0:
			bananaGuard(initBrightness);
			break;

		case 1:
			solidWheelColorChange(PATTERN_TIMING_SYNC, INSIDE_SOLID,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 2:
			solidWheelColorChange(PATTERN_TIMING_STAGGER, INSIDE_SOLID,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 3:
			solidWheelColorChange(PATTERN_TIMING_SYNC, INSIDE_CANDY_CANE,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 4:
			solidWheelColorChange(PATTERN_TIMING_NONE, INSIDE_SOLID,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 5:
			solidWheelColorChange(PATTERN_TIMING_SYNC, INSIDE_RAINBOW,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 6:
			solidWheelColorChange(PATTERN_TIMING_ALTERNATE, INSIDE_SOLID,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 7:
			solidWheelColorChange(PATTERN_TIMING_SYNC, INSIDE_HALF_HALF,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 8:
			solidWheelColorChange(PATTERN_TIMING_NONE, INSIDE_CANDY_CANE,
				currPattern->pattern_param1, currPattern->pattern_param2, initBrightness, currPattern->pattern_param3%2);
			break;
		case 9:
			walkingLights(currPattern->pattern_param1, currPattern->pattern_param2,
				currPattern->pattern_param3%2, initBrightness, 20+(currPattern->pattern_param5*10));
			break;

		case 20:
			solidColor(currPattern->pattern_param1, currPattern->pattern_param2,
				currPattern->pattern_param3, initBrightness);
			break;
	}
}


void LightManager::bananaGuard(float brightness /*=0.8*/) {
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();

	// Over 2000ms break into 200ms sections (10 total) segments
	byte litIndex = (currTime%(6*125))/125;

	// 3 segments at 1200s each
	byte colorIndex = (currTime%(3*750))/750;

	CRGB paramColor;
	switch(colorIndex) {
		case 0: paramColor = CRGB((float)255*brightness, 0, 0); break;
		case 1: paramColor = CRGB(0, (float)255*brightness, 0); break;
		case 2: paramColor = CRGB(0, 0, (float)255*brightness); break;
	}

	for(byte i=0; i<NUM_RGB_LEDS; i++) {
		if(litIndex == i)
			ledstrip[i] = paramColor;
		else
			ledstrip[i] = CRGB(0,0,0);
	}
}


void LightManager::solidColor(byte wheelPos, byte brightness, byte sentyRequested, float initialBrightness) {
	float float_brightness = ((float)map(brightness, 0, 255, 0, 100))/100;

	CRGB newColor = CRGB(0,0,0);
	if(sentyRequested == 255 || sentyRequested == singleMan->addrMan()->getAddress()) {
		newColor = colorFromWheelPosition(wheelPos, float_brightness);
	}

	for(byte i=0; i<NUM_RGB_LEDS; i++) {
		ledstrip[i] = newColor;
	}
}


// Treat all 6 LEDs as 1 solid colour, default pattern look is to have all
//  the lanterns showing exactly the same pattern PATTERN_TIMING_SYNC
//	if (allLaternLEDs) then make all 6 LEDs the same
// MATH: cycle through all 255 wheel positions over ~3sec
// == 3000/255 == 11.7 ... make a step 12ms == 12*255 == 3060
void LightManager::solidWheelColorChange(
	LightPatternTimingOptions timingType,
	LightPatternInsideColor insideColors,
	byte patternSpeed,
	byte brightnessSpeed,
	float initialBrightness,
	bool syncedBrightness)
{
	unsigned long currTime = 0;
	if(timingType == PATTERN_TIMING_NONE)
		currTime = millis();
	else
		currTime = singleMan->radioMan()->getAdjustedMillis();

	float brightnessFloat = this->cosFade(initialBrightness, brightnessSpeed, syncedBrightness);
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

	byte numColours = 0;
	if (insideColors == INSIDE_SOLID) numColours = 1;
	else if (insideColors == INSIDE_CANDY_CANE || insideColors == INSIDE_HALF_HALF) numColours = 2;
	else /* if (insideColors == INSIDE_RAINBOW) */ numColours = 6;

	byte offsetForLanternLeds = COLOR_STEPS_IN_WHEEL / numColours;
	for(byte i=0; i<NUM_RGB_LEDS; i++) {
		byte LEDcolor = baseWheel;
		if(numColours > 1) {
			if (insideColors == INSIDE_HALF_HALF)
				LEDcolor += offsetForLanternLeds*(i/(NUM_RGB_LEDS/numColours));
			else
				LEDcolor += offsetForLanternLeds*(i%numColours);
		}

		ledstrip[i] = colorFromWheelPosition(LEDcolor, brightnessFloat);
	}
}


// Walk through all the sentries and for each lantern choose a color
void LightManager::walkingLights(byte patternSpeed, byte brightnessSpeed,
	bool syncedBrightness, float initialBrightness, byte initialBackground)
{
	unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
	float brightnessFloat = this->cosFade(initialBrightness, brightnessSpeed, syncedBrightness);

	unsigned long colorTimeBetweenSteps = patternSpeed*5;
	unsigned long totalSteps = singleMan->healthMan()->totalSentries();
	byte currStep = (currTime%(totalSteps*colorTimeBetweenSteps))/colorTimeBetweenSteps;

	int bgColorComponent = brightnessFloat * (float)initialBackground;
	CRGB background = CRGB(bgColorComponent, bgColorComponent, bgColorComponent);

	bool isMe = singleMan->addrMan()->getAddress() == currStep;
	static CRGB standout;
	if(!isMe) standout = colorFromWheelPosition(random(0, COLOR_STEPS_IN_WHEEL));

	for(byte i=0; i<NUM_RGB_LEDS; i++) {
		ledstrip[i] = isMe ? standout : background;
	}
}
