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

	singleMan->outputMan()->print(LOG_INFO, F("Zone: "));
	singleMan->outputMan()->println(LOG_INFO, getZone());
	singleMan->outputMan()->print(LOG_INFO, F("Address: "));
	singleMan->outputMan()->println(LOG_INFO, getAddress());
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
	while(hasAddress() == false &&
		millis() < (requestStart + singleMan->radioMan()->ntpRequestTimeout()))
	{
		delay(5);
		singleMan->radioMan()->checkRadioForData();

		// Peeking because later we'll want to process collected messages
		RF24Message* currMessage = singleMan->radioMan()->peekMessage();
		if(currMessage != NULL && (currMessage->messageType == NEW_ADDRESS_RESPONSE)) {
			setAddress(currMessage->param1_byte);
		}
	}
}


void AddressManager::obtainAddress() {

	for(byte i=0; i < this->newAddressRetries; i++) {
		singleMan->outputMan()->print(LOG_INFO, F("Attempt to get address: "));
		singleMan->outputMan()->println(LOG_INFO, i);

		sendAddressRequest();
		if(hasAddress())
			break;
	}

	// if timed out after this->newAddressRetries tries getting an address then
	//	there is no one else so become server
	if(hasAddress() == false) {
		// Force a new pattern to ensure all pattern params have a sane default
		singleMan->lightMan()->chooseNewPattern(10);

		// Now set address as it will use the newly inited pattern
		setAddress(0);
	}
}


void AddressManager::sendNewAddressResponse() {
	byte targetSentry = singleMan->healthMan()->nextAvailSentryID();

	RF24Message addressResponseMessage;
	addressResponseMessage.messageType = NEW_ADDRESS_RESPONSE;
	addressResponseMessage.sentrySrcID = 0;
	addressResponseMessage.sentryTargetID = 255;
	addressResponseMessage.param1_byte = targetSentry;

	singleMan->radioMan()->sendMessage(addressResponseMessage);
}
