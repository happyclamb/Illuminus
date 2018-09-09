#ifndef __OUTPUTMANAGER_H__
#define __OUTPUTMANAGER_H__

#include <Arduino.h>

#include "SingletonManager.h"
class SingletonManager;

class OutputManager
{
	public:
		OutputManager(SingletonManager* _singleMan);

		bool isLogLevelEnabled(OUTPUT_LOG_TYPES log_level);
		void setLogLevel(OUTPUT_LOG_TYPES log_level, bool newValue);

		void print(OUTPUT_LOG_TYPES log_level, byte msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, byte msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, int msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, int msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, unsigned int msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, unsigned int msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, float msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, float msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, unsigned long msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, unsigned long msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, long msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, long msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

		void print(OUTPUT_LOG_TYPES log_level, const __FlashStringHelper* msg)
			{ if(this->showOutput(log_level)) { Serial.print(msg); } }
		void println(OUTPUT_LOG_TYPES log_level, const __FlashStringHelper* msg)
			{ if(this->showOutput(log_level)) { Serial.println(msg); } }

	private:
		SingletonManager* singleMan = NULL;
		bool info_enabled = false;
		bool debug_enabled = false;
		bool radio_enabled = false;
		bool timing_enabled = false;

		bool showOutput(OUTPUT_LOG_TYPES log_level);
};

#endif // __OUTPUTMANAGER_H__
