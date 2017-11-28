#include "AddressManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

AddressManager::AddressManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	singleMan->setAddrMan(this);
}


byte AddressManager::getZone() {
	return this->singleMan->inputMan()->getZoneInput();
}

bool AddressManager::hasAddress() {
	return addressSet;
}

void AddressManager::setAddress(byte newAddress) {
	address = newAddress;
	addressSet = true;

	info_print("Assigned Zone: ");
	info_println(getZone());
	info_print("Assigned Address: ");
	info_println(getAddress());
	delay(5);
}

byte AddressManager::getAddress() {
	return address;
}


void AddressManager::sendAddressRequest() {
	RF24Message addressRequestMessage;
	addressRequestMessage.messageType = NEW_ADDRESS_REQUEST;
	addressRequestMessage.sentrySrcID = 255;
	addressRequestMessage.sentryTargetID = 0;
	singleMan->radioMan()->sendMessage(addressRequestMessage);

	unsigned long requestStart = millis();
	while(hasAddress() == false && millis() < (requestStart + 2000)) {
		delay(5); // Give the radio a moment to wait for a message
		singleMan->radioMan()->checkRadioForData();
		RF24Message *currMessage = singleMan->radioMan()->popMessage();
		while(currMessage != NULL) {
			if(currMessage->messageType == NEW_ADDRESS_RESPONSE) {
				setAddress(currMessage->byteParam1);
			}
			delete currMessage;

			if(hasAddress() == false)
				currMessage = singleMan->radioMan()->popMessage();
			else
				currMessage = NULL;
		}
	}
}

void AddressManager::obtainAddress() {

	for(byte i=0; i<NEW_ADDRESS_RETRIES; i++) {
		info_print("Attempt to get address: ");
		info_println(i);
		delay(5);

		sendAddressRequest();
		if(hasAddress())
			break;

		// Need to wait long enough for messages to get processed
		delay(100);
	}

	// if timed out after NEW_ADDRESS_RETRIES tries getting an address then
	//	there is no one else so become server
	if(hasAddress() == false) {
		setAddress(0);
	}
}

void AddressManager::sendNewAddressResponse() {
	byte targetSentry = singleMan->healthMan()->nextAvailSentryID();

	RF24Message addressResponseMessage;
	addressResponseMessage.messageType = NEW_ADDRESS_RESPONSE;
	addressResponseMessage.sentrySrcID = 0;
	addressResponseMessage.sentryTargetID = targetSentry;
	addressResponseMessage.byteParam1 = targetSentry;

	singleMan->radioMan()->sendMessage(addressResponseMessage);
}
