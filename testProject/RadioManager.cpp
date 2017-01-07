#include "RadioManager.h"

#include <SPI.h>
#include "RF24.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"

// http://maniacbug.github.io/RF24/classRF24.html

RadioManager::RadioManager(SingletonManager* _singleMan, uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		singleMan(_singleMan),
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		radioAddresses{ {0xF0F0F0F011LL, 0xF0F0F0F022LL},
										{0xF0F0F0F044LL, 0xF0F0F0F055LL},
										{0xF0F0F0F077LL, 0xF0F0F0F088LL},
										{0xF0F0F0F0AALL, 0xF0F0F0F0BBLL} }
{
	// Init Radio
	rf24.begin();

	// RF24_PA_MAX is default.
	//	rf24.setPALevel(RF24_PA_MAX);
	rf24.setPALevel(RF24_PA_HIGH);
	//	rf24.setPALevel(RF24_PA_LOW);

	// Set the data rate to the slowest (and most reliable) speed
	rf24.setDataRate(RF24_1MBPS);

	// Disable dynamicAck so we can send to multiple sentries
	rf24.setAutoAck(false);

	/* EXAMPLE from:
	http://tmrh20.github.io/RF24/classRF24.html#a6253607ac2a1995af91a35cea6899c31
	radio.enableDynamicAck();
	radio.write(&data,32,1);  // Sends a payload with no acknowledgement requested
	radio.write(&data,32,0);  // Sends a payload using auto-retry/autoACK
	*/

	rf24.setPayloadSize(sizeof(RF24Message));

	rf24.openWritingPipe(radioAddresses[0][1]);
	rf24.openReadingPipe(1, radioAddresses[0][0]);

	// kick off with listening
	rf24.startListening();

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

unsigned long RadioManager::generateUID() {
	unsigned long generatedUID = micros() << 3;
	generatedUID |= singleMan->addrMan()->getZone();

	return(generatedUID);
}

// continually polls for available data, and when found pushes
//	to the queue and then breaks to notify.
bool RadioManager::checkRadioForData() {

	if(rf24.available())
	{
		RF24Message *newMessage = new RF24Message();
		rf24.read(newMessage, sizeof(RF24Message));

		if(pushMessage(newMessage) == false)
			delete newMessage;
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

		// Sending messages requires a new UID; but don't want
		//	to change UID when echoing!
		messageToSend.UID = generateUID();

		internalSendMessage(messageToSend);
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
	}

	rf24.stopListening();
	rf24.closeReadingPipe(1);
	rf24.openWritingPipe(radioAddresses[0][0]);
	rf24.write(&messageToSend, sizeof(RF24Message));
	rf24.openWritingPipe(radioAddresses[0][1]);
	rf24.openReadingPipe(1, radioAddresses[0][0]);
	rf24.startListening();
}
