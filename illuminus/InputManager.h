#ifndef __INPUTMANAGER_H__
#define __INPUTMANAGER_H__

#define MAX_CLI_INPUT 50

#include <Arduino.h>
#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class InputManager
{
	public:
		InputManager(SingletonManager* _singleMan);

		void showOptions();
		void updateValues();

		bool isButton1Pressed() { return this->button1_pressed; }
		bool isButton2Pressed() { return this->button2_pressed; }
		bool isMotionDetected() { return this->motionLevel > 64; }
		byte getLightLevel()    { return this->lightLevel; }
		byte getSoundLevel()    { return this->soundLevel; }
		byte getZoneInput()     { return this->zoneInput; }

	private:
		SingletonManager* singleMan = NULL;

		byte zoneInput = 0;
		bool button1_pressed = false;
		bool button2_pressed = false;
		byte motionLevel = 0;
		byte lightLevel = 0;
		byte soundLevel = 0;

		void processData(const char * data);
		void processIncomingByte(const byte inByte);
		void setPattern(const char * data);
		void showLogLevels();
		void setLogLevel(const char * data);
};

#endif // __INPUTMANAGER_H__
