#ifndef __HEALTHMANAGER_H__
#define __HEALTHMANAGER_H__

#include <Arduino.h>
#include "IlluminusDefs.h"

#include "SingletonManager.h"
class SingletonManager;

class SentryHealth
{
	public:
		byte id = 0; // byte
		unsigned long last_NTP_request_start = 0; // 4 bytes
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
		void updateSentryNTPRequestTime(byte id);
		byte totalSentries();
		byte nextAvailSentryID();
		SentryHealth* findSentry(byte id);
		void checkAllSentryHealth();
		void selectNewServer();
		void printHealth();

	private:
		SingletonManager* singleMan = NULL;
		SentryHealthNode* healthQueue = NULL;
		byte sentryCount = 0;

		void pruneEndSentries();
		SentryHealth* addSentry(byte newID);
};

#endif // __HEALTHMANAGER_H__
