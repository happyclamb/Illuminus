#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

#include "IlluminusDefs.h"
#include "Message.h"
class RF24Message;
class MessageStack;

#include "SingletonManager.h"
class SingletonManager;

#define MAX_STORED_MSG_IDS 50
#define NTP_OFFSET_SUCCESSES_REQUIRED 5
#define NTP_OFFSET_SUCCESSES_USED 3

class RadioManager
{
	public:
		RadioManager(SingletonManager* _singleMan);

		unsigned int generateUID();

		unsigned long getIntervalBroadcastMessages(){ return this->INTERVAL_BETWEEN_MSGS; }
		void setIntervalBroadcastMessages(unsigned long newInterval){ this->INTERVAL_BETWEEN_MSGS = newInterval; }

		unsigned long ntpRequestTimeout(){ return this->NTP_REQUEST_TIMEOUT; }

		unsigned long getAdjustedMillis() { return millis() + currentMillisOffset; }
		void setMillisOffset(long newOffset) { currentMillisOffset = newOffset; }
		long getMillisOffset() { return currentMillisOffset; }

		void checkRadioForData();
		void checkSendWindow();

		RF24Message* peekMessage(Radio_Message_Type type);
		RF24Message* popMessageReceive();
		bool checkForInterference();

		void sendMessage(RF24Message* messageToSend);
		void echoMessage(RF24Message* messageToEcho);
		void printlnMessage(OUTPUT_LOG_TYPES log_level, RF24Message message);

		NTP_state sendNTPRequestToServer();
		NTP_state handleNTPServerResponse(RF24Message* ntpMessage);
		void handleNTPClientRequest(RF24Message* ntpRequest);

	private:
		SingletonManager* singleMan;
		RF24 rf24;
		long currentMillisOffset = 0;
		uint64_t pipeAddress = 0xABCDABCD78LL;

		MessageStack* messageReceiveStack = NULL;
		MessageStack* messageUpstreamStack = NULL;
		MessageStack* messageDownstreamStack = NULL;

		unsigned int sentUIDs[MAX_STORED_MSG_IDS];
		byte nextSentUIDIndex = 0;
		unsigned int receivedUIDs[MAX_STORED_MSG_IDS];
		byte nextReceivedUIDIndex = 0;

		unsigned long INTERVAL_BETWEEN_MSGS = 5000;
		unsigned long NTP_REQUEST_TIMEOUT = 2500;
		byte TRANSMISSION_WINDOW_SIZE = 25;

		void resetRadio();
		void transmitStack(MessageStack* messageStack);
		void queueSendMessage(RF24Message* messageToQueue);
		void queueReceivedMessage(RF24Message* newMessage);

		long calculateOffsetFromNTPResponseFromServer(RF24Message* ntpMessage);
};

#endif // __RADIOMANAGER_H__
