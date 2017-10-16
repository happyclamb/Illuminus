#include "RadioManager.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"

// http://maniacbug.github.io/RF24/classRF24.html
RadioManager::RadioManager(SingletonManager* _singleMan, uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		singleMan(_singleMan),
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		pipeAddresses{ { 0xABCDABCD71LL, 0x544d52687CLL },
		               { 0xBCDABCDA71LL, 0xBCDABCDAC1LL },
		               { 0xCDABCDAB71LL, 0xCDABCDABC1LL },
		               { 0xDABCDABC71LL, 0xDABCDABCC1LL } }
{
	// initialize RF24 printf library
	printf_begin();

	resetRadio();

	// init the sentUIDs array
	for(int i=0; i<MAX_STORED_MSG_IDS; i++)
	{
		sentUIDs[i] = 0;
		receivedUIDs[i] = 0;
	}

	// Seed the random generator for message UID
	analogSeed = analogRead(1);
	randomSeed(analogSeed);

	singleMan->setRadioMan(this);
}

void RadioManager::resetRadio() {
	// Init Radio
	if(rf24.begin() == false)	{
		info_println("RADIO INITIALIZE FAILURE");
	} else {
		info_println("RADIO DETAILS BEFORE RESET");
		rf24.printDetails();

		// Reset Failure; Edit RF24_config.h to enable
		// C:\Users\clamb\Documents\Arduino\libraries\RF24\RF24_config.h
		rf24.failureDetected = 0;

		// Disable dynamicAck so we can send to multiple sentries
		rf24.setAutoAck(false);

		// clamb: something to consider?
		// void 	setRetries (uint8_t delay, uint8_t count)

		//	The driver will delay for this duration when stopListening() is called.
		//		If AutoACK is disabled, this can be set as low as 0.
		rf24.txDelay = 5;

		// On all devices but Linux and ATTiny, a small delay is added to the CSN toggling function
		// This is intended to minimise the speed of SPI polling due to radio commands
		// If using interrupts or timed requests, this can be set to 0 Default:5
		rf24.csDelay = 5;

		// RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
		rf24.setPALevel(RF24_PA_HIGH);

		// Set the data rate to the slowest (and most reliable) speed
		// RF24_250KBPS for 250kbs (only available on some chips),
		// RF24_1MBPS for 1Mbps
		// RF24_2MBPS for 2Mbps (only available on some chips)
		rf24.setDataRate(RF24_1MBPS);

		// void 	setPayloadSize (uint8_t size)
		// uint8_t 	getPayloadSize (void)
		// uint8_t 	getDynamicPayloadSize (void)
		rf24.setPayloadSize(sizeof(RF24Message));

		// clamb: todo: set this to 3?
		// void 	setAddressWidth (uint8_t a_width)
		// rf24.setAddressWidth(3);

		// DisableCRC since not using autoACK? NO - still need it
		//void 	setCRCLength (rf24_crclength_e length)
		//rf24_crclength_e 	getCRCLength (void)
		// rf24.disableCRC();

		// Channel to be transmitting on; default 76
		// void 	setChannel (uint8_t channel)
		// uint8_t 	getChannel (void)
		// rf24.setChannel(5);

		// Long term change to interrups?
		// http://forcetronic.blogspot.ca/2016/07/using-nrf24l01s-irq-pin-to-generate.html
		// void 	maskIRQ (bool tx_ok, bool tx_fail, bool rx_ready)

		// Not using
		// void 	enableDynamicPayloads (void)
		// void 	enableDynamicAck ()
		// void 	enableAckPayload (void)

		rf24.openWritingPipe(pipeAddresses[0][1]);
		rf24.openReadingPipe(1, pipeAddresses[0][0]);

		// kick off with listening
		rf24.startListening();

		// https://forum.arduino.cc/index.php?topic=215065.0
		// http://forum.arduino.cc/index.php?topic=216306.0
		info_println("RADIO DETAILS AFTER RESET");
		rf24.printDetails();
	}
}

bool RadioManager::checkForInterference() {
	bool returnVal = false;

	if(rf24.testRPD() || rf24.testCarrier())
		returnVal = true;

	return returnVal;
}

unsigned long RadioManager::generateUID() {
	unsigned long generatedUID = micros() << 3;
	generatedUID |= singleMan->addrMan()->getZone();

	return(generatedUID);
}

// continually polls for available data, and when found pushes
//	to the queue and then breaks to notify.
bool RadioManager::checkRadioForData() {

	if(rf24.failureDetected) {
		info_println("RADIO ERROR DETECTED ON RECEIVE, resetting");
		resetRadio();
	} else {
		if(rf24.available())
		{
			info_println("DATA RECEIVED");

			RF24Message *newMessage = new RF24Message();
			rf24.read(newMessage, sizeof(RF24Message));

			if(pushMessage(newMessage) == false)
				delete newMessage;
		}
	}

	return (messageQueue == NULL);
}

RF24Message* RadioManager::popMessage() {
	RF24Message* returnMessage = NULL;
	if(messageQueue != NULL)
	{
		returnMessage = messageQueue->message;
		MessageNode *newHead = messageQueue->next;
		delete messageQueue;
		messageQueue = newHead;
	}

	return(returnMessage);
}

bool RadioManager::pushMessage(RF24Message *newMessage) {
	// If we've already sent or received this message let it die here
	for(int i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(sentUIDs[i] == newMessage->UID) {
			return false;
		}
		if(receivedUIDs[i] == newMessage->UID) {
			return false;
		}
	}

	if(messageQueue == NULL)
	{
		messageQueue = new MessageNode();
		messageQueue->next = NULL;
		messageQueue->message = newMessage;
	}
	else
	{
		MessageNode *lastNode = messageQueue;

		// find last location to insert message
		while(lastNode->next != NULL)
			lastNode = lastNode->next;

		MessageNode *newNode = new MessageNode();
		lastNode->next = newNode;
		newNode->message = newMessage;
	}

	// Store this as received
	receivedUIDs[nextReceivedUIDIndex++] = newMessage->UID;
	if(nextReceivedUIDIndex == MAX_STORED_MSG_IDS)
		nextReceivedUIDIndex = 0;

	return true;
}

void RadioManager::sendMessage(RF24Message messageToSend) {
	if(rf24.failureDetected) {
		info_println("RADIO ERROR DETECTED ON SEND, resetting");
		resetRadio();
	} else {
		// Sending messages requires a new UID; but don't want
		//	to change UID when echoing!
		messageToSend.UID = generateUID();

		internalSendMessage(messageToSend);
	}
}

// Ponder sending multiple times ??
void RadioManager::internalSendMessage(RF24Message messageToSend) {

	// Mark as sent
	bool alreadySent = false;
	for(int i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(sentUIDs[i] == messageToSend.UID) {
			alreadySent = true;
			break;
		}
	}

	// Store this as sent
	if(alreadySent == false) {
		sentUIDs[nextSentUIDIndex++] = messageToSend.UID;
		if(nextSentUIDIndex == MAX_STORED_MSG_IDS)
			nextSentUIDIndex = 0;

		rf24.stopListening();

		rf24.closeReadingPipe(1);
		rf24.openWritingPipe(pipeAddresses[0][0]);

		rf24.write(&messageToSend, sizeof(RF24Message));
		rf24.flush_tx();

		rf24.openWritingPipe(pipeAddresses[0][1]);
		rf24.openReadingPipe(1, pipeAddresses[0][0]);

		rf24.startListening();
	}
}
