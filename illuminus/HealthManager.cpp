#include "HealthManager.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"
#include "OutputManager.h"

// https://github.com/maniacbug/MemoryFree
// 52 bytes
#include <MemoryFree.h>

HealthManager::HealthManager(SingletonManager* _singleMan) :
	singleMan(_singleMan),
	healthQueue(NULL)
{
	singleMan->setHealthMan(this);
}


byte HealthManager::getServerAddress() {
	SentryHealthNode *currNode = this->healthQueue;
	while(currNode != NULL) {
		if(currNode->health->isAlive == true)
			return currNode->health->id;
		currNode = currNode->next;
	}

	return 255;
}


void HealthManager::updateSentryInfo(byte id,
	unsigned long ntpRequestTime, unsigned long messageTime, byte lightLevel) {

	// 255 is the 'special' everyone address so don't store it
	if(id == 255)
		return;

	SentryHealth* currSentry = findSentry(id);
	if(currSentry == NULL) {
		currSentry = addSentry(id);
	}

	currSentry->isAlive = true;
	if(ntpRequestTime > 0)
		currSentry->last_NTP_request = ntpRequestTime;
	if(messageTime > 0)
		currSentry->last_message = messageTime;
	if(lightLevel > 0)
		currSentry->last_light_level = lightLevel;
}


SentryHealth* HealthManager::findSentry(byte id) {
	SentryHealthNode *currNode = this->healthQueue;

	while(currNode != NULL) {
		if(currNode->health->id == id)
			return currNode->health;

		currNode = currNode->next;
	}

	return NULL;
}


byte HealthManager::nextAvailSentryID() {
	SentryHealthNode *currNode = this->healthQueue;

	// If nothing is found then sentry '0' is next
	if(currNode == NULL)
		return 0;

	// Check healths before looking for IDs
	checkAllSentryHealth();

	int nextID = 0;
	while(currNode != NULL) {

		// If we found a dead sentry, reuse the ID
		if(currNode->health->isAlive == false) {
			nextID = currNode->health->id;
			break;
		}

		nextID++;
		currNode = currNode->next;
	}

	return nextID;
}


byte HealthManager::getOldestNTPRequest() {
	SentryHealthNode *currNode = this->healthQueue;

	// If nothing is found then sentry '0' is next
	if(currNode == NULL)
		return 0;

	// Check healths before looking for IDs
	checkAllSentryHealth();

	byte old_id = 0;
	unsigned long old_time = millis();
	while(currNode != NULL) {

		// Only care about alive sentries
		if(currNode->health->isAlive && currNode->health->last_NTP_request <= old_time) {
			old_time = currNode->health->last_NTP_request;
			old_id = currNode->health->id;
		}

		currNode = currNode->next;
	}

	return old_id;
}


void HealthManager::checkAllSentryHealth() {
	SentryHealthNode *currNode = this->healthQueue;

	while(currNode != NULL) {

		unsigned long currTime = millis();
		unsigned long lastRequest = currNode->health->last_message;
		unsigned long deadTime = lastRequest + this->deathOffset;

		if(currTime > deadTime && currNode->health->isAlive == true) {
			currNode->health->isAlive = false;

			singleMan->outputMan()->print(LOG_INFO, F("Sentry "));
			singleMan->outputMan()->print(LOG_INFO, currNode->health->id);
			singleMan->outputMan()->println(LOG_INFO, F(" went offline   "));

			printHealth(LOG_INFO);
		}

		currNode = currNode->next;
	}

	pruneEndSentries();
}


void HealthManager::pruneEndSentries() {
	SentryHealthNode *currNode = this->healthQueue;
	while(currNode != NULL) {

		// If the lastNode isAlive == false then nuke it
		if(currNode->next != NULL && currNode->next->next == NULL
			&& currNode->next->health->isAlive == false)
		{
			singleMan->outputMan()->print(LOG_INFO, F("Deleting node:"));
			singleMan->outputMan()->println(LOG_INFO, currNode->next->health->id);

			delete currNode->next->health;
			currNode->next->health = NULL;
			delete currNode->next;
			currNode->next = NULL;

			this->sentryCount--;

			printHealth(LOG_INFO);
		}

		currNode = currNode->next;
	}
}


