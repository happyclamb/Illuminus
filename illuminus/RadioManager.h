#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <RF24.h>

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

		unsigned long getIntervalNTPCoordMessages(){ return this->INTERVAL_NTP_COORD_MSGS; }
		unsigned long getIntervalColorMessages(){ return this->INTERVAL_COLOR_MSGS; }
		void setIntervalColorMessages(unsigned long newInterval){ this->INTERVAL_COLOR_MSGS = newInterval; }

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

		void handleNTPClientRequest(RF24Message* ntpMessage);
		NTP_state sendNTPRequestToServer();
		NTP_state handleNTPServerOffset(long serverNTPOffset);
		long calculateOffsetFromNTPResponseFromServer(RF24Message* ntpMessage);

		byte receiveStackSize();
		byte sendStackSize();

	private:
		SingletonManager* singleMan;
		RF24 rf24;
		long currentMillisOffset = 0;
		uint64_t pipeAddress = 0xABCDABCD78LL;

		MessageStack* messageReceiveStack = NULL;
		MessageStack* messageSendStack = NULL;

		unsigned int sentUIDs[MAX_STORED_MSG_IDS];
		byte nextSentUIDIndex = 0;
		unsigned int receivedUIDs[MAX_STORED_MSG_IDS];
		byte nextReceivedUIDIndex = 0;

		unsigned long INTERVAL_COLOR_MSGS = 1511;
		unsigned long INTERVAL_NTP_COORD_MSGS = 7307;
		unsigned long NTP_REQUEST_TIMEOUT = 751;
		byte TRANSMISSION_WINDOW_SIZE = 23;

		void resetRadio();
		void transmitStack(bool limited_transmit);
		void queueSendMessage(RF24Message* messageToQueue);
		void queueReceivedMessage(RF24Message* newMessage);
};

#endif // __RADIOMANAGER_H__
