#ifndef __ADDRESSMANAGER_H__
#define __ADDRESSMANAGER_H__

#include <Arduino.h>
#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class AddressManager
{
	public:
		AddressManager(SingletonManager* _singleMan);

		byte getZone();
		bool hasAddress();
		byte getAddress();

	private:
		SingletonManager* singleMan;
		bool addressSet;
		byte address;
		bool zoneSet;
		byte zone;
};

#endif // __ADDRESSMANAGER_H__
