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

	// Seems like a good time to double check that everyone is healthy
	checkAllSentryHealth();
}

SentryHealth* HealthManager::findSentry(byte id) {
	SentryHealthNode *currNode = healthQueue;

	while(currNode != NULL) {
		if(currNode->health->id == id)
			return currNode->health;

		currNode = currNode->next;
	}

	return NULL;
}

byte HealthManager::totalSentries() {
	return sentryCount;
}

byte HealthManager::nextAvailSentryID() {
	SentryHealthNode *currNode = healthQueue;

	// If nothing is found then sentry '0' is next
	if(currNode == NULL)
		return 0;

	// Check health of Everything
	checkAllSentryHealth();

	int nextID = 0;
	while(currNode != NULL) {

		// If we found a dead sentry, reuse the ID
		if(currNode->health->isAlive == false) {
			nextID = currNode->health->id;
			// have to revive or else it'll get clobbered
			currNode->health->isAlive = true;
			break;
		}

		nextID++;
		currNode = currNode->next;
	}

Serial.print("NextAvailID: ");
Serial.println(nextID);
	// set the time to create the Node in the array
	updateSentryNTPRequestTime(nextID);
	return nextID;
}

void HealthManager::checkAllSentryHealth() {
	SentryHealthNode *currNode = healthQueue;


	while(currNode != NULL) {

		// double the amout of time we think the sentry will respond in to give wiggle room
		unsigned long currTime = millis();
		unsigned long lastRequest = currNode->health->last_NTP_request_start;
		unsigned long deadTime = lastRequest + (TIME_BETWEEN_NTP_MSGS * sentryCount * 2);

		if(currTime >= deadTime && currNode->health->isAlive == true) {
Serial.print("is DEAD  currNode->health->id: ");
Serial.print(currNode->health->id);

Serial.print("  currTime:");
Serial.print(currTime);
Serial.print("  deadTime:");
Serial.print(deadTime);
Serial.print("  lastRequest:");
Serial.println(lastRequest);

				currNode->health->isAlive = false;

				printHealth();
		}

		currNode = currNode->next;
	}

	pruneEndSentries();

//	printHealth();
}

void HealthManager::pruneEndSentries() {
	SentryHealthNode *currNode = healthQueue;
	while(currNode != NULL) {

		// If the lastNode isAlive == false then nuke it
		if(currNode->next != NULL && currNode->next->next == NULL
				 && currNode->next->health->isAlive == false) {
			Serial.print("Removing node:");
			Serial.println(currNode->next->health->id);

			delete currNode->next->health;
			currNode->next->health = NULL;
			delete currNode->next;
			currNode->next = NULL;
			sentryCount--;

			printHealth();
		}

		currNode = currNode->next;
	}
}

void HealthManager::printHealth() {
	SentryHealthNode *currNode = healthQueue;

	Serial.print("-------HealthManager::printHealth   sentryCount> ");
	Serial.println(sentryCount);

	byte i=0;
	while(currNode != NULL) {
		Serial.print("   index:");
		Serial.print(i);
		Serial.print("  id:");
		Serial.print(currNode->health->id);
		Serial.print("  isAlive:");
		Serial.print(currNode->health->isAlive);
		Serial.print("  lastRequest:");
		Serial.println(currNode->health->last_NTP_request_start);

		i++;
		currNode = currNode->next;
	}
	Serial.println("---------------------");
}

SentryHealth* HealthManager::addSentry(byte newID) {
	SentryHealth* returnHealth = NULL;

	if(healthQueue == NULL)
	{
		returnHealth = new SentryHealth();
		returnHealth->id = newID;
		returnHealth->last_NTP_request_start = millis();
		returnHealth->isAlive = true;

		healthQueue = new SentryHealthNode();
		healthQueue->health  = returnHealth;
		healthQueue->next = NULL;

		sentryCount = 1;
	} else {

		SentryHealthNode *lastNode = healthQueue;
		sentryCount = 1;
		while(lastNode->next != NULL) {
			sentryCount++;
			lastNode = lastNode->next;
		}

		returnHealth = new SentryHealth();
		returnHealth->id = newID;
		returnHealth->last_NTP_request_start = millis();
		returnHealth->isAlive = true;

		lastNode->next = new SentryHealthNode();
		lastNode->next->health = returnHealth;
		lastNode->next->next = NULL;

		sentryCount++;
	}

	Serial.print("addSentry: New Total Sentries:");
	Serial.println(sentryCount);

printHealth();

	return(returnHealth);
}
