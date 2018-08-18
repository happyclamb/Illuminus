#include <Arduino.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
#include "InputManager.h"
#include "OutputManager.h"
#include "AddressManager.h"
#include "LightManager.h"
#include "RadioManager.h"
#include "HealthManager.h"

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

	singleMan->outputMan()->println(LOG_INFO, F("Info Logging enabled"));
	singleMan->outputMan()->println(LOG_DEBUG, F("Info Logging enabled"));
	singleMan->outputMan()->println(LOG_TIMING, F("Info Logging enabled"));

	singleMan->outputMan()->print(LOG_INFO, F("Setup complete; Zone: "));
	singleMan->outputMan()->println(LOG_INFO, singleMan->addrMan()->getZone());
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
	bool forceNTPCheck = false;

	if(singleMan->addrMan()->hasAddress() == false) {

		// The light automically goes into flashing blue mode while no address found
		// Send off a BLOCKING request for a new address
		singleMan->addrMan()->obtainAddress();

		// Print out the usage options now that the lantern is up and running
		singleMan->inputMan()->showOptions();

		// First time through, start with a request for an NTP check
		forceNTPCheck = true;

		// Always set the 0 address since nothing runs without a server
		// If this was a sentry, add all its children to the healthMan
		for(byte i=0; i <= singleMan->addrMan()->getAddress(); i++)
			singleMan->healthMan()->updateSentryNTPRequestTime(i);
	}

	// Now handle sentry or server loop
	if (singleMan->addrMan()->getAddress() == 0)
		serverLoop();
	else
		sentryLoop(forceNTPCheck);

	static unsigned long lastHealthCheck = 0;
	if(millis() > lastHealthCheck + singleMan->radioMan()->getIntervalBetweenNTPChecks()) {
		// Every TIME_BETWEEN_NTP_MSGS lets check the health of the sentries
		singleMan->healthMan()->checkAllSentryHealth();
		lastHealthCheck = millis();
	}

	// Give a small break before looking messages
	delay(5);
}



void serverLoop() {
	static bool bootNTPSequence = true;
	static unsigned long lastNTPCheck = 0;
	static unsigned long lastLEDUpdateCheck = 0;

	// Collect and handle any messages in the queue
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();
	if(currMessage != NULL)
	{
		singleMan->healthMan()->updateSentryNTPRequestTime(currMessage->sentrySrcID);

		switch(currMessage->messageType) {
			case NEW_ADDRESS_REQUEST:
				singleMan->addrMan()->sendNewAddressResponse();
				break;
			case NTP_CLIENT_REQUEST:
				singleMan->radioMan()->handleNTPClientRequest(currMessage);
				break;
			case COLOR_MESSAGE_FROM_SENTRY:
				// Going to do something magic with response ??
				// Maybe something about marking it as known value and resend if not set ?
				break;
			case NTP_CLIENT_FINISHED:
				// spamming NTP requests will flood the system; so only
				// do the NTPCheck skipping for first run through of the sentries.
				if(bootNTPSequence) {
					// successfully finished current NTP, force jump to next sentry
					//	by making the time well before the next check
					lastNTPCheck = millis() - singleMan->radioMan()->getIntervalBetweenNTPChecks() - 100;
				}
				break;
		}

		// The server never echos messages so cleanup
		delete currMessage;
	}

	// TODO: this needs to use the interval once all the sentries have been called - or
	//	else as more sentries are added this is going to take a LONG time to sync !!
	if(millis() > lastNTPCheck + singleMan->radioMan()->getIntervalBetweenNTPChecks()) {

		// always update the server time
		singleMan->healthMan()->updateSentryNTPRequestTime(0);

		// don't bother sending COORD messages if there is no one to listen
		if(singleMan->healthMan()->totalSentries() > 1) {

			// If there is only one sentry, don't bother sending anything NTP_COORD_MESSAGES
			static int nextSentryToRunNTP = 1;

			// Fire off a message to the next sentry to run an NTPloop
			RF24Message ntpStartMessage;
			ntpStartMessage.messageType = NTP_COORD_MESSAGE;
			ntpStartMessage.sentrySrcID = 0;
			ntpStartMessage.sentryTargetID = nextSentryToRunNTP;

			// only request response if it'll be used (aka: during boot sequence)
			ntpStartMessage.param1_byte = bootNTPSequence ? 1 : 0;

			singleMan->radioMan()->sendMessage(ntpStartMessage);

			singleMan->outputMan()->print(LOG_INFO, F("Sending NTP COORD message to: "));
			singleMan->outputMan()->println(LOG_INFO, nextSentryToRunNTP);
			singleMan->healthMan()->printHealth();

			// Wrap back to start; reset bootNTPSequence if set.
			nextSentryToRunNTP++;
			if(nextSentryToRunNTP == singleMan->healthMan()->totalSentries()) {
				nextSentryToRunNTP = 1;
				bootNTPSequence = false;
			}

			// Update lastNTPCheck
			lastNTPCheck = millis();
		}
	}

	if(millis() > lastLEDUpdateCheck + singleMan->radioMan()->getIntervalBetweenPatternUpdates()) {

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

		singleMan->outputMan()->print(LOG_INFO, F("Sending Light Update    "));
		nextPattern->printPattern(singleMan);
		singleMan->outputMan()->println(LOG_INFO, F(""));

		// Update lastLEDUpdateCheck
		lastLEDUpdateCheck = millis();
	}
}




