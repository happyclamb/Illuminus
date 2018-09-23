#ifndef __INPUTMANAGER_H__
#define __INPUTMANAGER_H__

#include "SingletonManager.h"
class SingletonManager;

#define MAX_CLI_INPUT 32

class InputManager
{
	public:
		InputManager(SingletonManager* _singleMan);

		void showOptions();
		void updateValues();

		bool isButton1Pressed() { return this->button1_pressed; }
		bool isButton2Pressed() { return this->button2_pressed; }
		bool isMotionDetected() { return this->motionLevel_avg > 64; }
		byte getLightLevel()    { return this->lightLevel_avg; }
		byte getSoundLevel()    { return this->soundLevel_avg; }
		byte getZoneInput()     { return this->zoneInput; }

	private:
		SingletonManager* singleMan = NULL;

		int READ_FREQUENCY = 251;

		byte zoneInput = 0;
		bool button1_pressed = false;
		bool button2_pressed = false;

		byte inputIndex = 0;
		byte motionLevel[5];
		byte lightLevel[5];
		byte soundLevel[5];
		byte motionLevel_avg = 0;
		byte lightLevel_avg = 0;
		byte soundLevel_avg = 0;

		void processData(const char * data);
		void processIncomingByte(const byte inByte);
		void setPattern(const char * data);
		void showLogLevels();
		void setLogLevel(const char * data);
};

#endif // __INPUTMANAGER_H__
