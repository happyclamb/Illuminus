#include "RadioManager.h"


// http://maniacbug.github.io/RF24/classRF24.html
RadioManager::RadioManager(SingletonManager* _singleMan):
		singleMan(_singleMan),
		rf24(RF24(RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN))
{
	// initialize RF24 radio
	resetRadio();

	// init stacks
	messageReceiveStack = new MessageStack();
	messageSendStack = new MessageStack();

	// Use some vaguely random read to seed the lanterns
	randomSeed(analogRead(INPUT_A3_PIN));

	// init the sentUIDs array
	for(byte i=0; i<MAX_STORED_MSG_IDS; i++)
	{
		this->sentUIDs[i] = 65535;
		this->receivedUIDs[i] = 65535;
	}

	singleMan->setRadioMan(this);
}

byte RadioManager::receiveStackSize()    { return messageReceiveStack->length(); }
byte RadioManager::sendStackSize()   { return messageSendStack->length(); }

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
		rf24.txDelay = 1;

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

		// void 	setAddressWidth (uint8_t a_width)
		// rf24.setAddressWidth(3);

		// DisableCRC since not using autoACK? NO - still need it
		//void 	setCRCLength (rf24_crclength_e length)
		//rf24_crclength_e 	getCRCLength (void)
		// rf24.disableCRC();

		// Long term change to interrups?
		// http://forcetronic.blogspot.ca/2016/07/using-nrf24l01s-irq-pin-to-generate.html
		// void 	maskIRQ (bool tx_ok, bool tx_fail, bool rx_ready)

		// Not using
		// void 	enableDynamicPayloads (void)
		// void 	enableDynamicAck ()
		// void 	enableAckPayload (void)

		// RF Channel to be transmitting on 0->127; default 76
		byte currentZone = singleMan->addrMan()->getZone();
		rf24.setChannel(1 + currentZone*40);

		for (byte i=0; i<6; i++) rf24.closeReadingPipe(i);
		rf24.openReadingPipe(0, this->pipeAddress);

		// kick off with listening
		rf24.startListening();

		// https://forum.arduino.cc/index.php?topic=215065.0
		// http://forum.arduino.cc/index.php?topic=216306.0

		// using channels for message types
		// https://forum.arduino.cc/index.php?topic=429763.0
	}
}


bool RadioManager::checkForInterference() {
	bool returnVal = false;

	if(rf24.testRPD() || rf24.testCarrier())
		returnVal = true;

	return returnVal;
}


unsigned int RadioManager::generateUID() {
	unsigned int generatedUID;

	if(singleMan->addrMan()->hasAddress()) {
		generatedUID = (unsigned int)(((unsigned int)micros()/(unsigned int)100)*(unsigned int)100);
		generatedUID += singleMan->addrMan()->getAddress();
	} else {
		generatedUID = random(65500);
	}

	return(generatedUID);
}


// polls for available data, and if found pushes to the queue
void RadioManager::checkRadioForData() {
	if(rf24.failureDetected) {
		singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Check, resetting"));
		resetRadio();
	} else {
		while(rf24.available()) {

			// read takes ~ 0.15ms (150 microseconds)
			RF24Message* newMessage = new RF24Message();
			rf24.read(newMessage, sizeof(RF24Message));

			// If the payload is part of the NTP chain then immediately update time param times
			if(newMessage->messageType == NTP_CLIENT_REQUEST &&
				singleMan->addrMan()->getAddress() == singleMan->healthMan()->getServerAddress())
			{
				newMessage->param7_server_start = millis();
			}
			else if(newMessage->messageType == NTP_SERVER_RESPONSE &&
				newMessage->sentryTargetID == singleMan->addrMan()->getAddress())
			{
				newMessage->param4_client_end = millis();
			}

			// Queue the received message
			this->queueReceivedMessage(newMessage);
			newMessage = NULL;
		}
	}
}


void RadioManager::queueReceivedMessage(RF24Message *newMessage) {

	// If we've already sent or received this message let it die here
	for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
		if(this->sentUIDs[i] == newMessage->UID) {
			delete newMessage;
			return;
		}
		if(this->receivedUIDs[i] == newMessage->UID) {
			delete newMessage;
			return;
		}
	}

	if (singleMan->outputMan()->isLogLevelEnabled(LOG_RADIO)) {
		if(newMessage->sentryTargetID == singleMan->addrMan()->getAddress()
			|| newMessage->sentryTargetID == 255)
		{
			singleMan->outputMan()->print(LOG_RADIO, millis());
			singleMan->outputMan()->print(LOG_RADIO, F("  Radio Receive "));
			this->printlnMessage(LOG_RADIO, *newMessage);
		}
	}

	this->messageReceiveStack->push(newMessage);

	// Store this as received
	this->receivedUIDs[nextReceivedUIDIndex++] = newMessage->UID;
	if(nextReceivedUIDIndex == MAX_STORED_MSG_IDS)
		nextReceivedUIDIndex = 0;
}

