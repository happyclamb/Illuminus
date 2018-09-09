#include "Message.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"


RF24Message::RF24Message(RF24Message* baseMessage) {
	messageType = baseMessage->messageType;
	UID = baseMessage->UID;
	sentrySrcID = baseMessage->sentrySrcID;
	sentryTargetID = baseMessage->sentryTargetID;
	param1_byte = baseMessage->param1_byte;
	param2_byte = baseMessage->param2_byte;
	param3_byte = baseMessage->param3_byte;
	param4_client_end = baseMessage->param4_client_end;
	param5_client_start = baseMessage->param5_client_start;
	param6_server_end = baseMessage->param6_server_end;
	param7_server_start = baseMessage->param7_server_start;
}

// add to end
void MessageStack::push(RF24Message* newItem) {
	// If the top is empty; create one
	if (this->top == NULL) {
		this->top = new MessageStackNode();
		this->top->next = NULL;
		this->top->message = newItem;
	} else {
		MessageStackNode *temp = this->top;
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = new MessageStackNode();
		temp->next->message = newItem;
	}

	this->count++;
}

// pop from start
RF24Message* MessageStack::shift() {
	RF24Message* returnMessage = NULL;
	if(this->top != NULL) {
		returnMessage = this->top->message;
		MessageStackNode *newTop = this->top->next;
		delete this->top;
		this->top = newTop;
		this->count--;
	}
	return(returnMessage);
}

// peek at first message of TYPE
RF24Message* MessageStack::peekMessageType(Radio_Message_Type type) {
	if(this->top != NULL) {
		MessageStackNode *temp = this->top;
		while(temp != NULL) {
 		 	if (temp->message->messageType == type) {
				return(temp->message);
			}
			temp = temp->next;
		}
	}
	return (NULL);
}
