#include <Arduino.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
#include "RadioManager.h"
#include "LightManager.h"
#include "AddressManager.h"
#include "HealthManager.h"

SingletonManager *singleMan = NULL;

void setup() {
//	Serial.begin(115200);
	Serial.begin(9600);
	// Create the holder for global objects
	singleMan = new SingletonManager();

	// Initilize the AddressManager
	AddressManager *addrMan = new AddressManager(singleMan);

	// Initilize the HealthManager
	HealthManager *healthMan = new HealthManager(singleMan);

	// Initialize the RadioManager
	RadioManager *radioMan = new RadioManager(singleMan, RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
	delay(5); // Wait for radio to init before continuing

	// Initilize the LightManager
	LightManager *lightMan = new LightManager(singleMan);
	delay(5); // Wait for light to init before continuing

	// Start interrupt handler for LightManagement
	init_TIMER2_irq();

	Serial.println("Setup complete");

	info_println("Info Logging enabled");
	debug_println("Debug Logging enabled");
	timing_println("Timing Logging enabled");
}

// initialize timer2 to redraw the LED strip and BigLight
void init_TIMER2_irq()
{
	// disable all interrupts while changing them
	noInterrupts();

	// Select clock source: internal I/O clock
	ASSR &= ~(1<<AS2);

	// Configure timer2 in normal mode (pure counting, no PWM etc.)
	TCCR2A = 0;
	TCCR2B = 0;

	// Set maximum divisor of 1/1024
	TCCR2B |= (1 << CS20);
	TCCR2B |= (1 << CS21);
	TCCR2B |= (1 << CS22);

	// Disable Compare Match A interrupt enable (only want overflow)
	TIMSK2 = 0;
	TIMSK2 |= (1<<TOIE2);

	// Finally load the timer start counter; lowest is where to count
	// start as interrupt happens at wrap around
	TCNT2 = 0; // <-- maximum time possible between interrupts

	// enable all interrupts now that things are ready to go
	interrupts();
}

// interrupt service routine for
ISR(TIMER2_OVF_vect)
{
	singleMan->lightMan()->redrawLights();

	// load timer last to maximize time until next call
	TCNT2 = 0; // <-- maximum time possible between interrupts
}

void loop() {
	// LED control is handled by interrupt on timer2

	bool forceNTPCheck = false;
	if(singleMan->addrMan()->hasAddress() == false) {
		// The light automically goes into flashing blue mode while no address found
		// Send off a blocking request for a new address
		singleMan->addrMan()->obtainAddress();

		// First time through, start with a request for an NTP check
		forceNTPCheck = true;

		// Always set the 0 address since nothing runs without a server
		// If this was a sentry, add all its children to the healthMan
		for(byte i=0; i <= singleMan->addrMan()->getAddress(); i++)
			singleMan->healthMan()->updateSentryNTPRequestTime(i);
	}	else {
		// Now handle sentry or server loop
		if (singleMan->addrMan()->getAddress() == 0)
			serverLoop();
		else
			sentryLoop(forceNTPCheck);
	}

	static unsigned long lastHealthCheck = 0;
	if(millis() > lastHealthCheck + TIME_BETWEEN_NTP_MSGS) {
		// Every TIME_BETWEEN_NTP_MSGS lets check the health of the sentries
		singleMan->healthMan()->checkAllSentryHealth();
		lastHealthCheck = millis();
	}

	// Small delay to allow for states to settle down.
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
			case NTP_CLIENT_FINISHED:
				// spamming NTP requests will flood the system; so only
				// do the NTPCheck skipping for first run through of the sentries.
				if(bootNTPSequence)
				{
					// successfully finished current NTP, force jump to next sentry
					lastNTPCheck = millis() - TIME_BETWEEN_NTP_MSGS - 10;
				}
				break;
		}

		// The server never echos messages so cleanup
		delete currMessage;
	}

	if(millis() > lastNTPCheck + TIME_BETWEEN_NTP_MSGS) {

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
			ntpStartMessage.byteParam1 = bootNTPSequence ? 1 : 0;

			singleMan->radioMan()->sendMessage(ntpStartMessage);

			info_print("Sending NTP COORD message to: ");
			info_println(nextSentryToRunNTP);
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

	if(millis() > lastLEDUpdateCheck + TIME_BETWEEN_LED_MSGS) {

		// generate newPatterns for LEDs since the interrupt will do the painting
		singleMan->lightMan()->chooseNewPattern();

		LightPattern nextPattern = singleMan->lightMan()->getNextPattern();

		// send color updates
		RF24Message lightMessage;
		lightMessage.messageType = COLOR_MESSAGE;
		lightMessage.sentrySrcID = 0;
		lightMessage.sentryTargetID = 255;
		lightMessage.byteParam1 = nextPattern.pattern;
		lightMessage.byteParam2 = nextPattern.pattern_param1;
		lightMessage.server_start = nextPattern.startTime;

		singleMan->radioMan()->sendMessage(lightMessage);
		info_print("Sending Light Update    ");
		nextPattern.printPattern();
		info_println("");

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
					info_println("NTP_COORD_MESSAGE message received");

					// Don't need to echo this message as it is now at final destination
					singleMan->radioMan()->setInformServerWhenNTPDone(currMessage->byteParam1 == 1 ? true : false);
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

			case COLOR_MESSAGE:
				LightPattern newPattern;
				newPattern.pattern = currMessage->byteParam1;
				newPattern.pattern_param1 = currMessage->byteParam2;
				newPattern.startTime = currMessage->server_start;

				// update pattern for LEDs since the interrupt will do the painting
				singleMan->lightMan()->setNextPattern(newPattern);

				info_print("COLOR_MESSAGE message received.  Next pattern is:");
				newPattern.printPattern();
				info_println();
				break;
		}

		if(doEcho)
			singleMan->radioMan()->echoMessage(*currMessage);

		delete currMessage;
	}

	if(ntpState == NTP_WAITING_FOR_RESPONSE) {
		// lets assume we need 3* the number of requests sent, so each request has that long to timeout.
		unsigned long ntpRequestTimeout = TIME_BETWEEN_NTP_MSGS / ((unsigned long ) NTP_OFFSET_SUCCESSES_REQUIRED * 3);
		if(millis() > timeOfLastNTPRequest + ntpRequestTimeout) {
			debug_println("NTP Request timeout");
			// assume that request has timedout and send another
			ntpState = NTP_SEND_REQUEST;
		}
	}

	if(ntpState == NTP_SEND_REQUEST) {
			debug_println("Sending Request");
			ntpState = singleMan->radioMan()->sendNTPRequestToServer();
			timeOfLastNTPRequest = millis();
	}
}
