#include "OutputManager.h"
#include "SingletonManager.h"

OutputManager::OutputManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	singleMan->setOutputMan(this);
}

bool OutputManager::showOutput(byte log_level) {
	switch(log_level) {
		case LOG_ERROR:  return(true);
		case LOG_INFO:   return(this->info_enabled);
		case LOG_DEBUG:  return(this->debug_enabled);
		case LOG_TIMING: return(this->timing_enabled);
		default:
			Serial.print(F("UNKNOWN_LOG_LEVEL<<"));
			Serial.print(log_level);
			Serial.print(F(">>"));
	}
	return false;
}
