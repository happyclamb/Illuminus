#ifndef __ADDRESSMANAGER_H__
#define __ADDRESSMANAGER_H__

#include "SingletonManager.h"
class SingletonManager;

class AddressManager
{
	public:
		AddressManager(SingletonManager* _singleMan);

		byte getZone();

		bool hasAddress() { return this->addressSet; }
		byte getAddress() { return this->address; }
		void setAddress(byte newAddress);

		void obtainAddress();
		void sendNewAddressResponse();

	private:
		SingletonManager* singleMan = NULL;
		bool addressSet = false;
		byte address = 255;
		byte newAddressRetries = 5;

		void sendAddressRequest();
};

#endif // __ADDRESSMANAGER_H__
