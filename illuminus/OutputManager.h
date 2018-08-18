#ifndef __OUTPUTMANAGER_H__
#define __OUTPUTMANAGER_H__

#include <Arduino.h>
#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

enum Output_Log_Types {
	LOG_ERROR,
	LOG_INFO,
	LOG_DEBUG,
	LOG_TIMING
};

class OutputManager
{
	public:
		OutputManager(SingletonManager* _singleMan);

		bool isInfoEnabled(){ return this->info_enabled; }
		void setInfoEnabled(bool newValue){ this->info_enabled = newValue; }
		bool isDebugEnabled(){ return this->debug_enabled; }
		void setDebugEnabled(bool newValue){ this->debug_enabled = newValue; }
		bool isTimingEnabled(){ return this->timing_enabled; }
		void setTimingEnabled(bool newValue){ this->timing_enabled = newValue; }

		void print(byte log_level, byte msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(byte log_level, byte msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(byte log_level, int msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(byte log_level, int msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(byte log_level, unsigned long msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(byte log_level, unsigned long msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(byte log_level, long msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(byte log_level, long msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(byte log_level, const __FlashStringHelper* msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(byte log_level, const __FlashStringHelper* msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

	private:
		SingletonManager* singleMan = NULL;
		bool info_enabled = false;
		bool debug_enabled = false;
		bool timing_enabled = false;

		bool showOutput(byte log_level);
};

#endif // __OUTPUTMANAGER_H__
