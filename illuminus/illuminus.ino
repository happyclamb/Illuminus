#include <Arduino.h>

#include "IlluminusDefs.h"

#include "SingletonManager.h"
#include "RadioManager.h"
#include "LightManager.h"
#include "AddressManager.h"

SingletonManager *singleMan = NULL;

void setup() {
	Serial.begin(9600);

	// Create the holder for global objects
	singleMan = new SingletonManager();

	// Initilize the AddressHandler
	AddressManager *addrMan = new AddressManager(singleMan);

	// Initialize the Radio handler
	RadioManager *radioMan = new RadioManager(singleMan, RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
	delay(5); // Wait for radio to init before continuing

	// Initilize the LightHandler
	LightManager *lightMan = new LightManager(singleMan);
	delay(5); // Wait for light to init before continuing

	// Start interrupt handler for LightManagement
	init_TIMER2_irq();
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
		forceNTPCheck = true;
	}	else {
		// Now handle sentry or server loop
		if (singleMan->addrMan()->getAddress() == 0)
			serverLoop();
		else
			sentryLoop(forceNTPCheck);
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

	if(millis() > lastNTPCheck + TIME_BETWEEN_NTP_MSGS)
	{
		static int nextSentryToRunNTP = 1;

		// Fire off a message to the next sentry to run an NTPloop
		RF24Message ntpStartMessage;
		ntpStartMessage.messageType = NTP_COORD_MESSAGE;
		// only request response if it'll be used (aka: during boot sequence)
		ntpStartMessage.byteParam1 = bootNTPSequence ? 1 : 0;
		ntpStartMessage.sentrySrcID = 0;
		ntpStartMessage.sentryTargetID = nextSentryToRunNTP++;

		singleMan->radioMan()->sendMessage(ntpStartMessage);

		// Wrap back to start; reset bootNTPSequence if set.
		if(nextSentryToRunNTP > singleMan->addrMan()->getLanternCount()) {
			nextSentryToRunNTP = 1;
			bootNTPSequence = false;
		}

		// Update lastNTPCheck
		lastNTPCheck = millis();
	} else if(millis() > lastLEDUpdateCheck + TIME_BETWEEN_LED_MSGS) {

		// generate newPatterns for LEDs since the interrupt will do the painting
		singleMan->lightMan()->chooseNewPattern();

		LightPattern nextPattern = singleMan->lightMan()->getNextPattern();
		unsigned long colorStartTime = singleMan->lightMan()->getNextPatternStartTime();

		// send color updates
		RF24Message lightMessage;
		lightMessage.messageType = COLOR_MESSAGE;
		lightMessage.byteParam1 = nextPattern.pattern;
		lightMessage.byteParam2 = nextPattern.pattern_param1;
		lightMessage.byteParam3 = singleMan->addrMan()->getLanternCount();
		lightMessage.sentrySrcID = 0;
		lightMessage.sentryTargetID = 255;
		lightMessage.server_start = colorStartTime;

		// Fire off a message to the next sentry to run an NTPupdate
		singleMan->radioMan()->sendMessage(lightMessage);

		// Update lastLEDUpdateCheck
		lastLEDUpdateCheck = millis();
	}
}

void sentryLoop(bool forceNTPCheck) {
	static bool inNTPLoop = false;

	if(forceNTPCheck) {
		inNTPLoop = true;
		singleMan->radioMan()->setInformServerWhenNTPDone(true);
	}

	// spin until there is something in a queue
	singleMan->radioMan()->checkRadioForData();
	RF24Message *currMessage = singleMan->radioMan()->popMessage();

	byte address = singleMan->addrMan()->getAddress();
	if(currMessage != NULL)
	{
		bool doEcho = true;
		switch(currMessage->messageType)
		{
			case NTP_COORD_MESSAGE:
				if(currMessage->sentryTargetID == address)
				{
					// Don't need to echo this message as it is now at final destination
					singleMan->radioMan()->setInformServerWhenNTPDone(currMessage->byteParam1 == 1 ? true : false);
					inNTPLoop = true;
					doEcho = false;
				}
				break;
			case COLOR_MESSAGE:
				LightPattern nextPattern;
				nextPattern.pattern = currMessage->byteParam1;
				nextPattern.pattern_param1 = currMessage->byteParam2;

				// update pattern for LEDs since the interrupt will do the painting
				singleMan->lightMan()->setNextPattern(nextPattern, currMessage->server_start);
				// Finally, update number of active sentries
				singleMan->addrMan()->setLanternCount(currMessage->byteParam3);
				break;
			case NTP_SERVER_RESPONSE:
				if(currMessage->sentryTargetID == address)
				{
					// Don't need to echo this message as it is now complete
					inNTPLoop = singleMan->radioMan()->handleNTPServerResponse(currMessage);
					doEcho = false;
				}
				break;
		}

		if(doEcho)
			singleMan->radioMan()->echoMessage(*currMessage);

		delete currMessage;
	}

	if(inNTPLoop)
		singleMan->radioMan()->sendNTPRequestToServer();
}
