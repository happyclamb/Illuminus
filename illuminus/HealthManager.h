#ifndef __HEALTHMANAGER_H__
#define __HEALTHMANAGER_H__

#include <Arduino.h>
#include "IlluminusDefs.h"
#include "OutputManager.h"

#include "SingletonManager.h"
class SingletonManager;

class SentryHealth
{
	public:
		byte id = 0; // byte
		unsigned long last_NTP_request = 0; // 4 bytes
		unsigned long last_message = 0; // 4 bytes
		bool isAlive = true; // byte
};

class SentryHealthNode {
	public:
		SentryHealth *health = NULL;
		SentryHealthNode *next = NULL;
};

class HealthManager
{
	public:
		HealthManager(SingletonManager* _singleMan);
		void updateSentryHealthTime(byte id, unsigned long ntpRequestTime, unsigned long messageTime);
		byte totalSentries();
		byte nextAvailSentryID();
		byte getOldestNTPRequest();
		SentryHealth* findSentry(byte id);
		void checkAllSentryHealth();
		void selectNewServer();
		void printHealth(OUTPUT_LOG_TYPES log_level);
		unsigned long getDeathOffset() { return deathOffset; }
		void setLastAddressAllocated(byte newValue) { this->last_address_responded = newValue; }


	private:
		SingletonManager* singleMan = NULL;
		SentryHealthNode* healthQueue = NULL;
		byte sentryCount = 0;
		unsigned long deathOffset = 120000;
		byte last_address_responded = 0;

		void pruneEndSentries();
		SentryHealth* addSentry(byte newID);
};

#endif // __HEALTHMANAGER_H__
