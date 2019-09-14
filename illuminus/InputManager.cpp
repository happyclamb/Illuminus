#include "InputManager.h"


InputManager::InputManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	// Setup the zone pins
	pinMode(ZONE_0_PIN, INPUT_PULLUP);
	pinMode(ZONE_1_PIN, INPUT_PULLUP);

	// Setup the digital inputs
	this->button_pins[0] = INPUT1_PIN;   pinMode(this->button_pins[0], INPUT_PULLUP);
	this->button_pins[1] = INPUT2_PIN;   pinMode(this->button_pins[1], INPUT_PULLUP);
	this->button_pins[2] = INPUT_A5_PIN; pinMode(this->button_pins[2], INPUT_PULLUP);
	this->button_pins[3] = INPUT_A4_PIN; pinMode(this->button_pins[3], INPUT_PULLUP);
	this->button_pins[4] = INPUT_A1_PIN; pinMode(this->button_pins[4], INPUT);
	for(byte i=0; i<5; i++) {
		this->button_last_pressed[i] = 0;
		this->button_pressed[i] = false;
		this->button_handled[i] = true;
	}

	// Setup the analog inputs
	this->analog_pins[0] = INPUT_A0_PIN;
	this->analog_pins[1] = INPUT_A2_PIN;
	this->analog_pins[2] = INPUT_A3_PIN;
	for(byte i=0; i<3; i++) {
		this->analog_level[i] = map(analogRead(this->analog_pins[i]), 0, 1023, 0, 255);
		this->analog_handled[i] = true;
	}

	updateValues();

	singleMan->setInputMan(this);
}

bool InputManager::hasUnhandledInput() {
	bool returnVal = false;

	if(this->isInteractiveMode()) {
		for(byte i=0; i<5; i++) {
			if (this->button_handled[i] == false)
				returnVal = true;
		}
		for(byte i=0; i<3; i++) {
			if (this->analog_handled[i] == false)
				returnVal = true;
		}
	}

	return returnVal;
}

bool InputManager::isButtonPressed(byte index) {
	bool returnVal = this->button_pressed[index] == true && this->button_handled[index] == false;
	this->button_handled[index] = true;
	return returnVal;
}

byte InputManager::getAnalog(byte index) {
	this->analog_handled[index] = true;
	return this->analog_level[index];
}

void InputManager::updateValues() {

	// if serial data available, process it
	while(Serial.available () > 0) {
		processIncomingByte(Serial.read());
	}

	// If the interactive_mode is more than duration, empty it
	if(this->isInteractiveMode() && millis() > this->interactive_mode + INTERACTIVE_MODE_DURATION) {
		this->setInteractiveMode(false);
	}

	zoneInput = 0;
	if(digitalRead(ZONE_0_PIN) == LOW)
		zoneInput += 1;
	if(digitalRead(ZONE_1_PIN) == LOW)
		zoneInput += 2;

	// Poll all the buttons and get them ready for servicing
	for(byte i=0; i<5; i++) {
		if(this->button_last_pressed[i] > 0 && (millis() > this->button_last_pressed[i] + BUTTON_READ_INTERVAL)) {
			this->button_pressed[i] = false;
			this->button_handled[i] = true;
			this->button_last_pressed[i] = 0;
		}
		// button_pin[4] is wired inverse to the other buttons
		if(this->button_last_pressed[i] == 0 && digitalRead(this->button_pins[i]) == (i != 4 ? LOW : HIGH)) {
			this->setInteractiveMode(true);
			this->button_pressed[i] = true;
			this->button_handled[i] = false;
			this->button_last_pressed[i] = millis();
		}
	}

	// Analog inputs (value 0->1023)
	static unsigned long nextRead = 0;
	if(millis() > nextRead) {
		nextRead = millis() + POT_READ_INTERVAL;
		for(byte i=0; i<3; i++) {
			byte new_level = map(analogRead(this->analog_pins[i]), 0, 1023, 0, 255);

			if (new_level >= this->analog_level[i] + POT_READ_SENSITIVITY
				|| new_level + POT_READ_SENSITIVITY <= this->analog_level[i])
			{
				this->setInteractiveMode(true);
				this->analog_level[i] = new_level;
				this->analog_handled[i] = false;
			}
		}
	}

	// Handle output of logging inputs
	static unsigned long nextOutput = 0;
	if(millis() > nextOutput) {
		nextOutput = millis() + OUTPUT_FREQUENCY;

		singleMan->outputMan()->print(LOG_INPUTS, F("i:"));
		singleMan->outputMan()->print(LOG_INPUTS, isInteractiveMode());

		singleMan->outputMan()->print(LOG_INPUTS, F(" ui:"));
		singleMan->outputMan()->print(LOG_INPUTS, hasUnhandledInput());

		for(byte i=0; i<5; i++) {
			singleMan->outputMan()->print(LOG_INPUTS, F(" d"));
			singleMan->outputMan()->print(LOG_INPUTS, i);
			singleMan->outputMan()->print(LOG_INPUTS, F(":"));
			singleMan->outputMan()->print(LOG_INPUTS, this->button_pressed[i]);
		}

		for(byte i=0; i<3; i++) {
			singleMan->outputMan()->print(LOG_INPUTS, F(" a"));
			singleMan->outputMan()->print(LOG_INPUTS, i);
			singleMan->outputMan()->print(LOG_INPUTS, F(":"));
			singleMan->outputMan()->print(LOG_INPUTS, this->analog_level[i]);
		}

		singleMan->outputMan()->println(LOG_INPUTS, F(""));
	}
}

