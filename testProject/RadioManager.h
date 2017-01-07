#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

#define MAX_STORED_MSG_IDS 64

class RF24Message
{
	public:
		byte messageType = 0; // byte
		unsigned long UID = 0; // 4 bytes

		byte byteParam1 = 0; // byte
		byte byteParam2 = 0; // byte
		byte byteParam3 = 0; // byte
};

enum Radio_Message_Type {
			BLANK_MESSAGE,
		 	TEST_MESSAGE };

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

		bool checkRadioForData();
		RF24Message* popMessage();

		void sendMessage(RF24Message messageToSend);

	private:
		SingletonManager* singleMan;
		RF24 rf24;
		long currentMillisOffset = 0;
		byte radioAddresses[4][2][6];
		MessageNode* messageQueue = NULL;
		unsigned long sentUIDs[MAX_STORED_MSG_IDS];
		int nextSentUIDIndex = 0;
		unsigned long receivedUIDs[MAX_STORED_MSG_IDS];
		int nextReceivedUIDIndex = 0;
		int analogSeed = 0;

		void internalSendMessage(RF24Message messageToSend);
		bool pushMessage(RF24Message* newMessage);
};

#endif // __RADIOMANAGER_H__
