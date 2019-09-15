#ifndef __INPUTMANAGER_H__
#define __INPUTMANAGER_H__

#include "SingletonManager.h"
class SingletonManager;

#define MAX_CLI_INPUT 32

class InputManager
{
	public:
		InputManager(SingletonManager* _singleMan);

		bool isInteractiveMode() { return this->hasControlBox() && this->interactive_mode > 0; }
		void setInteractiveMode(bool newMode) { this->interactive_mode = newMode ? millis() : 0; }

		void showOptions();
		void updateValues();

		byte getZoneInput() { return this->zoneInput; }
		bool hasControlBox() { return this->isControlBox; }

		bool hasUnhandledInput();
		bool isButtonPressed(byte index);
		byte getAnalog(byte index);

	private:
		SingletonManager* singleMan = NULL;

		unsigned long interactive_mode = 0;

		unsigned long INTERACTIVE_MODE_DURATION = 120000; // 2minutes; 2*60*1000
		unsigned long OUTPUT_FREQUENCY = 1000;
		unsigned long BUTTON_READ_INTERVAL = 750;
		unsigned long POT_READ_INTERVAL = 200;
		unsigned long POT_READ_SENSITIVITY = 2;

		byte zoneInput = 0;
		bool isControlBox = false;

		int button_pins[5];
		unsigned long button_last_pressed[5];
		bool button_pressed[5];
		bool button_handled[5];

		int analog_pins[3];
		byte analog_level[3];
		bool analog_handled[3];

		void processData(const char * data);
		void processIncomingByte(const byte inByte);
		void setPattern(const char * data);
		void showLogLevels();
		void setLogLevel(const char * data);
};

#endif // __INPUTMANAGER_H__
