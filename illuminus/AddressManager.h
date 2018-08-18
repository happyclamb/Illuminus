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
		void setAddress(byte newAddress);

		void obtainAddress();
		void sendNewAddressResponse();

	private:
		SingletonManager* singleMan = NULL;
		bool addressSet = false;
		byte address = 255;
		byte newAddressRetries = 3;

		void sendAddressRequest();
};

#endif // __ADDRESSMANAGER_H__
