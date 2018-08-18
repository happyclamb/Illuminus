#ifndef __SINGLETONMANAGER_H__
#define __SINGLETONMANAGER_H__

#include "InputManager.h"
class InputManager;

#include "OutputManager.h"
class OutputManager;

#include "AddressManager.h"
class AddressManager;

#include "LightManager.h"
class LightManager;

#include "RadioManager.h"
class RadioManager;

#include "HealthManager.h"
class HealthManager;

class SingletonManager
{
	public:
		SingletonManager() {}

		InputManager* inputMan() { return inputManager; }
		void setInputMan(InputManager* _inputMan) { inputManager = _inputMan; }

		OutputManager* outputMan() { return outputManager; }
		void setOutputMan(OutputManager* _outputMan) { outputManager = _outputMan; }

		AddressManager* addrMan()  { return addrManager; }
		void setAddrMan(AddressManager* _addrMan) { addrManager = _addrMan; }

		LightManager* lightMan() { return lightManager; }
		void setLightMan(LightManager* _lightMan) { lightManager = _lightMan; }

		RadioManager* radioMan() { return radioManager; }
		void setRadioMan(RadioManager* _radioMan) { radioManager = _radioMan; }

		HealthManager* healthMan()  { return healthManager; }
		void setHealthMan(HealthManager* _healthMan) { healthManager = _healthMan; }

	private:
		InputManager* inputManager = NULL;
		OutputManager* outputManager = NULL;
		AddressManager* addrManager = NULL;
		LightManager* lightManager = NULL;
		RadioManager* radioManager = NULL;
		HealthManager* healthManager = NULL;
};

#endif // __SINGLETONMANAGER_H__
