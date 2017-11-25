#include "InputManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

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
