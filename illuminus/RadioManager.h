#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

class RF24Message
{
	public:
		byte messageType; // byte

		byte sentryRequestID; // byte
		unsigned long UID; // 4 bytes

		byte byteParam1; // byte
		byte byteParam2; // byte

		unsigned long client_start; // 4 bytes
		unsigned long client_end; // 4 bytes
		unsigned long server_start; // 4 bytes
		unsigned long server_end; // 4 bytes
};

enum Radio_Message_Type { NTP_COORD_MESSAGE, NTP_CLIENT_REQUEST, NTP_SERVER_RESPONSE, COLOR_MESSAGE };


struct MessageNode {
	RF24Message *message;
	MessageNode *next;
};

#define MAX_STORED_MSG_IDS 150
class RadioManager
{
	public:
		RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin);
		void init();
		unsigned long generateUID();
		unsigned long getAdjustedMillis();

		bool checkRadioForData();
		RF24Message* popMessage();

		void sendMessage(RF24Message messageToSend);
		void echoMessage(RF24Message messageToEcho);
		void sendNTPRequestToServer();
		bool handleNTPServerResponse(RF24Message* ntpMessage);
		void handleNTPClientRequest(RF24Message* ntpMessage);

	private:
		RF24 rf24;
		long currentMillisOffset;
		byte radioAddresses[6][6];
		MessageNode* messageQueue;
		unsigned long sentUIDs[MAX_STORED_MSG_IDS];
		int nextSentUIDIndex;
		unsigned long receivedUIDs[MAX_STORED_MSG_IDS];
		int nextReceivedUIDIndex;

		void setMillisOffset(long newOffset);
		void pushMessage(RF24Message* newMessage);
		long calculateOffsetFromNTPResponseFromServer(RF24Message* ntpMessage);
};

#endif // __RADIOMANAGER_H__
