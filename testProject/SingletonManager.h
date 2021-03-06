#ifndef __SINGLETONMANAGER_H__
#define __SINGLETONMANAGER_H__

#include "InputManager.h"
class InputManager;

#include "AddressManager.h"
class AddressManager;

#include "LightManager.h"
class LightManager;

#include "RadioManager.h"
class RadioManager;

class SingletonManager
{
	public:
		SingletonManager() {}

		InputManager* inputMan() { return inputManager; }
		void setInputMan(InputManager* _inputMan) { inputManager = _inputMan; }

		AddressManager* addrMan()  { return addrManager; }
		void setAddrMan(AddressManager* _addrMan) { addrManager = _addrMan; }

		LightManager* lightMan() { return lightManager; }
		void setLightMan(LightManager* _lightMan) { lightManager = _lightMan; }

		RadioManager* radioMan() { return radioManager; }
		void setRadioMan(RadioManager* _radioMan) { radioManager = _radioMan; }

	private:
		InputManager* inputManager = NULL;
		AddressManager* addrManager = NULL;
		LightManager* lightManager = NULL;
		RadioManager* radioManager = NULL;
};

#endif // __SINGLETONMANAGER_H__
