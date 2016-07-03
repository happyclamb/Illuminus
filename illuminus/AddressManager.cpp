#include "AddressManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

AddressManager::AddressManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	// Setup the addressing pins
	pinMode(ADDR_0_PIN, INPUT);
	pinMode(ADDR_1_PIN, INPUT);
	pinMode(ADDR_2_PIN, INPUT);
	pinMode(ADDR_3_PIN, INPUT);
	pinMode(ADDR_4_PIN, INPUT);

	singleMan->setAddrMan(this);
}

byte AddressManager::getZone() {
	if (zoneSet == false)
	{
		zone = 0;
		if(digitalRead(ADDR_0_PIN) == LOW)
			zone += 1;
		if(digitalRead(ADDR_1_PIN) == LOW)
			zone += 2;
		if(digitalRead(ADDR_2_PIN) == LOW)
			zone += 4;

		zoneSet = true;
	}
	return (zone);
}


bool AddressManager::hasAddress() {
	return addressSet;
}

void AddressManager::setAddress(byte newAddress) {
	address = newAddress;
	addressSet = true;
}

byte AddressManager::getAddress() {
	return address;
}

void AddressManager::sendAddressRequest() {
	RF24Message addressRequestMessage;
	addressRequestMessage.messageType = NEW_ADDRESS_REQUEST;
	singleMan->radioMan()->sendMessage(addressRequestMessage);

	unsigned long requestStart = millis();

	while(hasAddress() == false && millis() < (requestStart + 2000)) {

		singleMan->radioMan()->checkRadioForData();
		RF24Message *currMessage = singleMan->radioMan()->popMessage();
		while(currMessage != NULL) {
			if(currMessage->messageType == NEW_ADDRESS_RESPONSE) {
				setAddress(currMessage->byteParam1);
				lanternCount = getAddress() + 1;
			}
			delete currMessage;

			currMessage = singleMan->radioMan()->popMessage();
		}

		delay(3);
	}
}

void AddressManager::obtainAddress() {

	for(int i=0; i<7; i++) {
		sendAddressRequest();
		if(hasAddress())
			break;
	}

	// if timed out after 7 tries getting an address then there is no one else so become server
	if(hasAddress() == false) {
		setAddress(0);
		lanternCount = 1;
	}

	Serial.print("address: ");
	Serial.println(getAddress());
}

void AddressManager::sendNewAddressResponse() {
	RF24Message addressResponseMessage;
	addressResponseMessage.messageType = NEW_ADDRESS_RESPONSE;
	addressResponseMessage.byteParam1 = lanternCount;
	singleMan->radioMan()->sendMessage(addressResponseMessage);

	// A new lantern was added; increase count
	lanternCount++;
}
