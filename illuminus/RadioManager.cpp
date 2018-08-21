#include "RadioManager.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"

// http://maniacbug.github.io/RF24/classRF24.html
RadioManager::RadioManager(SingletonManager* _singleMan, uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		singleMan(_singleMan),
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		pipeAddresses{ { 0xABCDABCD71LL, 0xABCDABCD75LL, 0xABCDABCD79LL },
		               { 0xBCDABCDA71LL, 0xBCDABCDAC5LL, 0xBCDABCDAC9LL },
		               { 0xCDABCDAB71LL, 0xCDABCDABC5LL, 0xCDABCDABC9LL },
		               { 0xDABCDABC71LL, 0xDABCDABCC5LL, 0xDABCDABCC9LL } }
{
	// initialize RF24 radio
	resetRadio();

	randomSeed(analogRead(0));

	// init the sentUIDs array
	for(byte i=0; i<MAX_STORED_MSG_IDS; i++)
	{
		this->sentUIDs[i] = 4294967295;
		this->receivedUIDs[i] = 4294967295;
	}

	singleMan->setRadioMan(this);
}

void RadioManager::resetRadio() {
	// Init Radio
	if(rf24.begin() == false)	{
		singleMan->outputMan()->println(LOG_ERROR, F("RADIO INITIALIZE FAILURE"));
	} else {

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

		for (byte i=0; i<6; i++) rf24.closeReadingPipe(i);
		byte currentZone = singleMan->addrMan()->getZone();
		rf24.openWritingPipe(this->pipeAddresses[currentZone][0]);
		rf24.openReadingPipe(1, this->pipeAddresses[currentZone][1]);
		rf24.openReadingPipe(2, this->pipeAddresses[currentZone][2]);

		// kick off with listening
		rf24.startListening();

		// https://forum.arduino.cc/index.php?topic=215065.0
		// http://forum.arduino.cc/index.php?topic=216306.0
	}
}

bool RadioManager::checkForInterference() {
	bool returnVal = false;

	if(rf24.testRPD() || rf24.testCarrier())
		returnVal = true;

	return returnVal;
}

unsigned long RadioManager::generateUID() {
	unsigned long generatedUID = ((micros()/(unsigned long)100)*(unsigned long)100);

	if(singleMan->addrMan()->hasAddress()) {
			generatedUID += singleMan->addrMan()->getAddress();
	} else {
			generatedUID += random(50,100);
	}

	return(generatedUID);
}

void RadioManager::setMillisOffset(long newOffset) {
	currentMillisOffset = newOffset;

	if (singleMan->outputMan()->isLogLevelEnabled(LOG_INFO)) {
		singleMan->outputMan()->print(LOG_INFO, F("RadioManager::setMillisOffset:: "));
		singleMan->outputMan()->print(LOG_INFO, currentMillisOffset);
		singleMan->outputMan()->print(LOG_INFO, F("      CurrentTime: "));
		singleMan->outputMan()->print(LOG_INFO, millis());
		singleMan->outputMan()->print(LOG_INFO, F("      AdjustedTime: "));
		singleMan->outputMan()->print(LOG_INFO, getAdjustedMillis());
		singleMan->outputMan()->println(LOG_INFO, F(""));
	}
}

unsigned long RadioManager::getAdjustedMillis() {
	return millis() + currentMillisOffset;
}

// polls for available data, and if found pushes to the queue
bool RadioManager::checkRadioForData() {

	if(rf24.failureDetected) {
		singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Check, resetting"));
		resetRadio();
	} else {
		if(rf24.available()) {

			RF24Message* newMessage = new RF24Message();
			rf24.read(newMessage, sizeof(RF24Message));

			// If the payload is a NTP_CLIENT_REQUEST then immediately
			//	update server_start time
			if(newMessage->messageType == NTP_CLIENT_REQUEST)
				newMessage->param7_server_start = millis();
			else if(newMessage->messageType == NTP_SERVER_RESPONSE)
				newMessage->param4_client_end = millis();

			if(newMessage->sentryTargetID == singleMan->addrMan()->getAddress()
				|| newMessage->sentryTargetID == 255 || newMessage->sentryTargetID == 0)
			{
				singleMan->outputMan()->print(LOG_RADIO, F("Radio Received "));
				this->printlnMessage(LOG_RADIO, *newMessage);
			}

			if(pushMessage(newMessage) == false)
				delete newMessage;
		}
	}

	return (this->messageQueue == NULL);
}

void RadioManager::printlnMessage(OUTPUT_LOG_TYPES log_level, RF24Message message) {
	if (singleMan->outputMan()->isLogLevelEnabled(log_level)) {
		singleMan->outputMan()->print(log_level, F(":::  Message> "));

		if (singleMan->outputMan()->isLogLevelEnabled(LOG_DEBUG)) {
			switch (message.messageType) {
				case NEW_ADDRESS_REQUEST:        singleMan->outputMan()->print(LOG_DEBUG, F("NEW_ADDRESS_REQUEST  "));  break;
				case NEW_ADDRESS_RESPONSE:       singleMan->outputMan()->print(LOG_DEBUG, F("NEW_ADDRESS_RESPONSE "));  break;
				case NTP_COORD_MESSAGE:          singleMan->outputMan()->print(LOG_DEBUG, F("NTP_COORD_MESSAGE    "));  break;
				case NTP_CLIENT_REQUEST:         singleMan->outputMan()->print(LOG_DEBUG, F("NTP_CLIENT_REQUEST   "));  break;
				case NTP_SERVER_RESPONSE:        singleMan->outputMan()->print(LOG_DEBUG, F("NTP_SERVER_RESPONSE  "));  break;
				case NTP_CLIENT_FINISHED:        singleMan->outputMan()->print(LOG_DEBUG, F("NTP_CLIENT_FINISHED  "));  break;
				case COLOR_MESSAGE_TO_SENTRY:    singleMan->outputMan()->print(LOG_DEBUG, F("COLOR_MSG_TO_SENTRY  "));  break;
				case COLOR_MESSAGE_FROM_SENTRY:  singleMan->outputMan()->print(LOG_DEBUG, F("COLOR_MSG_FROM_SENTRY"));  break;
			}

			singleMan->outputMan()->print(LOG_DEBUG, F("  UID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.UID);
			singleMan->outputMan()->print(LOG_DEBUG, F("  sentrySrcID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.sentrySrcID);
			singleMan->outputMan()->print(LOG_DEBUG, F("  sentryTargetID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.sentryTargetID);
			singleMan->outputMan()->print(LOG_DEBUG, F("  param1> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param1_byte);
			singleMan->outputMan()->print(LOG_DEBUG, F("  param5> "));
			singleMan->outputMan()->println(LOG_DEBUG, message.param5_client_start);
		} else {
			singleMan->outputMan()->println(log_level, message.messageType);
		}
	}
}


RF24Message* RadioManager::peekMessage() {
	RF24Message* returnMessage = NULL;

	MessageNode* messageTail = this->messageQueue;
	while(messageTail != NULL) {
		returnMessage = messageTail->message;
		messageTail = messageTail->next;
	}

	return(returnMessage);
}


RF24Message* RadioManager::popMessage() {
	RF24Message* returnMessage = NULL;
	if(this->messageQueue != NULL)
	{
		returnMessage = this->messageQueue->message;
		MessageNode *newHead = this->messageQueue->next;
		delete this->messageQueue;
		this->messageQueue = newHead;
	}

	return(returnMessage);
}

bool RadioManager::pushMessage(RF24Message *newMessage) {

	// singleMan->outputMan()->println(LOG_DEBUG, F("pushMessage--sentUIDs[]="));
	// for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
	// 	if(i!=0) 	singleMan->outputMan()->print(LOG_DEBUG, F(","));
	// 	singleMan->outputMan()->print(LOG_DEBUG, this->sentUIDs[i]);
	// }
	// singleMan->outputMan()->print(LOG_DEBUG, F(""));

	// If we've already sent or received this message let it die here
	for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(this->sentUIDs[i] == newMessage->UID) {
			singleMan->outputMan()->print(LOG_DEBUG, F("NOT PUSHED, SENT BY ME: "));
			singleMan->outputMan()->println(LOG_DEBUG, this->sentUIDs[i]);
			return false;
		}
		if(this->receivedUIDs[i] == newMessage->UID) {
			singleMan->outputMan()->print(LOG_DEBUG, F("NOT PUSHED, ALREADY PROCESSED: "));
			singleMan->outputMan()->println(LOG_DEBUG, this->receivedUIDs[i]);
			return false;
		}
	}

	if(this->messageQueue == NULL)
	{
		this->messageQueue = new MessageNode();
		this->messageQueue->next = NULL;
		this->messageQueue->message = newMessage;
	}
	else
	{
		MessageNode *lastNode = this->messageQueue;

		// find last location to insert message
		while(lastNode->next != NULL)
			lastNode = lastNode->next;

		MessageNode *newNode = new MessageNode();
		lastNode->next = newNode;
		newNode->message = newMessage;
	}

	// Store this as received
	this->receivedUIDs[nextReceivedUIDIndex++] = newMessage->UID;
	if(nextReceivedUIDIndex == MAX_STORED_MSG_IDS)
		nextReceivedUIDIndex = 0;

	return true;
}


void RadioManager::internalSendMessage(RF24Message messageToSend) {

	// Mark as sent
	bool alreadySent = false;
	for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(this->sentUIDs[i] == messageToSend.UID) {
			alreadySent = true;
			break;
		}
	}

	// Store this as sent
	if(alreadySent == false) {

		// Check for hardware failure, reset radio - then send message
		if(rf24.failureDetected) {
			singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Send, resetting"));
			resetRadio();
		}

		byte transmitChannel = (messageToSend.sentrySrcID < messageToSend.sentryTargetID) ? 1 : 2;

		delay(singleMan->addrMan()->getAddress()*RADIO_SEND_DELAY);

		// reset UID array pointer
		this->sentUIDs[nextSentUIDIndex++] = messageToSend.UID;
		if(nextSentUIDIndex == MAX_STORED_MSG_IDS)
			nextSentUIDIndex = 0;

		byte currentZone = singleMan->addrMan()->getZone();

		singleMan->outputMan()->print(LOG_RADIO, F("Send Message   "));
		this->printlnMessage(LOG_RADIO, messageToSend);

		rf24.stopListening();

		rf24.closeReadingPipe(transmitChannel);
		rf24.openWritingPipe(this->pipeAddresses[currentZone][transmitChannel]);

		if(!rf24.write(&messageToSend, sizeof(RF24Message))) {
			singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Write"));
		}
		rf24.flush_tx();

		rf24.openWritingPipe(this->pipeAddresses[currentZone][0]);
		rf24.openReadingPipe(transmitChannel, this->pipeAddresses[currentZone][transmitChannel]);

		rf24.startListening();
	}
}

void RadioManager::sendMessage(RF24Message messageToSend) {

		// Sending messages requires a new UID; but don't want
		//	to change UID when echoing!
		messageToSend.UID = generateUID();

		internalSendMessage(messageToSend);
}

void RadioManager::echoMessage(RF24Message messageToEcho) {

	byte address = singleMan->addrMan()->getAddress();

	// Only echo the message if current sentry isn't 'upstream'
	if((messageToEcho.sentrySrcID < messageToEcho.sentryTargetID && address < messageToEcho.sentryTargetID) ||
		(messageToEcho.sentryTargetID < messageToEcho.sentrySrcID && address < messageToEcho.sentrySrcID) ||
		(messageToEcho.sentryTargetID == 255) ||
		(messageToEcho.sentrySrcID == 255))
	{
			// Only care about checking for resending as we *do* want to echo received messages
			for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
				if(this->sentUIDs[i] == messageToEcho.UID) {
					return;
				}
			}

			internalSendMessage(messageToEcho);
		}
}

NTP_state RadioManager::sendNTPRequestToServer() {

	RF24Message ntpOut;
	ntpOut.messageType = NTP_CLIENT_REQUEST;
	ntpOut.sentrySrcID = singleMan->addrMan()->getAddress();
	ntpOut.sentryTargetID = 0;
	ntpOut.param4_client_end = 0;
	ntpOut.param5_client_start = millis();
	ntpOut.param6_server_end = 0;
	ntpOut.param7_server_start = 0;

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

			for(byte i=1; i<NTP_OFFSET_SUCCESSES_REQUIRED; ++i)
			{
					for(byte j=0;j<(NTP_OFFSET_SUCCESSES_REQUIRED-i);++j)
							if(offsetCollection[j] > offsetCollection[j+1])
							{
									long temp = offsetCollection[j];
									offsetCollection[j] = offsetCollection[j+1];
									offsetCollection[j+1] = temp;
							}
			}

			int excludeCount = (NTP_OFFSET_SUCCESSES_REQUIRED - NTP_OFFSET_SUCCESSES_USED) / 2;

			long long summedOffset = 0;
			for(byte i=excludeCount; i<(excludeCount+NTP_OFFSET_SUCCESSES_USED); i++)
				summedOffset += offsetCollection[i];

			long averagedOffset = summedOffset/(long)NTP_OFFSET_SUCCESSES_USED;
			this->setMillisOffset(averagedOffset);

			// Tell the server that syncronization has happened.
			RF24Message ntpClientFinished;
			ntpClientFinished.messageType = NTP_CLIENT_FINISHED;
			ntpClientFinished.sentrySrcID = singleMan->addrMan()->getAddress();
			ntpClientFinished.sentryTargetID = 0;
			ntpClientFinished.param5_client_start = averagedOffset;
			sendMessage(ntpClientFinished);

			// reset variables to wait for next NTP sync
			currOffsetIndex = 0;
			returnState = NTP_DONE;
		}
	}

	return(returnState);
}

long RadioManager::calculateOffsetFromNTPResponseFromServer(RF24Message *ntpMessage) {

	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
		singleMan->outputMan()->println(LOG_TIMING, F("*** calculateOffsetFromNTPResponseFromServer ***"));

		singleMan->outputMan()->print(LOG_TIMING, F("calculateOffset --> VagueTxRxTime: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param4_client_end - ntpMessage->param5_client_start);
		singleMan->outputMan()->print(LOG_TIMING, F("    client_start: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param5_client_start);
		singleMan->outputMan()->print(LOG_TIMING, F("    client_end: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param4_client_end);
		singleMan->outputMan()->print(LOG_TIMING, F("    server_start: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param7_server_start);
		singleMan->outputMan()->print(LOG_TIMING, F("    server_end: "));
		singleMan->outputMan()->println(LOG_TIMING, ntpMessage->param6_server_end);
	}

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
	long t1_t0 = ntpMessage->param7_server_start - ntpMessage->param5_client_start;
	long t2_t3 = ntpMessage->param6_server_end - ntpMessage->param4_client_end;
	long long offset_LL = (t1_t0 + t2_t3);
	long offset = (long) (offset_LL / 2);

	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
			singleMan->outputMan()->print(LOG_TIMING, F("t1_t0: "));
		singleMan->outputMan()->print(LOG_TIMING, t1_t0);
		singleMan->outputMan()->print(LOG_TIMING, F("    t2_t3: "));
		singleMan->outputMan()->print(LOG_TIMING, t2_t3);
		singleMan->outputMan()->print(LOG_TIMING, F("    offset_LL: "));
		singleMan->outputMan()->print(LOG_TIMING, t1_t0 + t2_t3);
		singleMan->outputMan()->print(LOG_TIMING, F("    offset: "));
		singleMan->outputMan()->println(LOG_TIMING, offset);
	}

	//	long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;
	long t3_t0 = ntpMessage->param4_client_end - ntpMessage->param5_client_start;
	long t2_t1 = ntpMessage->param6_server_end - ntpMessage->param7_server_start;
	long halfRtripDelay = (t3_t0 - t2_t1) / 2;

	// Update offset to use the delay
	offset = offset + halfRtripDelay;

	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
		singleMan->outputMan()->print(LOG_TIMING, F("t3_t0: "));
		singleMan->outputMan()->print(LOG_TIMING, t3_t0);
		singleMan->outputMan()->print(LOG_TIMING, F("    t2_t1: "));
		singleMan->outputMan()->print(LOG_TIMING, t2_t1);
		singleMan->outputMan()->print(LOG_TIMING, F("    halfRtripDelay: "));
		singleMan->outputMan()->print(LOG_TIMING, halfRtripDelay);
		singleMan->outputMan()->print(LOG_TIMING, F("    offset: "));
		singleMan->outputMan()->println(LOG_TIMING, offset);
	}

	// return
	return(offset);
}


void RadioManager::handleNTPClientRequest(RF24Message* ntpMessage) {

	// Set the payload
	ntpMessage->messageType = NTP_SERVER_RESPONSE;
	ntpMessage->sentryTargetID = ntpMessage->sentrySrcID;
	ntpMessage->sentrySrcID = 0;

	ntpMessage->param6_server_end = millis();
	sendMessage(*ntpMessage);
}
