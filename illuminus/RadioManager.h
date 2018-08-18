#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

#define MAX_STORED_MSG_IDS 50
#define NTP_OFFSET_SUCCESSES_REQUIRED 7
#define NTP_OFFSET_SUCCESSES_USED 3

class RF24Message
{
	public:
		byte messageType = 0; // 1 byte
		unsigned long UID = 0; // 4 bytes

		byte sentrySrcID = 0; // 1 byte
		byte sentryTargetID = 0; // 1 byte

		byte byteParam1 = 0; // 1 byte
		byte byteParam2 = 0; // 1 byte
		byte byteParam3 = 0; // 1 byte

		unsigned long client_start = 0; // 4 bytes
		unsigned long client_end = 0; // 4 bytes
		unsigned long server_start = 0; // 4 bytes
		unsigned long server_end = 0; // 4 bytes
};

enum Radio_Message_Type {
			NEW_ADDRESS_REQUEST,
			NEW_ADDRESS_RESPONSE,
			NTP_COORD_MESSAGE,
			NTP_CLIENT_REQUEST,
			NTP_SERVER_RESPONSE,
			NTP_CLIENT_FINISHED,
			COLOR_MESSAGE_TO_SENTRY,
			COLOR_MESSAGE_FROM_SENTRY };

class MessageNode {
	public:
		RF24Message *message = NULL;
		MessageNode *next = NULL;
};

enum NTP_state {
			NTP_DONE,
			NTP_WAITING_FOR_RESPONSE,
			NTP_SEND_REQUEST };

class RadioManager
{
	public:
		RadioManager(SingletonManager* _singleMan, uint8_t radio_ce_pin, uint8_t radio__cs_pin);

		unsigned long generateUID();

		unsigned long getIntervalBetweenPatternUpdates(){ return TIME_BETWEEN_LED_MSGS; }
		void setIntervalBetweenPatternUpdates(unsigned long newInterval){ TIME_BETWEEN_LED_MSGS = newInterval; }
		unsigned long getIntervalBetweenNTPChecks(){ return TIME_BETWEEN_NTP_MSGS; }
		unsigned long ntpRequestTimeout(){ return NTP_TIMEOUT; }

		unsigned long getAdjustedMillis();
		void setMillisOffset(long newOffset);

		bool checkRadioForData();
		RF24Message* popMessage();
		bool checkForInterference();

		void sendMessage(RF24Message messageToSend);
		void echoMessage(RF24Message messageToEcho);

		bool setInformServerWhenNTPDone(bool newValue);
		NTP_state sendNTPRequestToServer();
		NTP_state handleNTPServerResponse(RF24Message* ntpMessage);
		void handleNTPClientRequest(RF24Message* ntpMessage);

	private:
		SingletonManager* singleMan;
		RF24 rf24;
		long currentMillisOffset = 0;
		uint64_t pipeAddresses[4][2];

		MessageNode* messageQueue = NULL;
		unsigned long sentUIDs[MAX_STORED_MSG_IDS];
		int nextSentUIDIndex = 0;
		unsigned long receivedUIDs[MAX_STORED_MSG_IDS];
		int nextReceivedUIDIndex = 0;
		int analogSeed = 0;
		bool informServerWhenNTPDone = true;

		unsigned long TIME_BETWEEN_NTP_MSGS = 10000;
		unsigned long TIME_BETWEEN_LED_MSGS = 1000;
		unsigned long NTP_TIMEOUT = 2000;

		void resetRadio();
		void internalSendMessage(RF24Message messageToSend);
		bool pushMessage(RF24Message* newMessage);

		long calculateOffsetFromNTPResponseFromServer(RF24Message* ntpMessage);
};

#endif // __RADIOMANAGER_H__
