#include "RadioManager.h"

#include <SPI.h>
#include "RF24.h"

#include "Utils.h"

// http://maniacbug.github.io/RF24/classRF24.html

RadioManager::RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		currentMillisOffset(0),
		messageQueue(NULL),
		radioAddresses{ 0xF0F0F0F0AA, 0xF0F0F0F0BB, 0xF0F0F0F0CC, 0xF0F0F0F0DD }
{
}

void RadioManager::init() {
	// Init Radio
	rf24.begin();

	// RF24_PA_MAX is default.
	//	rf24.setPALevel(RF24_PA_MAX);
	rf24.setPALevel(RF24_PA_LOW);

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

	// Everyone is listening on [0], will broadcast on [0] and switch back after
	rf24.openWritingPipe(radioAddresses[1]);
	rf24.openReadingPipe(1, radioAddresses[0]);

	// kick off with listening
	rf24.startListening();

	// Seed the random generator for message UID
	randomSeed(analogRead(1));
}

long RadioManager::generateUID() {
	return(random(-2147483648, 2147483647));
}

void RadioManager::setMillisOffset(long newOffset) {
//	Serial.print("RadioManager::setMillisOffset    newOffset: ");
//	Serial.println(newOffset);
	currentMillisOffset = newOffset;
}

unsigned long RadioManager::getAdjustedMillis() {
	return millis() + currentMillisOffset;
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
			newMessage->server_start = micros();

		pushMessage(newMessage);
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

void RadioManager::pushMessage(RF24Message *newMessage) {
	if(messageQueue == NULL)
	{
		messageQueue = new MessageNode();
		messageQueue->next = NULL;
		messageQueue->message = newMessage;
	}
	else
	{
		MessageNode *tail = NULL;
		while((tail = messageQueue->next) != NULL)
		{
			MessageNode *newNode = new MessageNode();
			tail->next = newNode;
			newNode->message = newMessage;
			newNode->next = NULL;
		}
	}
}


// Ponder sending out on multiple pipes ??
void RadioManager::sendMessage(RF24Message messageToSend) {
	rf24.stopListening();

	rf24.closeReadingPipe(1);
	rf24.openWritingPipe(radioAddresses[0]);
	rf24.write(&messageToSend, sizeof(RF24Message));

	rf24.openWritingPipe(radioAddresses[1]);
	rf24.openReadingPipe(1, radioAddresses[0]);

	rf24.startListening();
}

void RadioManager::sendNTPRequestToServer()
{
	RF24Message ntpOut;
	ntpOut.messageType = NTP_CLIENT_REQUEST;
	ntpOut.sentryRequestID = getAddress();
	ntpOut.UID = generateUID();
	ntpOut.server_end = 0;
	ntpOut.server_start = 0;
	ntpOut.client_end = 0;
	ntpOut.client_start = micros();

	sendMessage(ntpOut);
}

#define OFFSET_SUCCESSES 7
#define MIDDLE_SUCCESSES 3
bool RadioManager::handleNTPServerResponse(RF24Message* ntpMessage) {
	static long offsetCollection[OFFSET_SUCCESSES];
	static int currOffsetIndex = 0;

	bool inNTPLoop = true;

	long ntpOffset = calculateOffsetFromNTPResponseFromServer(ntpMessage);
	if(ntpOffset != 0)
	{
		offsetCollection[currOffsetIndex] = ntpOffset;
		currOffsetIndex++;

		// Once there are OFFSET_SUCCESSES offsets, average and set it.
		if(currOffsetIndex==OFFSET_SUCCESSES) {

			for(int i=1;i<OFFSET_SUCCESSES;++i)
			{
					for(int j=0;j<(OFFSET_SUCCESSES-i);++j)
							if(offsetCollection[j] > offsetCollection[j+1])
							{
									long temp = offsetCollection[j];
									offsetCollection[j] = offsetCollection[j+1];
									offsetCollection[j+1] = temp;
							}
			}

			int excludeCount = (OFFSET_SUCCESSES - MIDDLE_SUCCESSES) / 2;

			long long summedOffset = 0;
			for(int i=excludeCount; i<excludeCount+MIDDLE_SUCCESSES;i++)
				summedOffset += offsetCollection[i];

			this->setMillisOffset(summedOffset/MIDDLE_SUCCESSES);

			currOffsetIndex = 0;
			inNTPLoop = false;
		}
	}

	return(inNTPLoop);
}

long RadioManager::calculateOffsetFromNTPResponseFromServer(RF24Message *ntpMessage) {
	// Spew Debug Info
/*
	Serial.print("VagueTxRxTime: ");
	Serial.print(ntpMessage.client_end - ntpMessage.client_start);
	Serial.print("    client_start: ");
	Serial.print(ntpMessage.client_start);
	Serial.print("    client_end: ");
	Serial.print(ntpMessage.client_end);
	Serial.print("    server_start: ");
	Serial.print(ntpMessage.server_start);
	Serial.print("    server_end: ");
	Serial.println(ntpMessage.server_end);
*/

	/* Finally have enough data to Do The Math
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

	//	long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;
	long t3_t0 = ntpMessage->client_end - ntpMessage->client_start;
	long t2_t1 = ntpMessage->server_end - ntpMessage->server_start;
	long halfRtripDelay = (t3_t0 - t2_t1) / 2;

	// Update offset to use the delay
	offset = offset + halfRtripDelay;

	// Need to convert from Micros to Millis
	offset = ((offset+500)/1000);

	// return
	return(offset);
}


void RadioManager::handleNTPClientRequest(RF24Message* ntpMessage) {
	static int getNTPRequestCount=0;

	// since we're reusing the message; need to update the
	// messageID or else echoing won't work
	ntpMessage->UID = generateUID();

	// Set the payload
	ntpMessage->messageType = NTP_SERVER_RESPONSE;
	ntpMessage->server_end = micros();
	sendMessage(*ntpMessage);
}
