#include "AddressManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

AddressManager::AddressManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	// Setup the addressing pins
	pinMode(ZONE_0_PIN, INPUT_PULLUP);
	pinMode(ZONE_1_PIN, INPUT_PULLUP);

	singleMan->setAddrMan(this);
}


byte AddressManager::getZone() {
	zone = 0;
	// HIGH == off, LOW == on
	if(digitalRead(ZONE_0_PIN) == LOW)
		zone += 1;
	if(digitalRead(ZONE_1_PIN) == LOW)
		zone += 2;

	return (zone);
}
