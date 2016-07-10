#ifndef __SINGLETONMANAGER_H__
#define __SINGLETONMANAGER_H__

#include "RadioManager.h"
class RadioManager;

#include "LightManager.h"
class LightManager;

#include "AddressManager.h"
class AddressManager;

#include "HealthManager.h"
class HealthManager;

class SingletonManager
{
	public:
		SingletonManager() {}

		RadioManager* radioMan() { return radioManager; }
		void setRadioMan(RadioManager* _radioMan) { radioManager = _radioMan; }

		LightManager* lightMan() { return lightManager; }
		void setLightMan(LightManager* _lightMan) { lightManager = _lightMan; }

		AddressManager* addrMan()  { return addrManager; }
		void setAddrMan(AddressManager* _addrMan) { addrManager = _addrMan; }

		HealthManager* healthMan()  { return healthManager; }
		void setHealthMan(HealthManager* _healthMan) { healthManager = _healthMan; }

	private:
		RadioManager* radioManager = NULL;
		LightManager* lightManager = NULL;
		AddressManager* addrManager = NULL;
		HealthManager* healthManager = NULL;
};

#endif // __SINGLETONMANAGER_H__