void RadioManager::printlnMessage(OUTPUT_LOG_TYPES log_level, RF24Message message) {
	if (singleMan->outputMan()->isLogLevelEnabled(log_level)) {
		singleMan->outputMan()->print(log_level, F(":::  Message> "));

		if (singleMan->outputMan()->isLogLevelEnabled(LOG_DEBUG)) {
			switch (message.messageType) {
				case NEW_ADDRESS_REQUEST:      singleMan->outputMan()->print(LOG_DEBUG, F("NEW_ADDRESS_REQUEST "));  break;
				case NEW_ADDRESS_RESPONSE:     singleMan->outputMan()->print(LOG_DEBUG, F("NEW_ADDRESS_RESPONSE"));  break;
				case NTP_COORD_MESSAGE:        singleMan->outputMan()->print(LOG_DEBUG, F("NTP_COORD_MESSAGE   "));  break;
				case NTP_RESET_MESSAGE:        singleMan->outputMan()->print(LOG_DEBUG, F("NTP_RESET_MESSAGE   "));  break;
				case NTP_CLIENT_REQUEST:       singleMan->outputMan()->print(LOG_DEBUG, F("NTP_CLIENT_REQUEST  "));  break;
				case NTP_SERVER_RESPONSE:      singleMan->outputMan()->print(LOG_DEBUG, F("NTP_SERVER_RESPONSE "));  break;
				case NTP_CLIENT_FINISHED:      singleMan->outputMan()->print(LOG_DEBUG, F("NTP_CLIENT_FINISHED "));  break;
				case COLOR_MESSAGE_TO_SENTRY:  singleMan->outputMan()->print(LOG_DEBUG, F("COLOR_MSG_TO_SENTRY "));  break;
				case KEEP_ALIVE_FROM_SENTRY:   singleMan->outputMan()->print(LOG_DEBUG, F("SENTRY_KEEP_ALIVE   "));  break;
			}

			singleMan->outputMan()->print(LOG_DEBUG, F(" UID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.UID);
			singleMan->outputMan()->print(LOG_DEBUG, F(" sentrySrcID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.sentrySrcID);
			singleMan->outputMan()->print(LOG_DEBUG, F(" sentryTargetID> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.sentryTargetID);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p1> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param1_byte);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p2> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param2_byte);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p3> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param3_byte);

			singleMan->outputMan()->print(LOG_DEBUG, F(" p4> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param4_client_end);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p5> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param5_client_start);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p6> "));
			singleMan->outputMan()->print(LOG_DEBUG, message.param6_server_end);
			singleMan->outputMan()->print(LOG_DEBUG, F(" p7> "));
			singleMan->outputMan()->println(LOG_DEBUG, message.param7_server_start);
		} else {
			singleMan->outputMan()->println(log_level, message.messageType);
		}
	}
}


RF24Message* RadioManager::peekMessage(Radio_Message_Type type) {
	return(this->messageReceiveStack->peekMessageType(type));
}


RF24Message* RadioManager::popMessageReceive() {
	return(this->messageReceiveStack->shift());
}


void RadioManager::transmitStack(bool limited_transmit) {

	if(messageSendStack->isEmpty() == false) {

		// Check for hardware failure, reset radio - then send message
		if(rf24.failureDetected) {
			singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Send, resetting"));
			resetRadio();
		}

		// sending time is about 1.5 ms (1500 microseconds)
		rf24.stopListening();
		rf24.closeReadingPipe(0);
		rf24.openWritingPipe(this->pipeAddress);

		RF24Message *messageToSend = messageSendStack->shift();
		byte address = singleMan->addrMan()->getAddress();
		byte messagesSent = 0;
		while (messageToSend != NULL) {

			bool doSend = true;
			// Abort send if already sent
			for(byte i=0; i<MAX_STORED_MSG_IDS; i++) {
				if(this->sentUIDs[i] == messageToSend->UID) {
					doSend = false;
					break;
				}
			}

			if(doSend) {
				// reset UID array pointer
				this->sentUIDs[nextSentUIDIndex++] = messageToSend->UID;
				if(nextSentUIDIndex == MAX_STORED_MSG_IDS)
					nextSentUIDIndex = 0;

				// If in limited transmit mode only allow NEW_ADDRESS_REQUEST requests
				//	or ones sent by this lantern.
				//	Still delete other messages to prevent stack from overflowing
				if(limited_transmit == false || limited_transmit && (
						messageToSend->messageType == NEW_ADDRESS_REQUEST ||
						messageToSend->sentrySrcID == address
					))
				{
					singleMan->outputMan()->print(LOG_RADIO, millis());
					singleMan->outputMan()->print(LOG_RADIO, F("  Radio Send    "));
					this->printlnMessage(LOG_RADIO, messageToSend);

					// If the payload is part of the NTP chain then immediately update time param times
					if(messageToSend->messageType == NTP_CLIENT_REQUEST &&
						messageToSend->sentrySrcID == address)
					{
						messageToSend->param5_client_start = millis();
					}
					else if(messageToSend->messageType == NTP_SERVER_RESPONSE &&
						singleMan->addrMan()->getAddress() == singleMan->healthMan()->getServerAddress())
					{
						messageToSend->param6_server_end = millis();
					}

					// Write the message
					if(!rf24.write(messageToSend, sizeof(RF24Message))) {
						singleMan->outputMan()->println(LOG_ERROR, F("RADIO ERROR On Write"));
					}
					messagesSent++;
				}
			}

			delete messageToSend;
			messageToSend = NULL;

			// Radio buffer size is 3*32bytes; so pause every third messageToEcho
			//	to allow for the buffer to be cleared and other radios to read their
			//	radio buffers to minimize lost messages
			if(messagesSent < 3) {
				messageToSend = messageSendStack->shift();
			}
		}

		rf24.flush_tx();
		rf24.openReadingPipe(0, this->pipeAddress);
		rf24.startListening();
	}
}

void RadioManager::sendMessage(RF24Message* messageToSend) {

		// Sending messages requires a new UID; but don't want
		//	to change UID when echoing!
		messageToSend->UID = generateUID();

		queueSendMessage(messageToSend);
}

void RadioManager::echoMessage(RF24Message* messageToEcho) {
	queueSendMessage(messageToEcho);
}

void RadioManager::queueSendMessage(RF24Message* messageToQueue) {
	byte address = singleMan->addrMan()->getAddress();

	if(messageToQueue->sentryTargetID == 255 ||
		(messageToQueue->sentrySrcID < messageToQueue->sentryTargetID && address < messageToQueue->sentryTargetID) ||
		(messageToQueue->sentrySrcID > messageToQueue->sentryTargetID && address > messageToQueue->sentryTargetID)
	) {
		this->messageSendStack->push(messageToQueue);
	} else {
		delete messageToQueue;
	}
}

void RadioManager::checkSendWindow() {

	bool transmit = false;
	bool limited_transmit = false;
	if(singleMan->addrMan()->hasAddress() == false) {
		// no address; so broadcast on channel 4 as it's not part of the windowing scheme
		limited_transmit = true;
	}
	else if(singleMan->radioMan()->getMillisOffset() == 0
		&& singleMan->healthMan()->getServerAddress() != singleMan->addrMan()->getAddress())
	{
		// not server and NTP hasn't finished yet; so just broadcast
		limited_transmit = true;
	}
	else {

		byte totalSentries = singleMan->healthMan()->totalSentries();
		transmit = totalSentries == 1 ? true : false;

		if (totalSentries > 1) {
			unsigned long currTime = singleMan->radioMan()->getAdjustedMillis();
			byte transmitStep = (currTime%(2*totalSentries*TRANSMISSION_WINDOW_SIZE))/TRANSMISSION_WINDOW_SIZE;

			// Intentially give a double window at start and end of the line
			byte transmitAddress = transmitStep;
			if(transmitStep >= totalSentries) {
				transmitAddress = totalSentries - (transmitAddress - totalSentries);
			}

			transmit = (transmitAddress == singleMan->addrMan()->getAddress());
		}
	}

	// Handle transmission
	if(transmit || limited_transmit) {

		// since this will be fired immediately after receiving a message there
		//	is a great chance that responding immediately is going to land in another
		//	window, decrease chances of this by broadcasing in the later half of a
		//	transmission window.
		if(limited_transmit && this->messageSendStack->isEmpty() == false) {
			delay(TRANSMISSION_WINDOW_SIZE/2);
		}

		transmitStack(limited_transmit);
	}
}


NTP_state RadioManager::sendNTPRequestToServer() {

	RF24Message *ntpRequest = new RF24Message();
	ntpRequest->messageType = NTP_CLIENT_REQUEST;
	ntpRequest->sentrySrcID = singleMan->addrMan()->getAddress();
	ntpRequest->sentryTargetID = 0;
	ntpRequest->param4_client_end = 0;
	ntpRequest->param5_client_start = millis();
	ntpRequest->param6_server_end = 0;
	ntpRequest->param7_server_start = 0;

	sendMessage(ntpRequest);

	return(NTP_WAITING_FOR_RESPONSE);
}

NTP_state RadioManager::handleNTPServerOffset(long serverNTPOffset) {
	static long offsetCollection[NTP_OFFSET_SUCCESSES_REQUIRED];
	static byte currOffsetIndex = 0;
	static byte fails = 0;

	NTP_state returnState = NTP_SEND_REQUEST;

	// Timeout is marked by passing 0 to this function
	if(serverNTPOffset == 0) {
		fails++;

		// If there are 3 fails; then reset internal variables and
		//	stop the round of NTP. Wait until next NTP_COORD_MESSAGE
		//	as likely	the server has died.
		if(fails == 3) {
			fails = 0;
			currOffsetIndex = 0;
			returnState = NTP_DONE;
		}
	} else {
		offsetCollection[currOffsetIndex] = serverNTPOffset;
		currOffsetIndex++;
	}

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
		RF24Message* ntpClientFinished = new RF24Message();
		ntpClientFinished->messageType = NTP_CLIENT_FINISHED;
		ntpClientFinished->sentrySrcID = singleMan->addrMan()->getAddress();
		ntpClientFinished->sentryTargetID = 0;
		ntpClientFinished->param5_client_start = averagedOffset;
		sendMessage(ntpClientFinished);

		// reset variables to wait for next NTP sync
		currOffsetIndex = 0;
		returnState = NTP_DONE;
	}

	return(returnState);
}

long RadioManager::calculateOffsetFromNTPResponseFromServer(RF24Message *ntpMessage) {

#ifdef LOG_TIMING_DEFINED
	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
		singleMan->outputMan()->println(LOG_TIMING, F("*** calculateOffsetFromNTPResponseFromServer"));

		singleMan->outputMan()->print(LOG_TIMING, F("calcOffset --> VagueTxRxTime: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param4_client_end - ntpMessage->param5_client_start);
		singleMan->outputMan()->print(LOG_TIMING, F(" client_start: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param5_client_start);
		singleMan->outputMan()->print(LOG_TIMING, F(" client_end: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param4_client_end);
		singleMan->outputMan()->print(LOG_TIMING, F(" server_start: "));
		singleMan->outputMan()->print(LOG_TIMING, ntpMessage->param7_server_start);
		singleMan->outputMan()->print(LOG_TIMING, F(" server_end: "));
		singleMan->outputMan()->println(LOG_TIMING, ntpMessage->param6_server_end);
	}
#endif

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

#ifdef LOG_TIMING_DEFINED
	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
			singleMan->outputMan()->print(LOG_TIMING, F("t1_t0: "));
		singleMan->outputMan()->print(LOG_TIMING, t1_t0);
		singleMan->outputMan()->print(LOG_TIMING, F(" t2_t3: "));
		singleMan->outputMan()->print(LOG_TIMING, t2_t3);
		singleMan->outputMan()->print(LOG_TIMING, F(" offset_LL: "));
		singleMan->outputMan()->print(LOG_TIMING, t1_t0 + t2_t3);
		singleMan->outputMan()->print(LOG_TIMING, F(" offset: "));
		singleMan->outputMan()->println(LOG_TIMING, offset);
	}
#endif

	//	long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;
	long t3_t0 = ntpMessage->param4_client_end - ntpMessage->param5_client_start;
	long t2_t1 = ntpMessage->param6_server_end - ntpMessage->param7_server_start;
	long halfRtripDelay = (t3_t0 - t2_t1) / 2;

	// Update offset to use the delay
	offset = offset + halfRtripDelay;

#ifdef LOG_TIMING_DEFINED
	if(singleMan->outputMan()->isLogLevelEnabled(LOG_TIMING)) {
		singleMan->outputMan()->print(LOG_TIMING, F("t3_t0: "));
		singleMan->outputMan()->print(LOG_TIMING, t3_t0);
		singleMan->outputMan()->print(LOG_TIMING, F(" t2_t1: "));
		singleMan->outputMan()->print(LOG_TIMING, t2_t1);
		singleMan->outputMan()->print(LOG_TIMING, F(" halfRtripDelay: "));
		singleMan->outputMan()->print(LOG_TIMING, halfRtripDelay);
		singleMan->outputMan()->print(LOG_TIMING, F(" offset: "));
		singleMan->outputMan()->println(LOG_TIMING, offset);
	}
#endif

	// return
	return(offset);
}


void RadioManager::handleNTPClientRequest(RF24Message* ntpRequest) {

	// Set the payload
	RF24Message* ntpResponse = new RF24Message(ntpRequest);
	ntpResponse->messageType = NTP_SERVER_RESPONSE;
	ntpResponse->sentryTargetID = ntpRequest->sentrySrcID;
	ntpResponse->sentrySrcID = singleMan->addrMan()->getAddress();
	ntpResponse->param6_server_end = millis();

	sendMessage(ntpResponse);
}
