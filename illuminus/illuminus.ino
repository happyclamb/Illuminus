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
	RadioManager *radioMan = new RadioManager(singleMan);
	delay(50); // Wait for radio to init before continuing

	// Start interrupt handler for LightManagement
	init_TIMER1_irq();

	singleMan->outputMan()->println(LOG_INFO, F("Info Logging enabled"));
	singleMan->outputMan()->println(LOG_DEBUG, F("Debug Logging enabled"));
#ifdef LOG_TIMING_DEFINED
	singleMan->outputMan()->println(LOG_TIMING, F("Timing Logging enabled"));
#endif
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
		// 1/64;  20ms ==  20,000uS @ 4uS/tick ==  5,000 tics == 65535 - 5000  == 60035  // smooth
		// 1/64;  25ms ==  25,000uS @ 4uS/tick ==  6,250 tics == 65535 - 8750  == 59285  // smooth
		// 1/64;  35ms ==  35,000uS @ 4uS/tick ==  8,750 tics == 65535 - 8750  == 56785  // smooth
		// 1/64;  50ms ==  50,000uS @ 4uS/tick == 12,500 tics == 65535 - 12500 == 53035  // smooth
		// 1/64; 100ms == 100,000uS @ 4uS/tick == 25,000 tics == 65535 - 25000 == 40535  // jittery

	timer1_counter = 59285;
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
	static byte cachedServerAddress = 255;

	// Update Inputs
	singleMan->inputMan()->updateValues();

	// Just starting up, no address assigned to request one.
	if(singleMan->addrMan()->hasAddress() == false) {

		// The light automically goes into flashing blue mode while no address found
		// Send off a BLOCKING request for a new address
		singleMan->addrMan()->obtainAddress();

		// Print out the usage options now that the lantern is up and running
		singleMan->inputMan()->showOptions();
	}

	// Every IntervalBetweenNTPChecks, check the health of the sentries
	static unsigned long lastHealthCheck = 0;
	if(millis() > (lastHealthCheck + singleMan->radioMan()->getIntervalNTPCoordMessages())) {

		// always update the local health
		singleMan->healthMan()->updateSentryNTPRequestTime(singleMan->addrMan()->getAddress(), millis());
		singleMan->healthMan()->updateSentryLightLevel(singleMan->addrMan()->getAddress(), 255);

		singleMan->healthMan()->checkAllSentryHealth();
		lastHealthCheck = millis();

		// After healthcheck this lantern might have become the server; if so reset everyone's offsets
		if (cachedServerAddress != singleMan->healthMan()->getServerAddress()) {
			cachedServerAddress = singleMan->healthMan()->getServerAddress();
			// If the address changed and I'm the new server; tell everyone to reset their offset
			if (singleMan->healthMan()->getServerAddress() == singleMan->addrMan()->getAddress()) {

				RF24Message* ntpResetMessage = new RF24Message();
				ntpResetMessage->messageType = NTP_RESET_MESSAGE;
				ntpResetMessage->sentrySrcID = singleMan->addrMan()->getAddress();
				ntpResetMessage->sentryTargetID = 255;
				ntpResetMessage->param7_server_start = (unsigned long)singleMan->radioMan()->getMillisOffset();
				singleMan->radioMan()->sendMessage(ntpResetMessage);

				// Finally, set the offset back to 0 == -1* singleMan->radioMan()->getMillisOffset()
				singleMan->radioMan()->setMillisOffset(0);
			}
		}
	}

	// Transmit any queued send messageStack
	singleMan->radioMan()->checkSendWindow();

	// Collect any messages in the queue
	singleMan->radioMan()->checkRadioForData();

	// Finally, handle sentry or server loop
	if (singleMan->healthMan()->getServerAddress() == singleMan->addrMan()->getAddress())
		serverLoop();
	else
		sentryLoop();
}



