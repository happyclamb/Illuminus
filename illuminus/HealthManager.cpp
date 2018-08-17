#include "HealthManager.h"

#include "IlluminusDefs.h"

#include "SingletonManager.h"

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

	info_print(F("HealthManager::nextAvailSentryID: "));
	info_println(nextID);

	// set the time to create the Node in the array
	updateSentryNTPRequestTime(nextID);

	printHealth();

	return nextID;
}

void HealthManager::checkAllSentryHealth() {
	SentryHealthNode *currNode = this->healthQueue;

	while(currNode != NULL) {

		// double the amout of time we think the sentry will respond in to give wiggle room
		unsigned long currTime = millis();
		unsigned long lastRequest = currNode->health->last_NTP_request_start;
		unsigned long deathOffset = TIME_BETWEEN_NTP_MSGS * (unsigned long) sentryCount * 2;
		unsigned long deadTime = lastRequest + deathOffset;

		if(currTime >= deadTime && currNode->health->isAlive == true) {
			currNode->health->isAlive = false;

			info_print(F("Sentry "));
			info_print(currNode->health->id);
			info_print(F(" went offline   "));
			debug_print(F("-->  currTime:"));
			debug_print(currTime);
			debug_print(F("  deadTime:"));
			debug_print(deadTime);
			debug_print(F("  lastRequest:"));
			debug_print(lastRequest);
			debug_print(F("  deathOffset:"));
			debug_println(deathOffset);
			info_println();

			printHealth();
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

		info_print(F("Sentry "));
		info_println(currAddress);
		info_println(F(" changed addres to 0 and promoted to Server"));
		printHealth();
	}

}

void HealthManager::pruneEndSentries() {
	SentryHealthNode *currNode = this->healthQueue;
	while(currNode != NULL) {

		// If the lastNode isAlive == false then nuke it
		if(currNode->next != NULL && currNode->next->next == NULL
				 && currNode->next->health->isAlive == false) {

			info_print(F("Deleting node:"));
			info_println(currNode->next->health->id);

			delete currNode->next->health;
			currNode->next->health = NULL;
			delete currNode->next;
			currNode->next = NULL;

			this->sentryCount--;

			printHealth();
		}

		currNode = currNode->next;
	}
}

void HealthManager::printHealth() {
	return;

	// These appears to be trying to dump too much info to the serial and kaaaBOOOM
	SentryHealthNode *currNode = this->healthQueue;
	debug_print(F("-------HealthManager::printHealth   currTime> "));
	debug_println(millis());
	debug_print(F("   sentryCount> "));
	debug_println(sentryCount);

	byte i=0;
	while(currNode != NULL) {
		// debug_print(F("   index:"));
		// debug_print(i);
		// debug_print(F("  id:"));
		// debug_print(currNode->health->id);
		// debug_print(F("  isAlive:"));
		// debug_print(currNode->health->isAlive);
		// debug_print(F("  lastRequest:"));
		// debug_println(currNode->health->last_NTP_request_start);

		i++;
		currNode = currNode->next;
	}
	debug_println(F("---------------------"));
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

	info_print(F("addedSentry: "));
	info_println(newID);
	printHealth();

	return(returnHealth);
}
