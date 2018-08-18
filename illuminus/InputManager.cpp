#include "InputManager.h"
#include "IlluminusDefs.h"
#include "SingletonManager.h"
#include "LightManager.h"

InputManager::InputManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	// Setup the digital pins
	pinMode(INPUT1_PIN, INPUT_PULLUP);
	pinMode(INPUT2_PIN, INPUT_PULLUP);

	// Setup the zone pins
	pinMode(ZONE_0_PIN, INPUT_PULLUP);
	pinMode(ZONE_1_PIN, INPUT_PULLUP);

	updateValues();

	singleMan->setInputMan(this);
}


void InputManager::updateValues() {

	// if serial data available, process it
	while(Serial.available () > 0) {
		processIncomingByte(Serial.read());
	}

	// Poll the zone pins :: HIGH == off, LOW == on
	zoneInput = 0;
	if(digitalRead(ZONE_0_PIN) == LOW)
		zoneInput += 1;
	if(digitalRead(ZONE_1_PIN) == LOW)
		zoneInput += 2;

	// HIGH == off, LOW == on
	button1_pressed = false;
	if(digitalRead(INPUT1_PIN) == LOW)
		button1_pressed = true;

	button2_pressed = false;
	if(digitalRead(INPUT2_PIN) == LOW)
		button2_pressed = true;

	// Analog input (value 0->1024)
	int A0_value = analogRead(INPUTA_A0_PIN) / 4;

	// Analog inputs (value 0->1024)
	lightLevel = analogRead(LIGHT_SENSOR_A1_PIN);
	soundLevel = analogRead(SOUND_SENSOR_A2_PIN);
	motionLevel = analogRead(MOTION_SENSOR_A3_PIN);
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

	if(singleMan->addrMan()->getAddress() != 0)  {
		Serial.println(F("WARNING! Can only set interactive mode on master sentry"));
	}

	Serial.println(F("Options::"));
	Serial.println(F("i                start up interactive mode"));
	Serial.println(F("a                return back to auto choosing patterns"));
	Serial.println(F("l                show current log levels"));
	Serial.println(F("l ###            toggle log level [info|debug|timing]"));
	Serial.println(F("p ### ### ###    select a pattern along with parameters to pass"));
	Serial.println(F("b ###            select a brightness of big LED"));
	Serial.println(F("h                shows current health of lanterns in network"));
	Serial.println(F(""));
}

void InputManager::setPattern(const char * data) {
	byte pattern=0, param_1=0, param_2=0;
	unsigned long start_time = singleMan->radioMan()->getAdjustedMillis() + 500;
	char pattern_string[MAX_CLI_INPUT];
	strcpy(pattern_string, data);

	char * strtokIndx;
	strtokIndx = strtok(pattern_string, " ");  pattern = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_1 = atoi(strtokIndx);
	strtokIndx = strtok(NULL, " ");            param_2 = atoi(strtokIndx);

	LightPattern *inputPattern = new LightPattern(pattern,param_1,param_2,start_time);
	singleMan->lightMan()->setNextPattern(inputPattern);
	delete inputPattern;
}

void InputManager::showLogLevels() {
	Serial.println(F("Log Levels::"));
	Serial.print(F("   info:   "));
	singleMan->outputMan()->isInfoEnabled() ? Serial.println(F("on")) : Serial.println(F("off"));
	Serial.print(F("   debug:  "));
	singleMan->outputMan()->isDebugEnabled() ? Serial.println(F("on")) : Serial.println(F("off"));
	Serial.print(F("   timing: "));
	singleMan->outputMan()->isTimingEnabled() ? Serial.println(F("on")) : Serial.println(F("off"));
}

void InputManager::setLogLevel(const char * data){
	if (strcmp(data, "info") == 0) singleMan->outputMan()->setInfoEnabled(!singleMan->outputMan()->isInfoEnabled());
	if (strcmp(data, "debug") == 0) singleMan->outputMan()->setDebugEnabled(!singleMan->outputMan()->isDebugEnabled());
	if (strcmp(data, "timing") == 0) singleMan->outputMan()->setTimingEnabled(!singleMan->outputMan()->isTimingEnabled());

	this->showLogLevels();
}

// process incoming serial data after a terminator received
void InputManager::processData(const char * data) {

	if (strcmp(data, "?") == 0 || strcmp(data, "help") == 0) {
		this->showOptions();
	}
	else if (strcmp(data, "i") == 0) {
		if(singleMan->addrMan()->getAddress() == 0)  {
			singleMan->lightMan()->setManualMode(true);
			Serial.println(F("Interactive mode set"));
		} else {
			Serial.println(F("ERROR! Can only set interactive mode on master sentry"));
		}
	} else if (strcmp(data, "a") == 0) {
		singleMan->lightMan()->setManualMode(false);
		Serial.println(F("Auto mode set"));
	} else if (data[0] == 'p') {
		if(singleMan->lightMan()->getManualMode()) {
			this->setPattern(&data[2]);
		} else {
			Serial.println(F("ERROR! Can only set pattern in interactive mode"));
		}
	} else if (data[0] == 'b') {
		byte bigLedBright = atoi(&data[2]);
		Serial.print(F("Server big LED brightness >> "));
		Serial.println(bigLedBright);
		singleMan->lightMan()->setBigLightBrightness(bigLedBright);
	} else if (data[0] == 'l') {
		if(strlen(data) == 1)
			this->showLogLevels();
		else
			this->setLogLevel(&data[2]);
	} else if (data[0] == 'h') {
		singleMan->healthMan()->printHealth(true);
	} else {
		Serial.print(F("process_data >> "));
		Serial.println(data);
	}
}
