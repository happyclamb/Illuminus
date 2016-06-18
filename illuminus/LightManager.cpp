#include "LightManager.h"

#include "Utils.h"
#include "IlluminusDefs.h"

#include "RadioManager.h"
#include <FastLED.h>		// http://fastled.io/docs/3.1/annotated.html

LightManager::LightManager(RadioManager& _radioMan):
	radioMan(_radioMan) {

	currPattern.pattern = 0;
	currPattern.pattern_param1 = 0;
	currPattern.pattern_param2 = 0;

	nextPattern.pattern = 0;
	nextPattern.pattern_param1 = 0;
	nextPattern.pattern_param2 = 0;

	nextPatternStartTime = 0;
}

void LightManager::init() {
	// Init RTG
	FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);
	for(int i=0; i<NUM_RGB_LEDS; i++)
		ledstrip[i] = CRGB(0,0,0);
	FastLED.show();

	// Turn on the big LED at 100%
	pinMode(BIG_LED_PIN, OUTPUT);
	byte bigLEDBrightness = 255 * 1.00;
	analogWrite(BIG_LED_PIN, bigLEDBrightness);
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

	unsigned long currTime = radioMan.getAdjustedMillis();
	if(currTime > lastPatternChangeTime + FORCE_PATTERN_CHANGE)
	{
		// increment internal pattern
		int newPatternCode = this->currPattern.pattern;
		newPatternCode++;

		if(newPatternCode == LIGHT_PATTERNS_DEFINED)
			newPatternCode = 0;

		this->nextPattern.pattern = newPatternCode;
		this->nextPattern.pattern_param1 = this->currPattern.pattern_param1;
		this->nextPattern.pattern_param2 = this->currPattern.pattern_param2;
		nextPatternStartTime = radioMan.getAdjustedMillis() + PATTERN_CHANGE_DELAY;

		lastPatternChangeTime = currTime;
	}
	else
	{
		// check to see if the pattern_param should be updated...
		#define TimeBetweenDebugPatternColorChange 1000
		unsigned long currTime = radioMan.getAdjustedMillis();

		static unsigned long colorChoice = 0;
		if(currTime > colorChoice + TimeBetweenDebugPatternColorChange) {
			this->nextPattern.pattern_param1++;
			nextPatternStartTime = radioMan.getAdjustedMillis() + PATTERN_CHANGE_DELAY;
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
	checkForPatternUpdate();
	updateLEDArrayFromCurrentPattern();
	FastLED.show();
}

void LightManager::checkForPatternUpdate() {
	unsigned long currTime = radioMan.getAdjustedMillis();
	if(currTime > nextPatternStartTime) {
		this->currPattern.pattern = this->nextPattern.pattern;
		this->currPattern.pattern_param1 = this->nextPattern.pattern_param1;
		this->currPattern.pattern_param2 = this->nextPattern.pattern_param2;
	}
}

void LightManager::updateLEDArrayFromCurrentPattern()
{
	switch(this->currPattern.pattern) {
		case 0: solidWheelColorChange(PATTERN_TIMING_NONE, true); break;
		case 1: solidWheelColorChange(PATTERN_TIMING_STAGGER, true); break;
		case 2: solidWheelColorChange(PATTERN_TIMING_SYNC, true); break;
		case 3: solidWheelColorChange(PATTERN_TIMING_NONE, false); break;
		case 4: solidWheelColorChange(PATTERN_TIMING_STAGGER, false); break;
		case 5: solidWheelColorChange(PATTERN_TIMING_SYNC, false); break;
		case 6: debugPattern(); break;
	}
}

void LightManager::debugPattern() {
	unsigned long currTime = radioMan.getAdjustedMillis();

	// Over 2000ms break into 200ms sections (10 total) segments
	int litIndex = (currTime%(2000))/200;

	// If the index is higher than 5, minus the offset
	// ie; 7-> 7-5*2 == 4 ... 3 should be lit
	if (litIndex > 5)
		litIndex -= (litIndex-5)*2;

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
		currTime = radioMan.getAdjustedMillis();

	// Over 255position*12ms broken into segements
	byte wheelPos = (currTime%(COLOR_STEPS_IN_WHEEL*COLOR_TIME_BETWEEN_WHEEL_STEPS))/COLOR_TIME_BETWEEN_WHEEL_STEPS;

	// Now handle the stagger between sentries.
	byte thisSentryOffset = 0;
	if(timingType == PATTERN_TIMING_STAGGER) {
		byte wheelSentryPositionOffsetAmount = COLOR_STEPS_IN_WHEEL / NUMBER_SENTRIES;
		thisSentryOffset = getAddress() * wheelSentryPositionOffsetAmount;
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


/*
// Makes a random pixel white at random interals
void sparkle()
{
  // figure this out later strip.numPixels()]
  static byte pixelSparkleLevel[40];

  int seededStars = 8;
  // turn on newStars at max brightness
  for(int i=0; i < seededStars; i++)
    pixelSparkleLevel[random(strip.numPixels())]=255;

  while(true)
  {
    // This range may seem random, but my favourite setting is at 10
    // and I wanted 10 to be exatly in the middle.
    // clamb: need more dials to allow control of this; commented out for now
    // and hardcoded to 10.
    //   global_miscB (value 0->10) default = 5
    //   int likelyhood = (global_miscB+1)*2;
    //   if(random(map(likelyhood,0,1023,1,18))==0)
    if(random(60)==0)
    {
       pixelSparkleLevel[random(strip.numPixels())]=255;
    }

    // global_miscA (value 0->10) default = 5
    // int fadeSpeed = (global_miscA+1)*2;
    int fadeSpeed = 1;

    for(int i=0; i<strip.numPixels(); i++)
    {
      // take base colour in 'pixelSparkleLevel' array and create a bluish tinge
      byte red = pixelSparkleLevel[i];
      byte green = map(pixelSparkleLevel[i], 0, 255, 0, 175);
      byte blue = map(pixelSparkleLevel[i], 0, 255, 0, 140);
      uint32_t newColor = Color(pixelSparkleLevel[i], green, blue);
      strip.setPixelColor(i, newColor);

      // Slowly fade the sparkle down and ensure it never drops
      // less than zero.
      if(pixelSparkleLevel[i] > fadeSpeed)
      {
        pixelSparkleLevel[i] -= fadeSpeed;
      }
      else
      {
        pixelSparkleLevel[i] = 0;
      }
    }
    strip.show();

    // global_miscA (value 0->10) default = 5
    uint8_t wait = global_variables[speed].currNumber*10;
    if(global_patternChanged)
      return;
    else
      delay(wait);
  }
}


void circleComet()
{
  byte red = 255;
  byte green = 255;
  byte blue = 255;

  int currentIndex = 0;
  int numLights = 4;
  int offset = strip.numPixels() / numLights;
  while(true)
  {
    // walk the length setting each pixel to give clearing effect
    for (int i=0; i < strip.numPixels(); i++)
    {
      boolean found = false;

      if(i%offset == currentIndex)
      {
        strip.setPixelColor(i, Color(red, green, blue));
        found = true;
      }

      if(!found)
        strip.setPixelColor(i, Color(0, 0, 0));
    }
    strip.show();

    // move the ball
    if(currentIndex == offset-1)
      currentIndex = 0;
    else
      currentIndex++;

    uint8_t wait = global_variables[speed].currNumber*20;
    if(global_patternChanged)
      return;
    else
      delay(wait);
  }
}


void rainbowComet()
{
  byte red = 255;
  byte green = 255;
  byte blue = 255;

  int forward = 1;
  int ballLength = 10;
  int currentIndex = ballLength * -1;
  boolean doIncrement = false;

  int fadeSpeed = 3; //global_miscA;

  int targetWheel = random(511);
  int inverseWheel = (targetWheel + 255) % 511;
  int currTargetWheel = targetWheel;
  int currAntiTargetWheel = inverseWheel;
  int currInverseWheel = inverseWheel;

  while(true)
  {
    // walk the length setting each pixel to give clearing effect
    for (int i=0; i < strip.numPixels(); i++)
    {
      boolean found = false;

      int nextIndex = currentIndex + forward*(ballLength);
      if(i == nextIndex)
      {
        strip.setPixelColor(nextIndex, BigWheel(currTargetWheel));
        found = true;
      }
      else if((forward > 0 && i < nextIndex && i > currentIndex) ||
        (forward < 0 && i < currentIndex && i > nextIndex))
      {
        found = true;
      }
      else if(i == currentIndex)
      {
        strip.setPixelColor(currentIndex, BigWheel(currAntiTargetWheel));
        found = true;
      }

      if(!found)
        strip.setPixelColor(i, BigWheel(currInverseWheel));
    }
    strip.show();

    // Control Logic
    if(currInverseWheel != inverseWheel)
    {
      currInverseWheel += fadeSpeed;

      if(currInverseWheel <= inverseWheel+fadeSpeed/2 &&
        currInverseWheel >= inverseWheel-fadeSpeed/2)
      {
        currInverseWheel = inverseWheel;
      }
      if(currInverseWheel >= 511)
        currInverseWheel = 0;
    }
    else
    {
      if(currTargetWheel <= targetWheel+fadeSpeed/2 &&
        currTargetWheel >= targetWheel-fadeSpeed/2)
      {
        doIncrement = true;
        currTargetWheel = inverseWheel;
        currAntiTargetWheel = targetWheel;
      }
      else
      {
        currTargetWheel += fadeSpeed;
        if(currTargetWheel >= 511)
          currTargetWheel = 0;

        currAntiTargetWheel -= fadeSpeed;
        if(currAntiTargetWheel <= 0)
          currAntiTargetWheel = 511;
      }
    }

    // move the ball
    if(doIncrement)
    {
      if(forward == 1)
        currentIndex++;
      else
        currentIndex--;

      boolean directionChange = false;
      if(currentIndex == strip.numPixels()+ballLength)
      {
        forward = -1;
        directionChange = true;
      }
      else if(currentIndex == 0-(ballLength))
      {
        forward = 1;
        directionChange = true;
      }

      if(directionChange)
      {
        currTargetWheel = targetWheel;
        currInverseWheel = inverseWheel;
        currAntiTargetWheel = inverseWheel;

        targetWheel = random(511);
        inverseWheel = (targetWheel + 255) % 511;
      }

      doIncrement = false;
    }

    // global_miscA (value 0->10) default = 5
    uint8_t wait = 0;
    if(global_patternChanged)
      return;
    else
      delay(wait);
  }
}
*/
