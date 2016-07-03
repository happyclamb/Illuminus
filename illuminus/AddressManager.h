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

		byte getLanternCount() { return lanternCount; }
		void setLanternCount(byte numLanterns) { lanternCount = numLanterns; }

		byte getZone();

		bool hasAddress();
		byte getAddress();
		void setAddress(byte newAddress);
		void obtainAddress();
		void sendNewAddressResponse();

	private:
		SingletonManager* singleMan = NULL;
		bool addressSet = false;
		byte address = 0;
		byte lanternCount = 0;
		bool zoneSet = false;
		byte zone = 0;

		void sendAddressRequest();
};

#endif // __ADDRESSMANAGER_H__