void sentryLoop(bool forceNTPCheck) {
	static NTP_state ntpState = NTP_DONE;
	static unsigned long timeOfLastNTPRequest = 0;

	if(forceNTPCheck) {
		ntpState = NTP_SEND_REQUEST;
		timeOfLastNTPRequest = 0;
		singleMan->radioMan()->setInformServerWhenNTPDone(true);
	}

	// spin until there is something in a queue
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();

	if(currMessage != NULL) {

		singleMan->healthMan()->updateSentryNTPRequestTime(currMessage->sentrySrcID);

		bool doEcho = true;
		byte address = singleMan->addrMan()->getAddress();
		switch(currMessage->messageType) {

			case NTP_COORD_MESSAGE:
				// keep sentry alive on all COORD messages
				singleMan->healthMan()->updateSentryNTPRequestTime(address);

				if(currMessage->sentryTargetID == address) {
					singleMan->outputMan()->println(LOG_DEBUG, F("NTP_COORD_MESSAGE message received"));

					// Don't need to echo this message as it is now at final destination
					singleMan->radioMan()->setInformServerWhenNTPDone(currMessage->param1_byte == 1 ? true : false);
					ntpState = NTP_SEND_REQUEST;
					timeOfLastNTPRequest = 0;
					doEcho = false;
				}
				break;

			case NTP_SERVER_RESPONSE:
				if(currMessage->sentryTargetID == address) {
					if(ntpState != NTP_DONE) {
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
				singleMan->lightMan()->setNextPattern(newPattern);
				delete newPattern;
				newPattern = NULL;

				// Send a response back to Server
				RF24Message responseMessage;
				responseMessage.messageType = COLOR_MESSAGE_FROM_SENTRY;
				responseMessage.sentryTargetID = 0;
				responseMessage.sentrySrcID = singleMan->addrMan()->getAddress();
				singleMan->radioMan()->sendMessage(responseMessage);

				break;
		}

		if(doEcho)
			singleMan->radioMan()->echoMessage(*currMessage);

		delete currMessage;
	}

	if(ntpState == NTP_WAITING_FOR_RESPONSE) {
		if(millis() > timeOfLastNTPRequest + singleMan->radioMan()->ntpRequestTimeout()) {
			singleMan->outputMan()->println(LOG_DEBUG, F("NTP Request timeout"));
			// assume that request has timedout and send another
			ntpState = NTP_SEND_REQUEST;
		}
	}

	if(ntpState == NTP_SEND_REQUEST) {
			singleMan->outputMan()->println(LOG_DEBUG, F("Sending NTP Request"));
			ntpState = singleMan->radioMan()->sendNTPRequestToServer();
			timeOfLastNTPRequest = millis();
	}
}
