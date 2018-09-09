#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <Arduino.h>
#include "RF24.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class RF24Message
{
	public:
		RF24Message(){}
		RF24Message(RF24Message* baseMessage);

		Radio_Message_Type messageType = UNDEFINED_MESSAGE; // 1 byte
		unsigned long UID = 0; // 4 bytes

		byte sentrySrcID = 0; // 1 byte
		byte sentryTargetID = 0; // 1 byte

		byte param1_byte = 0; // 1 byte
		byte param2_byte = 0; // 1 byte
		byte param3_byte = 0; // 1 byte

		unsigned long param4_client_end = 0; // 4 bytes
		unsigned long param5_client_start = 0; // 4 bytes
		unsigned long param6_server_end = 0; // 4 bytes
		unsigned long param7_server_start = 0; // 4 bytes
};

class MessageStack
{
	public:
		void push(RF24Message* newItem);  // add to end
		RF24Message* shift(); // pop from start
		RF24Message* peekMessageType(Radio_Message_Type type); // peek at first message of TYPE
		bool isEmpty() { return(this->top == NULL); }
		byte length() { return count; }

	private:
		byte count = 0;
		struct MessageStackNode {
        RF24Message* message = NULL;
        MessageStackNode* next = NULL;
    } *top = NULL;
};

#endif // __MESSAGE_H__
