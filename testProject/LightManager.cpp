#include "LightManager.h"

#include <Arduino.h>
#include <FastLED.h>		// http://fastled.io/docs/3.1/annotated.html

#include "IlluminusDefs.h"
#include "SingletonManager.h"

LightManager::LightManager(SingletonManager* _singleMan):
	singleMan(_singleMan) {

	// Init FastLED RGB
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


//*********
//*********  Everything from here forward runs on interrupt !! *******
//*********
void LightManager::redrawLights() {

	// Microphone Big LED
	setMicrophoneLED();

	// Pulse LED[0]
	pulsePattern();

	// LightSensor LED[1]
	setLightSensorLED();

	// MotionSensor LED[2]
	setMotionSensorLED();

	// ZonePins LED [3]
	setZoneLED();

	// Radio Data LED [4]
	setRadioLED();

	// Handle button inputs [5]
	setButtonLED();

	// Update LED
	FastLED.show();
}


void LightManager::setMicrophoneLED() {
	// Analog input (value 0->1024)
	int microphoneLevel = singleMan->inputMan()->getSoundLevel() / 4;
//	info_println(microphoneLevel);

	analogWrite(BIG_LED_PIN, 255);
}

void LightManager::setLightSensorLED() {
	// Analog input (value 0->1024)
	int lightSensorLevel = singleMan->inputMan()->getLightLevel() / 2;

	int lowLevel = lightSensorLevel;
	if(lowLevel > 255)
		lowLevel = 255;

	int highLevel = lightSensorLevel;
	if(highLevel < 255)
		highLevel = 0;
	else
		highLevel -= 255;

	ledstrip[1] = CRGB(highLevel,lowLevel,0);
}

void LightManager::setMotionSensorLED() {
	if(singleMan->inputMan()->isMotionDetected())
		ledstrip[2] = CRGB(0,255,0);
	else
		ledstrip[2] = CRGB(255,0,0);
}

void LightManager::setButtonLED() {

		int green = 0;
		if(singleMan->inputMan()->isButton1Pressed())
			green = 255;

		int blue = 0;
		if(singleMan->inputMan()->isButton2Pressed())
			blue = 255;

		ledstrip[5] = CRGB(0,green,blue);
}

void LightManager::setZoneLED() {
	int currentZone = singleMan->addrMan()->getZone();

	if(currentZone == 0)
		ledstrip[3] = CRGB(255,0,0);
	else if(currentZone == 1)
		ledstrip[3] = CRGB(0,255,0);
	else if(currentZone == 2)
		ledstrip[3] = CRGB(0,0,255);
	else if(currentZone == 3)
		ledstrip[3] = CRGB(255,255,255);
	else
		ledstrip[3] = CRGB(0,0,0);
}

void LightManager::setRadioLED() {
	int red = 0;
	if(millis() < lastRadioSend+1000)
		red = 255;

	int green = 0;
	if(millis() < lastRadioReceive+1000)
		green = 255;

	ledstrip[4] = CRGB(red,green,0);
}


void LightManager::pulsePattern() {
	// 4ms per Tick, 300 ticks per fade, 6 fades
	int fullPattern = ( millis() % (4*300*6) ) / 4;

	int fadeIndex = fullPattern%300;
	if(fadeIndex > 150)
		fadeIndex = 150 - (fadeIndex-150);

	switch (fullPattern / 300)
	{
		case 0:
		case 1:
		case 2:
			ledstrip[0] = CRGB(fadeIndex,0,0);
			break;
		case 3:
		case 4:
			ledstrip[0] = CRGB(0,fadeIndex,0);
			break;
		case 5:
			ledstrip[0] = CRGB(0,0,fadeIndex);
			break;
	}
}
