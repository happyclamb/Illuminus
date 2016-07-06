#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class RF24Message
{
	public:
		byte messageType = 0; // byte
		unsigned long UID = 0; // 4 bytes

		byte sentrySrcID = 0; // byte
		byte sentryTargetID = 0; // byte

		byte byteParam1 = 0; // byte
		byte byteParam2 = 0; // byte
		byte byteParam3 = 0; // byte

		unsigned long client_start = 0; // 4 bytes
		unsigned long client_end = 0; // 4 bytes
		unsigned long server_start = 0; // 4 bytes
		unsigned long server_end = 0; // 4 bytes
};

enum Radio_Message_Type {
			BLANK_MESSAGE,
			NEW_ADDRESS_REQUEST,
			NEW_ADDRESS_RESPONSE,
			NTP_COORD_MESSAGE,
			NTP_CLIENT_REQUEST,
			NTP_SERVER_RESPONSE,
			NTP_CLIENT_FINISHED,
			COLOR_MESSAGE };

class MessageNode {
	public:
		RF24Message *message = NULL;
		MessageNode *next = NULL;
};

class RadioManager
{
	public:
		RadioManager(SingletonManager* _singleMan, uint8_t radio_ce_pin, uint8_t radio__cs_pin);

		unsigned long generateUID();
		unsigned long getAdjustedMillis();

		bool setInformServerWhenNTPDone(bool newValue);
		bool checkRadioForData();
		RF24Message* popMessage();

		void sendMessage(RF24Message messageToSend);
		void echoMessage(RF24Message messageToEcho);
		void sendNTPRequestToServer();
		bool handleNTPServerResponse(RF24Message* ntpMessage);
		void handleNTPClientRequest(RF24Message* ntpMessage);

	private:
		SingletonManager* singleMan;
		RF24 rf24;
		long currentMillisOffset = 0;
		byte radioAddresses[6][6];
		MessageNode* messageQueue = NULL;
		unsigned long sentUIDs[MAX_STORED_MSG_IDS];
		int nextSentUIDIndex = 0;
		unsigned long receivedUIDs[MAX_STORED_MSG_IDS];
		int nextReceivedUIDIndex = 0;
		bool informServerWhenNTPDone = true;
		int analogSeed = 0;

		void internalSendMessage(RF24Message messageToSend);
		void setMillisOffset(long newOffset);
		bool pushMessage(RF24Message* newMessage);
		long calculateOffsetFromNTPResponseFromServer(RF24Message* ntpMessage);
};

#endif // __RADIOMANAGER_H__
