#include <Arduino.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
#include "InputManager.h"
#include "AddressManager.h"
#include "LightManager.h"
#include "RadioManager.h"

SingletonManager *singleMan = NULL;

void setup() {
	Serial.begin(9600);

	// Create the holder for global objects
	singleMan = new SingletonManager();

	// Needs to be set first as there seems to be a bug with the RF24 library where it breaks
	//	if the pinMode is set afer it is initialized
	InputManager *inputMan = new InputManager(singleMan);

	// Initilize the AddressManager
	AddressManager *addrMan = new AddressManager(singleMan);

	// Initilize the LightManager
	LightManager *lightMan = new LightManager(singleMan);
	delay(50); // Wait for light to init before continuing

	// Initialize the RadioManager
	RadioManager *radioMan = new RadioManager(singleMan, RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
	delay(50); // Wait for radio to init before continuing

	// Start interrupt handler for LightManagement
	init_TIMER1_irq();

	Serial.print("Setup complete; Zone: ");
	Serial.println(singleMan->addrMan()->getZone());

	info_println("Info Logging enabled");
	debug_println("Debug Logging enabled");
	timing_println("Timing Logging enabled");
}



// initialize timer1 to redraw the LED strip and BigLight
int timer1_counter;
void init_TIMER1_irq()
{
	// disable all interrupts while changing them
	noInterrupts();

	// Configure timer1 in normal mode (pure counting, no PWM etc.)
	TCCR1A = 0;
	TCCR1B = 0;

	// Set divisor
	/*
			http://electronics.stackexchange.com/questions/165675/how-to-calculate-the-time-of-one-single-tick-on-microcontroller
		Table 16-5. Clock Select Bit Description								For 16MHz:
			CS12 CS11 CS10 Description																			Time per counter tick		Max Period
			0    0    0    No clock source (Timer/Counter stopped)
			0    0    1    clkI/O /1 (No prescaling)												0.0625 uS								8.192 mS
			0    1    0    clkI/O /8 (From prescaler)												0.5 uS									65.536 mS			 32,767.5
			0    1    1    clkI/O /64 (From prescaler)											4 uS										524.288 mS		262,140
			1    0    0    clkI/O /256 (From prescaler)											16 uS										2097.152 mS
			1    0    1    clkI/O /1024 (From prescaler)										64uS										8388.608mS
			1    1    0    External clock source on T1 pin. Falling edge.
			1    1    1    External clock source on T1 pin. Rising edge.
	*/
	TCCR1B |= (1 << CS11);
	TCCR1B |= (1 << CS10);

	// Disable Compare Match A interrupt enable (only want overflow)
	TIMSK1 = 0;
	TIMSK1 |= (1<<TOIE1);

	// Finally load the timer start counter; lowest is where to count
	// start as interrupt happens at wrap around
		// 65535 == wrap;
		// 1/64; 20ms == 20,000uS @ 4uS/tick == 5,000 tics == 65535 - 5000 == 60035		// smooth
		// 1/64; 50ms == 50,000uS @ 4uS/tick == 12,500 tics == 65535 - 12500 == 53035		// smooth
		// 1/64; 100ms == 100,000uS @ 4uS/tick == 25,000 tics == 65535 - 25000 == 40535 // jittery

	timer1_counter = 53035;
	TCNT1 = timer1_counter;

	// enable all interrupts now that things are ready to go
	interrupts();
}

// interrupt service routine for
ISR(TIMER1_OVF_vect)
{
	singleMan->lightMan()->redrawLights();

	// load timer last to maximize time until next call
	TCNT1 = timer1_counter;
}


void loop() {
	// Update Inputs
	singleMan->inputMan()->updateValues();

	// LED control is handled by interrupt on timer1

	// Every 3 seconds send a radio message
	static long lastSend = 0;
	long sendOffset = (singleMan->addrMan()->getZone() + 2)*1000;

	if(millis() > (lastSend + sendOffset)) {

		RF24Message testMessage;
		testMessage.messageType = TEST_MESSAGE;

		singleMan->radioMan()->sendMessage(testMessage);
		lastSend = millis();
		singleMan->lightMan()->setLastRadioSend(lastSend);
	}

	// Check for interference
	if(singleMan->radioMan()->checkForInterference()) {
		info_println("RF24 INTERFERENCE DETECTED");
	}

	// Poll for any new RadioData
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();

	if(currMessage != NULL) {
		if(currMessage->messageType == TEST_MESSAGE) {
			singleMan->lightMan()->setLastRadioReceive(millis());
		}

		delete currMessage;
	}

	delay(5);
}
