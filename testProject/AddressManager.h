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

	private:
		SingletonManager* singleMan = NULL;
		byte zone = 0;
};

#endif // __ADDRESSMANAGER_H__
