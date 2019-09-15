#ifndef __HEALTHMANAGER_H__
#define __HEALTHMANAGER_H__

#include <MemoryFree.h>		// https://github.com/maniacbug/MemoryFree  (~52 bytes)

#include "SingletonManager.h"
class SingletonManager;

class SentryHealth
{
	public:
		byte id = 0; // byte
		unsigned long last_NTP_request = 0; // 4 bytes
		unsigned long last_message = 0; // 4 bytes
		bool isAlive = true; // byte
		byte last_light_level = 0; // byte
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

		void updateSentryNTPRequestTime(byte id, unsigned long ntpRequestTime)
			{ this->updateSentryInfo(id, ntpRequestTime, ntpRequestTime, 0); }
		void updateSentryMessageTime(byte id, unsigned long messageTime)
			{ this->updateSentryInfo(id, 0, messageTime, 0); }
		void updateSentryLightLevel(byte id, byte lightLevel)
			{ this->updateSentryInfo(id, 0, 0, lightLevel); }

		byte totalSentries() { return this->sentryCount; }
		byte nextAvailSentryID();
		byte getOldestNTPRequest();
		bool anyNonSyncedSentries();
		SentryHealth* findSentry(byte id);

		byte getServerAddress();

		void checkAllSentryHealth();
		void printHealth(OUTPUT_LOG_TYPES log_level);
		unsigned long getDeathOffset() { return DEATH_OFFSET; }

	private:
		SingletonManager* singleMan = NULL;
		SentryHealthNode* healthQueue = NULL;
		byte sentryCount = 0;
		unsigned long DEATH_OFFSET = 180000; // 3 min

		void pruneEndSentries();
		void updateSentryInfo(byte id, unsigned long ntpRequestTime, unsigned long messageTime, byte lightLevel);
		SentryHealth* addSentry(byte newID);
};

#endif // __HEALTHMANAGER_H__
