#include "OutputManager.h"


OutputManager::OutputManager(SingletonManager* _singleMan) :
	singleMan(_singleMan)
{
	singleMan->setOutputMan(this);
}

bool OutputManager::showOutput(OUTPUT_LOG_TYPES log_level) {
	switch(log_level) {
		case LOG_ERROR:
		case LOG_CLI:
		case LOG_INFO:
		case LOG_DEBUG:
		case LOG_RADIO:
		case LOG_INPUTS:
#ifdef LOG_TIMING_DEFINED
		case LOG_TIMING:
#endif
			return(this->isLogLevelEnabled(log_level));
		default:
			Serial.print(F("UNKNOWN_LOG_LEVEL<<"));
			Serial.print(log_level);
			Serial.print(F(">>"));
	}
	return false;
}

void OutputManager::setLogLevel(OUTPUT_LOG_TYPES log_level, bool newValue) {
	switch(log_level) {
		case LOG_INFO:   this->info_enabled   = newValue;   break;
		case LOG_DEBUG:  this->debug_enabled  = newValue;   break;
		case LOG_RADIO:  this->radio_enabled  = newValue;   break;
		case LOG_INPUTS: this->inputs_enabled = newValue;   break;
#ifdef LOG_TIMING_DEFINED
		case LOG_TIMING: this->timing_enabled = newValue;   break;
#endif
	}
}

bool OutputManager::isLogLevelEnabled(OUTPUT_LOG_TYPES log_level) {
	switch(log_level) {
		case LOG_ERROR:  return(true);
		case LOG_CLI:    return(true);
		case LOG_INFO:   return(this->info_enabled);
		case LOG_DEBUG:  return(this->debug_enabled);
		case LOG_RADIO:  return(this->radio_enabled);
		case LOG_INPUTS: return(this->inputs_enabled);
#ifdef LOG_TIMING_DEFINED
		case LOG_TIMING: return(this->timing_enabled);
#endif
	}
}