void HealthManager::printHealth(OUTPUT_LOG_TYPES log_level) {

	if (singleMan->outputMan()->isLogLevelEnabled(log_level)) {
		SentryHealthNode *currNode = this->healthQueue;
		singleMan->outputMan()->println(log_level, F("------- HealthManager -------"));
		singleMan->outputMan()->print(log_level, F("currTime> "));
		singleMan->outputMan()->print(log_level, millis());
		singleMan->outputMan()->print(log_level, F("  adjustedTime> "));
		singleMan->outputMan()->print(log_level, singleMan->radioMan()->getAdjustedMillis());
		singleMan->outputMan()->print(log_level, F("  sentryCount> "));
		singleMan->outputMan()->print(log_level, sentryCount);
		singleMan->outputMan()->print(log_level, F("  zone> "));
		singleMan->outputMan()->print(log_level, singleMan->addrMan()->getZone());
		singleMan->outputMan()->print(log_level, F("  this.address> "));
		singleMan->outputMan()->print(log_level, singleMan->addrMan()->getAddress());
		singleMan->outputMan()->print(log_level, F("  freeMemory> "));
		singleMan->outputMan()->println(log_level, freeMemory());

		singleMan->outputMan()->print(log_level, F("stacksize>  receive> "));
		singleMan->outputMan()->print(log_level, singleMan->radioMan()->receiveStackSize());
		singleMan->outputMan()->print(log_level, F("  upstream> "));
		singleMan->outputMan()->print(log_level, singleMan->radioMan()->upstreamStackSize());
		singleMan->outputMan()->print(log_level, F("  downstream> "));
		singleMan->outputMan()->println(log_level, singleMan->radioMan()->downstreamStackSize());

		singleMan->outputMan()->print(log_level, F("    currPattern> "));
		singleMan->lightMan()->getCurrPattern()->printlnPattern(singleMan, log_level);
		singleMan->outputMan()->print(log_level, F("    nextPattern > "));
		singleMan->lightMan()->getNextPattern()->printlnPattern(singleMan, log_level);

		byte i=0;
		while(currNode != NULL) {
			singleMan->outputMan()->print(log_level, F("  id:"));
			singleMan->outputMan()->print(log_level, currNode->health->id);
			singleMan->outputMan()->print(log_level, F("  isAlive:"));
			singleMan->outputMan()->print(log_level, currNode->health->isAlive);
			singleMan->outputMan()->print(log_level, F("  lastRequest:"));
			singleMan->outputMan()->print(log_level, currNode->health->last_message);
			singleMan->outputMan()->print(log_level, F("  lastNTP:"));
			singleMan->outputMan()->print(log_level, currNode->health->last_NTP_request);
			singleMan->outputMan()->print(log_level, F("  lightLevel:"));
			singleMan->outputMan()->println(log_level, currNode->health->last_light_level);

			i++;
			currNode = currNode->next;
		}
		singleMan->outputMan()->println(log_level, F("----------------------------"));
	}
}


SentryHealth* HealthManager::addSentry(byte newID) {
	SentryHealth* returnHealth = new SentryHealth();
	returnHealth->id = newID;
	returnHealth->last_message = millis();
	returnHealth->last_NTP_request = 0;
	returnHealth->last_light_level = 0;
	returnHealth->isAlive = true;

	// Create new health node
	SentryHealthNode *newNode = new SentryHealthNode();
	newNode->health = returnHealth;
	newNode->next = NULL;

	if(this->healthQueue == NULL) {
		this->healthQueue = newNode;
		this->sentryCount = 1;
	} else {

		SentryHealthNode *currNode = this->healthQueue;
		while(currNode != NULL) {
			SentryHealthNode *nextNode = currNode->next;
			if (currNode->health->id < newID) {
				if (nextNode == NULL || nextNode->health->id > newID) {
					newNode->next = nextNode;
					currNode->next = newNode;
					nextNode = NULL;
				}
			}
			currNode = nextNode;
		}

		this->sentryCount++;
	}

	singleMan->outputMan()->print(LOG_INFO, F("addedSentry: "));
	singleMan->outputMan()->println(LOG_INFO, newID);
	printHealth(LOG_INFO);

	return(returnHealth);
}
