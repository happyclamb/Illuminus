#include "InputManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

InputManager::InputManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	// Setup the addressing pins
	pinMode(INPUT1_PIN, INPUT_PULLUP);
	pinMode(INPUT2_PIN, INPUT_PULLUP);

	singleMan->setInputMan(this);
}


void InputManager::updateValues() {

	// Analog input (value 0->1024)
	int A0_value = analogRead(INPUTA_A0_PIN) / 4;

	// HIGH == off, LOW == on
	button1_pressed = false;
	if(digitalRead(INPUT1_PIN) == LOW)
		button1_pressed = true;

	button2_pressed = false;
	if(digitalRead(INPUT2_PIN) == LOW)
		button2_pressed = true;

	// Analog inputs (value 0->1024)
	motionLevel = analogRead(MOTION_SENSOR_A3_PIN);
	lightLevel = analogRead(LIGHT_SENSOR_A1_PIN);
	soundLevel = analogRead(SOUND_SENSOR_A2_PIN);
}
