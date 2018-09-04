#include "InputManager.h"
#include "OutputManager.h"

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

	for(byte i=0; i<5; i++) {
		this->motionLevel[i] = 0;
		this->lightLevel[i] = 0;
		this->soundLevel[i] = 0;
	}

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

	// Analog inputs (value 0->1023)
	static unsigned long lastRead = 0;
	if(millis() > (lastRead + 250)) {
		lightLevel[inputIndex]  = map(analogRead(LIGHT_SENSOR_A1_PIN), 0, 1023, 0, 255);
		soundLevel[inputIndex]  = map(analogRead(SOUND_SENSOR_A2_PIN), 0, 1023, 0, 255);
		motionLevel[inputIndex] = map(analogRead(MOTION_SENSOR_A3_PIN), 0, 1023, 0, 255);

		int motionLevel_sum = 0;
		int lightLevel_sum = 0;
		int soundLevel_sum = 0;
		for(byte i=0; i<5; i++) {
			motionLevel_sum += this->motionLevel[i];
			lightLevel_sum  += this->lightLevel[i];
			soundLevel_sum  += this->soundLevel[i];
		}
		this->motionLevel_avg = motionLevel_sum / 5;
		this->lightLevel_avg = lightLevel_sum / 5;
		this->soundLevel_avg = soundLevel_sum / 5;

		// Advance to next index
		lastRead = millis();
		if(++inputIndex == 5) inputIndex = 0;
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
	Serial.println(F("b #      update brightness of big LED"));
	Serial.println(F("l        show current log levels"));
	Serial.println(F("l #      toggle log level [info|debug|timing]"));
	Serial.println(F("h        Health of lanterns in network"));
	Serial.println(F("i        Interactive mode"));
	Serial.println(F("a        Auto choose patterns"));
	Serial.println(F("p # # #  Pattern with parameters"));
	Serial.println(F("d #      update pattern Duration"));
	Serial.println(F("f #      broadcast updates Frequency"));
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
	Serial.print(F("   timing: "));
	singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING) ? Serial.println(F("on")) : Serial.println(F("off"));
}

void InputManager::setLogLevel(const char * data){
	OUTPUT_LOG_TYPES log_level = LOG_CLI;
	if (strcmp(data, "info") == 0 || strcmp(data, "i") == 0)
		log_level = LOG_INFO;
	else if (strcmp(data, "debug") == 0 || strcmp(data, "d") == 0)
		log_level = LOG_DEBUG;
	else if (strcmp(data, "radio") == 0 || strcmp(data, "r") == 0)
		log_level = LOG_RADIO;
	else if (strcmp(data, "timing") == 0 || strcmp(data, "t") == 0)
		log_level = LOG_TIMING;

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
				singleMan->lightMan()->setManualMode(true);
				Serial.println(F("Interactive mode set"));
				return;
			}
			case 'a': {
				singleMan->lightMan()->chooseNewPattern(10);
				singleMan->lightMan()->setManualMode(false);
				Serial.println(F("Auto mode set"));
				return;
			}
			case 'p': {
				if(singleMan->lightMan()->getManualMode()) {
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
				Serial.print(F("Updated broadcast updates frequency >> "));
				Serial.println(newFrequency);
				singleMan->radioMan()->setIntervalBroadcastMessages(newFrequency);
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
