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
	while (Serial.available () > 0) {
		processIncomingByte(Serial.read ());
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


// process incoming serial data after a terminator received
void InputManager::process_data(const char * data) {

	Serial.println ("process_data!");
	Serial.println (data);

	if (strcmp(data, "on") == 0) {
		singleMan->lightMan()->setBigLightBrightness(255);
	} else if (strcmp(data, "off") == 0) {
		singleMan->lightMan()->setBigLightBrightness(0);
	}
}

void InputManager::processIncomingByte(const byte inByte) {
	static char input_line [MAX_CLI_INPUT];
	static unsigned int input_pos = 0;

	switch (inByte) {

		case '\r':   // discard carriage return
			break;

		case '\n':   // end of text
			input_line [input_pos] = 0;  // terminating null byte

			// terminator reached! process input_line
			process_data (input_line);

			// reset buffer for next time
			input_pos = 0;
			break;

		default:
			// keep adding if not full ... allow for terminating null byte
			if (input_pos < (MAX_CLI_INPUT - 1)) {
				input_line [input_pos++] = inByte;
			}
			break;
		}
}