void InputManager::processIncomingByte(const byte inByte) {
	static char input_line[MAX_CLI_INPUT];
	static unsigned int input_pos = 0;

	switch (inByte) {

		case '\r':   // discard carriage return
			break;

		case '\n':   // end of text
			input_line[input_pos] = 0;  // terminating null byte

			// terminator reached! process input_line
			processData(input_line);

			// reset buffer for next time
			input_pos = 0;
			break;

		default:
			// keep adding if not full ... allow for terminating null byte
			if (input_pos < (MAX_CLI_INPUT - 1)) {
				input_line[input_pos++] = inByte;
			}
			break;
		}
}

void InputManager::showOptions() {
	Serial.println(F("C&C Lantern CLI"));
	Serial.println(F("Options::"));
	Serial.println(F("b #    update brightness of big LED"));
	Serial.println(F("l      show current log levels"));
	Serial.println(F("l #    toggle log level [info|debug|timing|inputs]"));
	Serial.println(F("h      Health of lanterns in network"));
	Serial.println(F("i      Interactive mode"));
	Serial.println(F("a      Auto choose patterns"));
	Serial.println(F("p # #  Pattern with parameters"));
	Serial.println(F("d #    update pattern Duration"));
	Serial.println(F("f #    color updates Frequency"));
}

void InputManager::setPattern(const char * data) {
	byte pattern=0, param_1=0, param_2=0, param_3=0, param_4=0, param_5=0;
	unsigned long start_time = singleMan->radioMan()->getAdjustedMillis() + 500;
	char pattern_string[MAX_CLI_INPUT];
	strcpy(pattern_string, data);

	char * strtokIndx;
	strtokIndx = strtok(pattern_string, " ");  pattern = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_1 = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_2 = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_3 = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_4 = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_5 = atoi(strtokIndx);

	Serial.println(F("New Pattern::"));
	LightPattern *inputPattern = new LightPattern(
		pattern, param_1, param_2, param_3, param_4, param_5, start_time);
	singleMan->lightMan()->setNextPattern(inputPattern, LOG_CLI);
	delete inputPattern;
	Serial.println(F(""));
}

void InputManager::showLogLevels() {
	Serial.println(F("Log Levels::"));
	Serial.print(F("   info:   "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_INFO) ? Serial.println(F("on")) : Serial.println(F("off"));
	Serial.print(F("   debug:  "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_DEBUG) ? Serial.println(F("on")) : Serial.println(F("off"));
	Serial.print(F("   radio:  "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_RADIO) ? Serial.println(F("on")) : Serial.println(F("off"));
	Serial.print(F("   inputs:  "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_INPUTS) ? Serial.println(F("on")) : Serial.println(F("off"));
#ifdef LOG_TIMING_DEFINED
	Serial.print(F("   timing: "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING) ? Serial.println(F("on")) : Serial.println(F("off"));
#endif
}

void InputManager::setLogLevel(const char * data){
	OUTPUT_LOG_TYPES log_level = LOG_CLI;
	if (strcmp(data, "info") == 0 || strcmp(data, "i") == 0)
		log_level = LOG_INFO;
	else if (strcmp(data, "debug") == 0 || strcmp(data, "d") == 0)
		log_level = LOG_DEBUG;
	else if (strcmp(data, "radio") == 0 || strcmp(data, "r") == 0)
		log_level = LOG_RADIO;
#ifdef LOG_TIMING_DEFINED
	else if (strcmp(data, "timing") == 0 || strcmp(data, "t") == 0)
		log_level = LOG_TIMING;
#endif
	else if (strcmp(data, "inputs") == 0)
		log_level = LOG_INPUTS;

	if(log_level != LOG_CLI) {
		singleMan->outputMan()->setLogLevel(log_level, !singleMan->outputMan()->isLogLevelEnabled(log_level));
		this->showLogLevels();
	}
}

// process incoming serial data after a terminator received
void InputManager::processData(const char * data) {

	switch (data[0]) {
		case '?': {
			this->showOptions();
			return;
		}
		case 'b': {
			byte bigLedBright = atoi(&data[2]);
			Serial.print(F("Big LED brightness >> "));
			Serial.println(bigLedBright);
			singleMan->lightMan()->setBigLightBrightness(bigLedBright);
			return;
		}
		case 'l': {
			if(strlen(data) == 1)
				this->showLogLevels();
			else
				this->setLogLevel(&data[2]);
			return;
		}
		case 'h': {
			singleMan->healthMan()->printHealth(LOG_CLI);
			return;
		}
	}

	if (singleMan->healthMan()->getServerAddress() == singleMan->addrMan()->getAddress()) {
		switch (data[0]) {
			case 'i': {
				this->setInteractiveMode(true);
				Serial.println(F("Interactive mode set"));
				return;
			}
			case 'a': {
				this->setInteractiveMode(false);
				singleMan->lightMan()->chooseNewPattern(10);
				Serial.println(F("Auto mode set"));
				return;
			}
			case 'p': {
				if(this->isInteractiveMode()) {
					this->setPattern(&data[2]);
				} else {
					Serial.println(F("ERROR! Can only set pattern in interactive mode"));
				}
				return;
			}
			case 'd': {
				unsigned long newDuration = atol(&data[2]);
				Serial.print(F("Updated pattern duration >> "));
				Serial.println(newDuration);
				singleMan->lightMan()->setPatternDuration(newDuration);
				return;
			}
			case 'f': {
				unsigned long newFrequency = atol(&data[2]);
				Serial.print(F("Updated color updates frequency >> "));
				Serial.println(newFrequency);
				singleMan->radioMan()->setIntervalColorMessages(newFrequency);
				return;
			}
		}
	} else {
		Serial.println(F("ERROR! Only available on sentry 0"));
		return;
	}

	Serial.print(F("UNKNOWN COMMAND >> "));
	Serial.println(data);
}