void serverLoop() {
	static unsigned long nextColorMessage = millis() + singleMan->radioMan()->getIntervalColorMessages();
	static unsigned long nextNTPCoordMessage = millis() + singleMan->radioMan()->getIntervalNTPCoordMessages();

	// Handle any messages in the queue
	byte address = singleMan->addrMan()->getAddress();
	RF24Message *currMessage = singleMan->radioMan()->popMessageReceive();
	if(currMessage != NULL)
	{
		singleMan->healthMan()->updateSentryMessageTime(currMessage->sentrySrcID, millis());

		// Don't need to echo this message if it is now at final destination
		bool doEcho = (currMessage->sentryTargetID != address);

		switch(currMessage->messageType) {
			case NEW_ADDRESS_REQUEST:
				singleMan->addrMan()->sendNewAddressResponse();
				break;
			case NTP_CLIENT_REQUEST:
				singleMan->radioMan()->handleNTPClientRequest(currMessage);
				break;
			case KEEP_ALIVE_FROM_SENTRY:
				// Record the light level of the sentry
				singleMan->healthMan()->updateSentryLightLevel(currMessage->sentrySrcID, currMessage->param1_byte);
				break;
			case NTP_CLIENT_FINISHED:
				singleMan->healthMan()->updateSentryNTPRequestTime(currMessage->sentrySrcID, millis());
				singleMan->outputMan()->print(LOG_INFO, F("NTP_CLIENT_FINISHED:: sentrySrcID> "));
				singleMan->outputMan()->print(LOG_INFO, currMessage->sentrySrcID);
				singleMan->outputMan()->print(LOG_INFO, F("  avgOffset> "));
				singleMan->outputMan()->println(LOG_INFO, currMessage->param5_client_start);
				break;
		}

		if(doEcho) {
			singleMan->radioMan()->echoMessage(currMessage);
		} else {
			delete currMessage;
		}

		currMessage = NULL;
	}

	// Send coord messages
	if(millis() > nextNTPCoordMessage) {
		nextNTPCoordMessage = millis() + singleMan->radioMan()->getIntervalNTPCoordMessages();

		// If there is only one sentry, don't bother sending anything NTP_COORD_MESSAGES
		if(singleMan->healthMan()->totalSentries() > 1) {

			// Fire off a message to the next sentry to run NTP sequencing
			RF24Message* ntpStartMessage = new RF24Message();
			ntpStartMessage->messageType = NTP_COORD_MESSAGE;
			ntpStartMessage->sentrySrcID = singleMan->addrMan()->getAddress();
			ntpStartMessage->sentryTargetID = singleMan->healthMan()->getOldestNTPRequest();

			singleMan->radioMan()->sendMessage(ntpStartMessage);
		}
	}

	// Send the Color Message if it's time to update the nextColorMessage
	// OR if there is a button being pressed that'll change the pattern
	// if there is any sentries that have been added but haven't synced their time
	// don't send COLOR_MESSAGE_TO_SENTRY as it'll get in the way of the NTP requests
	if(singleMan->healthMan()->anyNonSyncedSentries() == false &&
		(millis() > nextColorMessage || singleMan->inputMan()->hasUnhandledInput()))
	{
		nextColorMessage = millis() + singleMan->radioMan()->getIntervalColorMessages();

		// generate newPatterns for LEDs since the interrupt will do the painting
		// this will use the inputManager to determine the new pattern
		singleMan->lightMan()->chooseNewPattern();

		// This is a pointer to the original, don't delete it.
		LightPattern* nextPattern = singleMan->lightMan()->getNextPattern();

		// send color updates
		RF24Message* lightMessage = new RF24Message();
		lightMessage->messageType = COLOR_MESSAGE_TO_SENTRY;
		lightMessage->sentrySrcID = singleMan->addrMan()->getAddress();
		lightMessage->sentryTargetID = 255;
		lightMessage->param1_byte = nextPattern->pattern;
		lightMessage->param2_byte = nextPattern->pattern_param1;
		lightMessage->param3_byte = nextPattern->pattern_param2;
		lightMessage->param4_client_end = nextPattern->pattern_param3;
		lightMessage->param5_client_start = nextPattern->pattern_param4;
		lightMessage->param6_server_end = nextPattern->pattern_param5;
		lightMessage->param7_server_start = nextPattern->startTime;

		singleMan->radioMan()->sendMessage(lightMessage);
	}
}



