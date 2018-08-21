#include <Arduino.h>

#include "SingletonManager.h"
SingletonManager *singleMan = NULL;

void setup() {
	Serial.begin(9600);

	// Create the holder for global objects
	singleMan = new SingletonManager();

	// Set first so it can be used for output during init routines
	OutputManager *outputMan = new OutputManager(singleMan);

	// Needs to be set early as there seems to be a bug with the RF24 library where it breaks
	//	if the pinMode is set after it is initialized
	InputManager *inputMan = new InputManager(singleMan);

	// Initilize the AddressManager
	AddressManager *addrMan = new AddressManager(singleMan);

	// Initilize the HealthManager
	HealthManager *healthMan = new HealthManager(singleMan);

	// Initilize the LightManager
	LightManager *lightMan = new LightManager(singleMan);
	delay(50); // Wait for light to init before continuing

	// Initialize the RadioManager
	RadioManager *radioMan = new RadioManager(singleMan, RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
	delay(50); // Wait for radio to init before continuing

	// Start interrupt handler for LightManagement
	init_TIMER1_irq();

	singleMan->outputMan()->println(LOG_INFO, F("Setup complete"));
	singleMan->outputMan()->println(LOG_INFO, F("Info Logging enabled"));
	singleMan->outputMan()->println(LOG_DEBUG, F("Debug Logging enabled"));
	singleMan->outputMan()->println(LOG_TIMING, F("Timing Logging enabled"));
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

	// Just starting up, no address assigned to request one.
	if(singleMan->addrMan()->hasAddress() == false) {

		// The light automically goes into flashing blue mode while no address found
		// Send off a BLOCKING request for a new address
		singleMan->addrMan()->obtainAddress();

		// Print out the usage options now that the lantern is up and running
		singleMan->inputMan()->showOptions();

		// Always set the 0 address since nothing runs without a server
		// If this was a sentry, add all its children to the healthMan
		for(byte i=0; i <= singleMan->addrMan()->getAddress(); i++)
			singleMan->healthMan()->updateSentryHealthTime(i, 0, millis());
	}

	// Now handle sentry or server loop
	if (singleMan->addrMan()->getAddress() == 0)
		serverLoop();
	else
		sentryLoop();

	// Every IntervalBetweenNTPChecks, check the health of the sentries
	static unsigned long lastHealthCheck = 0;
	if(millis() > (lastHealthCheck + singleMan->radioMan()->getIntervalBroadcastMessages())) {

		// always update the local health
		singleMan->healthMan()->updateSentryHealthTime(singleMan->addrMan()->getAddress(), millis(), millis());
		singleMan->healthMan()->checkAllSentryHealth();
		lastHealthCheck = millis();
	}
}



void serverLoop() {
	static unsigned long lastMessage = 0;
	static Radio_Message_Type nextType = NTP_COORD_MESSAGE;

	// Collect and handle any messages in the queue
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();

	if(currMessage != NULL)
	{
		singleMan->healthMan()->updateSentryHealthTime(currMessage->sentrySrcID, 0, millis());

		switch(currMessage->messageType) {
			case NEW_ADDRESS_REQUEST:
				singleMan->addrMan()->sendNewAddressResponse(currMessage);
				break;
			case NTP_CLIENT_REQUEST:
				singleMan->radioMan()->handleNTPClientRequest(currMessage);
				break;
			case COLOR_MESSAGE_FROM_SENTRY:
				// Going to do something magic with response ??
				// Maybe something about marking it as known value and resend if not set ?
				break;
			case NTP_CLIENT_FINISHED:
				singleMan->healthMan()->updateSentryHealthTime(currMessage->sentrySrcID, millis(), 0);
				singleMan->outputMan()->print(LOG_INFO, F("NTP_CLIENT_FINISHED:: sentrySrcID> "));
				singleMan->outputMan()->print(LOG_INFO, currMessage->sentrySrcID);
				singleMan->outputMan()->print(LOG_INFO, F("  avgOffset> "));
				singleMan->outputMan()->println(LOG_INFO, currMessage->param5_client_start);
				break;
		}

		// The server never echos messages so cleanup
		delete currMessage;
	}

	// Only send more coord messages if not in the middle ... or a long time has rolled by
	//	because the sentry died during the coord check.
	if(millis() > (lastMessage + singleMan->radioMan()->getIntervalBroadcastMessages()))
	{
		lastMessage = millis();

		switch (nextType) {
			case NTP_COORD_MESSAGE:
				// If there is only one sentry, don't bother sending anything NTP_COORD_MESSAGES
				if(singleMan->healthMan()->totalSentries() > 1) {

					int nextSentryToRunNTP = singleMan->healthMan()->getOldestNTPRequest();

					// Fire off a message to the next sentry to run an NTPloop
					RF24Message ntpStartMessage;
					ntpStartMessage.messageType = NTP_COORD_MESSAGE;
					ntpStartMessage.sentrySrcID = 0;
					ntpStartMessage.sentryTargetID = nextSentryToRunNTP;

					singleMan->radioMan()->sendMessage(ntpStartMessage);
				}
				nextType = COLOR_MESSAGE_TO_SENTRY;
				break;

			case COLOR_MESSAGE_TO_SENTRY:
				// generate newPatterns for LEDs since the interrupt will do the painting
				singleMan->lightMan()->chooseNewPattern();

				// This is a pointer to the original, don't delete it.
				LightPattern* nextPattern = singleMan->lightMan()->getNextPattern();

				// send color updates
				RF24Message lightMessage;
				lightMessage.messageType = COLOR_MESSAGE_TO_SENTRY;
				lightMessage.sentrySrcID = 0;
				lightMessage.sentryTargetID = 255;
				lightMessage.param1_byte = nextPattern->pattern;
				lightMessage.param2_byte = nextPattern->pattern_param1;
				lightMessage.param3_byte = nextPattern->pattern_param2;
				lightMessage.param4_client_end = nextPattern->pattern_param3;
				lightMessage.param5_client_start = nextPattern->pattern_param4;
				lightMessage.param6_server_end = nextPattern->pattern_param5;
				lightMessage.param7_server_start = nextPattern->startTime;

				singleMan->radioMan()->sendMessage(lightMessage);
				nextPattern->printlnPattern(singleMan, LOG_RADIO);

				nextType = NTP_COORD_MESSAGE;
				break;
		}
	}

}



void sentryLoop() {
	static NTP_state ntpState = NTP_DONE;
	static unsigned long timeOfLastNTPRequest = 0;

	// spin until there is something in a queue
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();

	if(currMessage != NULL) {

		singleMan->healthMan()->updateSentryHealthTime(currMessage->sentrySrcID, 0, millis());

		bool doEcho = true;
		byte address = singleMan->addrMan()->getAddress();
		switch(currMessage->messageType) {

			case NEW_ADDRESS_RESPONSE:
				// Already handled, just ignore it.
				if(currMessage->sentryTargetID == address) {
					doEcho = false;
				}
				break;

			case NTP_COORD_MESSAGE:
				// keep sentry alive on all COORD messages
				singleMan->healthMan()->updateSentryHealthTime(address, 0, millis());

				if(currMessage->sentryTargetID == address) {

					// Might currently be in the middle of NTP sequencing, don't reset
					if (ntpState == NTP_DONE) {
						ntpState = NTP_SEND_REQUEST;
						timeOfLastNTPRequest = 0;
					}

					// Don't need to echo this message as it is now at final destination
					doEcho = false;
				}
				break;

			case NTP_SERVER_RESPONSE:
				if(currMessage->sentryTargetID == address) {
					if(ntpState == NTP_WAITING_FOR_RESPONSE) {
						ntpState = singleMan->radioMan()->handleNTPServerResponse(currMessage);
					}
					// Don't need to echo this message as it is now complete
					doEcho = false;
				}
				break;

			case COLOR_MESSAGE_TO_SENTRY:
				LightPattern* newPattern = new LightPattern(
					currMessage->param1_byte, currMessage->param2_byte,
					currMessage->param3_byte, (byte)currMessage->param4_client_end,
					(byte)currMessage->param5_client_start, (byte)currMessage->param6_server_end,
					currMessage->param7_server_start);

				// update pattern for LEDs since the interrupt will do the painting
				singleMan->lightMan()->setNextPattern(newPattern, LOG_RADIO);
				delete newPattern;
				newPattern = NULL;

				// Send a response back to Server
				RF24Message responseMessage;
				responseMessage.messageType = COLOR_MESSAGE_FROM_SENTRY;
				responseMessage.sentryTargetID = 0;
				responseMessage.sentrySrcID = singleMan->addrMan()->getAddress();
				responseMessage.param4_client_end = currMessage->UID;
				responseMessage.param5_client_start = currMessage->UID;
				responseMessage.param6_server_end = currMessage->UID;
				responseMessage.param7_server_start = currMessage->UID;
				singleMan->radioMan()->sendMessage(responseMessage);

				break;
		}

		if(doEcho)
			singleMan->radioMan()->echoMessage(*currMessage);

		delete currMessage;
		currMessage = NULL;
	}

	// Handle case where no response resulted in timeout
	if(ntpState == NTP_WAITING_FOR_RESPONSE) {
		if(millis() > timeOfLastNTPRequest + singleMan->radioMan()->ntpRequestTimeout()) {
			singleMan->outputMan()->println(LOG_ERROR, F("NTP Request timeout"));
			// assume that request has timedout and send another
			ntpState = NTP_SEND_REQUEST;
		}
	}

	// If an NTP request is needed, send it now
	if(ntpState == NTP_SEND_REQUEST) {
			ntpState = singleMan->radioMan()->sendNTPRequestToServer();
			timeOfLastNTPRequest = millis();
	}
}
