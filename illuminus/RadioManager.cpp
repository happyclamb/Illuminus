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

	rf24.openWritingPipe(radioAddresses[singleMan->addrMan()->getZone()][1]);
	rf24.openReadingPipe(1, radioAddresses[singleMan->addrMan()->getZone()][0]);

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
	generatedUID |= singleMan->addrMan()->getAddress();

	return(generatedUID);
}

void RadioManager::setMillisOffset(long newOffset) {
	currentMillisOffset = newOffset;
	info_print("RadioManager::setMillisOffset:: ");
	info_print(currentMillisOffset);
	debug_print("      CurrentTime: ");
	debug_print(millis());
	debug_print("      AdjustedTime: ");
	debug_print(getAdjustedMillis());
	info_println();
}

unsigned long RadioManager::getAdjustedMillis() {
	return millis() + currentMillisOffset;
}

bool RadioManager::setInformServerWhenNTPDone(bool newValue) {
	informServerWhenNTPDone = newValue;
}

// continually polls for available data, and when found pushes
//	to the queue and then breaks to notify.
bool RadioManager::checkRadioForData() {

	if(rf24.available())
	{
		RF24Message *newMessage = new RF24Message();
		rf24.read(newMessage, sizeof(RF24Message));

		// If the payload is a NTP_CLIENT_REQUEST then immediately
		//	update server_start time
		if(newMessage->messageType == NTP_CLIENT_REQUEST)
			newMessage->server_start = millis();
		else if(newMessage->messageType == NTP_SERVER_RESPONSE)
				newMessage->client_end = millis();

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
	rf24.openWritingPipe(radioAddresses[singleMan->addrMan()->getZone()][0]);
	rf24.write(&messageToSend, sizeof(RF24Message));
	rf24.openWritingPipe(radioAddresses[singleMan->addrMan()->getZone()][1]);
	rf24.openReadingPipe(1, radioAddresses[singleMan->addrMan()->getZone()][0]);
	rf24.startListening();


	// Doing one realy quick and dirty resend here after a short delay
	delay(5);
	rf24.stopListening();
	rf24.closeReadingPipe(1);
	rf24.openWritingPipe(radioAddresses[singleMan->addrMan()->getZone()][0]);
	rf24.write(&messageToSend, sizeof(RF24Message));
	rf24.openWritingPipe(radioAddresses[singleMan->addrMan()->getZone()][1]);
	rf24.openReadingPipe(1, radioAddresses[singleMan->addrMan()->getZone()][0]);
	rf24.startListening();
}

void RadioManager::sendMessage(RF24Message messageToSend) {

		// Sending messages requires a new UID; but don't want
		//	to change UID when echoing!
		messageToSend.UID = generateUID();

		internalSendMessage(messageToSend);
}

void RadioManager::echoMessage(RF24Message messageToEcho) {

	// Only care about checking for resending as
	//	we *do* want to echo received messages
	for(int i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(sentUIDs[i] == messageToEcho.UID) {
			return;
		}
	}

	internalSendMessage(messageToEcho);
}

NTP_state RadioManager::sendNTPRequestToServer() {

	RF24Message ntpOut;
	ntpOut.messageType = NTP_CLIENT_REQUEST;
	ntpOut.sentrySrcID = singleMan->addrMan()->getAddress();
	ntpOut.sentryTargetID = 0;
	ntpOut.server_end = 0;
	ntpOut.server_start = 0;
	ntpOut.client_end = 0;
	ntpOut.client_start = millis();

	sendMessage(ntpOut);

	return(NTP_WAITING_FOR_RESPONSE);
}

NTP_state RadioManager::handleNTPServerResponse(RF24Message* ntpMessage) {
	static long offsetCollection[NTP_OFFSET_SUCCESSES_REQUIRED];
	static byte currOffsetIndex = 0;

	NTP_state returnState = NTP_SEND_REQUEST;

	long ntpOffset = calculateOffsetFromNTPResponseFromServer(ntpMessage);
	if(ntpOffset != 0)
	{
		offsetCollection[currOffsetIndex] = ntpOffset;
		currOffsetIndex++;
		// Once there are OFFSET_SUCCESSES offsets, average and set it.
		if(currOffsetIndex == NTP_OFFSET_SUCCESSES_REQUIRED) {

			for(int i=1;i<NTP_OFFSET_SUCCESSES_REQUIRED;++i)
			{
					for(int j=0;j<(NTP_OFFSET_SUCCESSES_REQUIRED-i);++j)
							if(offsetCollection[j] > offsetCollection[j+1])
							{
									long temp = offsetCollection[j];
									offsetCollection[j] = offsetCollection[j+1];
									offsetCollection[j+1] = temp;
							}
			}

			int excludeCount = (NTP_OFFSET_SUCCESSES_REQUIRED - NTP_OFFSET_SUCCESSES_USED) / 2;

			long long summedOffset = 0;
			for(int i=excludeCount; i<excludeCount+NTP_OFFSET_SUCCESSES_USED; i++)
				summedOffset += offsetCollection[i];

			long averagedOffset = summedOffset/NTP_OFFSET_SUCCESSES_USED;
			this->setMillisOffset(averagedOffset);

			if(informServerWhenNTPDone) {
				// Tell the server that syncronization has happened.
				RF24Message ntpClientFinished;
				ntpClientFinished.messageType = NTP_CLIENT_FINISHED;
				ntpClientFinished.sentrySrcID = singleMan->addrMan()->getAddress();
				ntpClientFinished.sentryTargetID = 0;
				ntpClientFinished.client_start = averagedOffset;
				sendMessage(ntpClientFinished);
			}

			// reset variables to wait for next NTP sync
			informServerWhenNTPDone = false;
			currOffsetIndex = 0;
			returnState = NTP_DONE;
		}
	}

	return(returnState);
}

long RadioManager::calculateOffsetFromNTPResponseFromServer(RF24Message *ntpMessage) {

	timing_println("***calculateOffsetFromNTPResponseFromServer ************************");

	timing_print("calculateOffset --> VagueTxRxTime: ");
	timing_print(ntpMessage->client_end - ntpMessage->client_start);

	timing_print("    client_start: ");
	timing_print(ntpMessage->client_start);
	timing_print("    client_end: ");
	timing_print(ntpMessage->client_end);
	timing_print("    server_start: ");
	timing_print(ntpMessage->server_start);
	timing_print("    server_end: ");
	timing_println(ntpMessage->server_end);

	/* Have enough data to Do The Math
			https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
		offSet = ((t1 - t0) + (t2-t3)) / 2
		halfRtripDelay = ((t3-t0) - (t2-t1)) / 2
				t0 == client_start
				t1 == server_start
				t2 == server_end
				t3 == client_end
	*/
	//  long offSet = ((long)(timeData.server_start - timeData.client_start) + (long)(timeData.server_end - timeData.client_end)) / (long)2;
	long t1_t0 = ntpMessage->server_start - ntpMessage->client_start;
	long t2_t3 = ntpMessage->server_end - ntpMessage->client_end;
	long long offset_LL = (t1_t0 + t2_t3);
	long offset = (long) (offset_LL / 2);

	timing_print("t1_t0: ");
	timing_print(t1_t0);
	timing_print("    t2_t3: ");
	timing_print(t2_t3);
	timing_print("    offset_LL: ");
	timing_print(t1_t0 + t2_t3);
	timing_print("    offset: ");
	timing_println(offset);


	//	long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;
	long t3_t0 = ntpMessage->client_end - ntpMessage->client_start;
	long t2_t1 = ntpMessage->server_end - ntpMessage->server_start;
	long halfRtripDelay = (t3_t0 - t2_t1) / 2;

	// Update offset to use the delay
	offset = offset + halfRtripDelay;

	timing_print("t3_t0: ");
	timing_print(t3_t0);
	timing_print("    t2_t1: ");
	timing_print(t2_t1);
	timing_print("    halfRtripDelay: ");
	timing_print(halfRtripDelay);
	timing_print("    offset: ");
	timing_println(offset);

	// return
	return(offset);
}


void RadioManager::handleNTPClientRequest(RF24Message* ntpMessage) {
	static int getNTPRequestCount=0;

	// Set the payload
	ntpMessage->messageType = NTP_SERVER_RESPONSE;
	ntpMessage->sentryTargetID = ntpMessage->sentrySrcID;
	ntpMessage->sentrySrcID = 0;

	ntpMessage->server_end = millis();
	sendMessage(*ntpMessage);
}