void sentryLoop() {
	// wait for signal to sync clock
	static NTP_state ntpState = NTP_DONE;
	static unsigned long timeOfLastNTPRequest = 0;
	static unsigned long timeOfNextKeepAlive = 0;

	// check the queue
	byte address = singleMan->addrMan()->getAddress();
	RF24Message *currMessage = singleMan->radioMan()->popMessageReceive();
	if(currMessage != NULL) {

		// Don't need to echo this message if it is now at final destination
		bool doEcho = (currMessage->sentryTargetID != address);

		// Update message time of sentry sending message
		singleMan->healthMan()->updateSentryMessageTime(currMessage->sentrySrcID, millis());

		switch(currMessage->messageType) {

			case NEW_ADDRESS_RESPONSE:
				// Already handled, just ignore it.
				break;

			case NTP_RESET_MESSAGE:
				{
					long offsetDelta = (long)currMessage->param7_server_start;
					singleMan->radioMan()->setMillisOffset(singleMan->radioMan()->getMillisOffset() - offsetDelta);
				}
				break;

			case NTP_COORD_MESSAGE:
				if(currMessage->sentryTargetID == address) {

					// Might currently be in the middle of NTP sequencing, don't reset
					if (ntpState == NTP_DONE) {
						ntpState = NTP_SEND_REQUEST;
						timeOfLastNTPRequest = 0;
					}
				}
				break;

			case NTP_SERVER_RESPONSE:
				if(currMessage->sentryTargetID == address) {
					if(ntpState == NTP_WAITING_FOR_RESPONSE) {
						long serverOffset = singleMan->radioMan()->calculateOffsetFromNTPResponseFromServer(currMessage);
						ntpState = singleMan->radioMan()->handleNTPServerOffset(serverOffset);
					}
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
				break;
		}

		if(doEcho) {
			singleMan->radioMan()->echoMessage(currMessage);
		} else {
			delete currMessage;
		}

		currMessage = NULL;
	}

	// Handle case where no response resulted in timeout
	if(ntpState == NTP_WAITING_FOR_RESPONSE) {
		if(millis() > timeOfLastNTPRequest + singleMan->radioMan()->ntpRequestTimeout()) {
			singleMan->outputMan()->println(LOG_ERROR, F("NTP Request timeout"));
			ntpState = singleMan->radioMan()->handleNTPServerOffset(0);
		}
	}

	// Send a keep_alive response to server
	if (millis() > timeOfNextKeepAlive) {
		timeOfNextKeepAlive = millis() + (singleMan->healthMan()->getDeathOffset() / 3);

		RF24Message* responseMessage = new RF24Message();
		responseMessage->messageType = KEEP_ALIVE_FROM_SENTRY;
		responseMessage->sentryTargetID = 0;
		responseMessage->sentrySrcID = singleMan->addrMan()->getAddress();
		responseMessage->param1_byte = singleMan->inputMan()->getAnalog(0);
		responseMessage->param4_client_end = currMessage->UID;
		responseMessage->param5_client_start = currMessage->UID;
		responseMessage->param6_server_end = currMessage->UID;
		responseMessage->param7_server_start = currMessage->UID;
		singleMan->radioMan()->sendMessage(responseMessage);
	}

	// If an NTP request is needed, send it now
	if(ntpState == NTP_SEND_REQUEST) {
			ntpState = singleMan->radioMan()->sendNTPRequestToServer();
			timeOfLastNTPRequest = millis();
	}
}
