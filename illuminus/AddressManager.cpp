#include "AddressManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

AddressManager::AddressManager(SingletonManager* _singleMan) :
	singleMan(_singleMan),
	addressSet(false),
	address(0),
	zoneSet(false),
	zone(0)
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

byte AddressManager::getAddress() {

	address = 0;
	if(digitalRead(ADDR_0_PIN) == LOW)
		address += 1;
	if(digitalRead(ADDR_1_PIN) == LOW)
		address += 2;
	if(digitalRead(ADDR_2_PIN) == LOW)
		address += 4;

	return address;
}
