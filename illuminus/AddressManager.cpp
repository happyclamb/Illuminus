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

	info_print(F("Assigned Zone: "));
	info_println(getZone());
	info_print(F("Assigned Address: "));
	info_println(getAddress());
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
	while(hasAddress() == false && millis() < (requestStart + 3000)) {
		delay(20); // Give the radio a moment to wait for a message
		singleMan->radioMan()->checkRadioForData();
		RF24Message* currMessage = singleMan->radioMan()->popMessage();
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

	for(byte i=0; i < this->newAddressRetries; i++) {
		info_print(F("Attempt to get address: "));
		info_println(i);

		sendAddressRequest();
		if(hasAddress())
			break;

		// Need to wait long enough for messages to get processed
		delay(250);
	}

	// if timed out after this->newAddressRetries tries getting an address then
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
	addressResponseMessage.sentryTargetID = 255;
	addressResponseMessage.byteParam1 = targetSentry;

	singleMan->radioMan()->sendMessage(addressResponseMessage);
}
