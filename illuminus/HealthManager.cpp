#include "HealthManager.h"

#include "IlluminusDefs.h"
#include "SingletonManager.h"
#include "OutputManager.h"

HealthManager::HealthManager(SingletonManager* _singleMan) :
	singleMan(_singleMan),
	healthQueue(NULL)
{
	singleMan->setHealthMan(this);
}

void HealthManager::updateSentryNTPRequestTime(byte id) {

	// 255 is the 'special' everyone address so don't store it
	if(id == 255)
		return;

	SentryHealth* currSentry = findSentry(id);
	if(currSentry == NULL) {
		currSentry = addSentry(id);
	}
	currSentry->last_NTP_request_start = millis();
	currSentry->isAlive = true;
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

byte HealthManager::totalSentries() {
	return this->sentryCount;
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

	singleMan->outputMan()->print(LOG_INFO, F("HealthManager::nextAvailSentryID: "));
	singleMan->outputMan()->println(LOG_INFO, nextID);

	// set the time to create the Node in the array
	updateSentryNTPRequestTime(nextID);

	return nextID;
}

void HealthManager::checkAllSentryHealth() {
	SentryHealthNode *currNode = this->healthQueue;

	while(currNode != NULL) {

		// double the amout of time we think the sentry will respond in to give wiggle room
		unsigned long currTime = millis();
		unsigned long lastRequest = currNode->health->last_NTP_request_start;
		unsigned long deathOffset = singleMan->radioMan()->getIntervalBetweenNTPChecks() * (unsigned long) sentryCount * 2;
		unsigned long deadTime = lastRequest + deathOffset;

		if(currTime >= deadTime && currNode->health->isAlive == true) {
			currNode->health->isAlive = false;

			singleMan->outputMan()->print(LOG_INFO, F("Sentry "));
			singleMan->outputMan()->print(LOG_INFO, currNode->health->id);
			singleMan->outputMan()->println(LOG_INFO, F(" went offline   "));

			if(singleMan->outputMan()->isLogLevelEnabled(LOG_DEBUG)) {
				singleMan->outputMan()->print(LOG_DEBUG, F("-->  currTime:"));
				singleMan->outputMan()->print(LOG_DEBUG, currTime);
				singleMan->outputMan()->print(LOG_DEBUG, F("  deadTime:"));
				singleMan->outputMan()->print(LOG_DEBUG, deadTime);
				singleMan->outputMan()->print(LOG_DEBUG, F("  lastRequest:"));
				singleMan->outputMan()->print(LOG_DEBUG, lastRequest);
				singleMan->outputMan()->print(LOG_DEBUG, F("  deathOffset:"));
				singleMan->outputMan()->println(LOG_DEBUG, deathOffset);
			}

			printHealth(LOG_INFO);
		}

		currNode = currNode->next;
	}

	pruneEndSentries();

	selectNewServer();
}

void HealthManager::selectNewServer() {

	// if still in setup stage OR already is the server then skip this
	if(singleMan->addrMan()->hasAddress() == false ||
			singleMan->addrMan()->getAddress() == 0)
		return;

	byte currAddress = singleMan->addrMan()->getAddress();
	bool foundAliveSentry = false;
	for(byte i=0; i<currAddress; i++) {
		SentryHealth* foundSentry = findSentry(i);
		if(foundSentry->isAlive == true) {
			foundAliveSentry = true;
			break;
		}
	}

	if(foundAliveSentry == false) {
		singleMan->addrMan()->setAddress(0);
		singleMan->radioMan()->setMillisOffset(0);
		updateSentryNTPRequestTime(0);

		SentryHealth* oldSentry = findSentry(currAddress);
		oldSentry->isAlive = false;

		singleMan->outputMan()->print(LOG_INFO, F("Sentry "));
		singleMan->outputMan()->print(LOG_INFO, currAddress);
		singleMan->outputMan()->println(LOG_INFO, F(" changed address to 0 and promoted to Server"));
		printHealth(LOG_INFO);
	}

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
		singleMan->outputMan()->print(log_level, F("currTime > "));
		singleMan->outputMan()->print(log_level, millis());
		singleMan->outputMan()->print(log_level, F("    adjustedTime > "));
		singleMan->outputMan()->print(log_level, singleMan->radioMan()->getAdjustedMillis());
		singleMan->outputMan()->print(log_level, F("    sentryCount > "));
		singleMan->outputMan()->print(log_level, sentryCount);
		singleMan->outputMan()->print(log_level, F("    this.address > "));
		singleMan->outputMan()->println(log_level, singleMan->addrMan()->getAddress());

		singleMan->outputMan()->print(log_level, F("nextPattern > "));
		singleMan->lightMan()->getNextPattern()->printPattern(singleMan, log_level);
		singleMan->outputMan()->println(log_level, F(""));

		byte i=0;
		while(currNode != NULL) {
			singleMan->outputMan()->print(log_level, F("  id:"));
			singleMan->outputMan()->print(log_level, currNode->health->id);
			singleMan->outputMan()->print(log_level, F("  isAlive:"));
			singleMan->outputMan()->print(log_level, currNode->health->isAlive);
			singleMan->outputMan()->print(log_level, F("  lastRequest:"));
			singleMan->outputMan()->println(log_level, currNode->health->last_NTP_request_start);

			i++;
			currNode = currNode->next;
		}
		singleMan->outputMan()->println(log_level, F("----------------------------"));
	}
}

SentryHealth* HealthManager::addSentry(byte newID) {
	SentryHealth* returnHealth = NULL;

	if(this->healthQueue == NULL) {
		returnHealth = new SentryHealth();
		returnHealth->id = newID;
		returnHealth->last_NTP_request_start = millis();
		returnHealth->isAlive = true;

		this->healthQueue = new SentryHealthNode();
		this->healthQueue->health  = returnHealth;
		this->healthQueue->next = NULL;

		this->sentryCount = 1;
	} else {

		SentryHealthNode *lastNode = this->healthQueue;
		while(lastNode->next != NULL) {
			lastNode = lastNode->next;
		}

		returnHealth = new SentryHealth();
		returnHealth->id = newID;
		returnHealth->last_NTP_request_start = millis();
		returnHealth->isAlive = true;

		lastNode->next = new SentryHealthNode();
		lastNode->next->health = returnHealth;
		lastNode->next->next = NULL;

		this->sentryCount++;
	}

	singleMan->outputMan()->print(LOG_INFO, F("addedSentry: "));
	singleMan->outputMan()->println(LOG_INFO, newID);
	printHealth(LOG_INFO);

	return(returnHealth);
}
