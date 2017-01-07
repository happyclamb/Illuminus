#ifndef __LIGHTMANAGER_H__
#define __LIGHTMANAGER_H__

#include <Arduino.h>
#include <FastLED.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class LightManager {
	public:
		LightManager(SingletonManager* _singleMan);

		void redrawLights(); // called from interrupt handler

		void setLastRadioSend(long newTime) { this->lastRadioSend = newTime; }
		void setLastRadioReceive(long newTime) { this->lastRadioReceive = newTime; }


	private:
		CRGB ledstrip[NUM_RGB_LEDS];
		SingletonManager* singleMan = NULL;
		long lastRadioSend = 0;
		long lastRadioReceive = 0;

		void setMicrophoneLED();
		void setLightSensorLED();
		void setMotionSensorLED();
		void setButtonLED();
		void setRadioLED();
		void setZoneLED();
		void pulsePattern();
};

#endif // __LIGHTMANAGER_H__
