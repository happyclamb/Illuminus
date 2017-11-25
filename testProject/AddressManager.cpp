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
