#ifndef __INPUTMANAGER_H__
#define __INPUTMANAGER_H__

#include <Arduino.h>
#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class InputManager
{
	public:
		InputManager(SingletonManager* _singleMan);

		void updateValues();
		bool isButton1Pressed() { return this->button1_pressed; }
		bool isButton2Pressed() { return this->button2_pressed; }
		bool isMotionDetected() { return this->motionLevel > 255; }
		int getLightLevel() { return this->lightLevel; }
		int getSoundLevel() { return this->soundLevel; }
		byte getZoneInput() { return this->zoneInput; }

	private:
		SingletonManager* singleMan = NULL;
		bool button1_pressed = false;
		bool button2_pressed = false;
		int motionLevel = 0;
		int lightLevel = 0;
		int soundLevel = 0;
		byte zoneInput = 0;
};

#endif // __INPUTMANAGER_H__
