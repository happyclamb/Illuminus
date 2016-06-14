#include "RadioManager.h"

#include <SPI.h>
#include "RF24.h"

#include "Utils.h"

// http://maniacbug.github.io/RF24/classRF24.html

RadioManager::RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		currentMillisOffset(0),
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		lastServerMessage(),
		newServerMessage(false),
		radioAddresses{ 0xF0F0F0F0AA, 0xF0F0F0F0BB, 0xF0F0F0F0CC, 0xF0F0F0F0DD }
{
}

// node0 == NTP rx
// node1 == NTP tx
// node2 == Message
void RadioManager::init() {
	// Init Radio
	rf24.begin();

	// RF24_PA_MAX is default.
  //	rf24.setPALevel(RF24_PA_MAX);
	rf24.setPALevel(RF24_PA_LOW);

	// Set the data rate to the slowest (and most reliable) speed
	rf24.setDataRate(RF24_1MBPS);

	// Disable dynamicAck so we can send to multiple sentries
  rf24.setAutoAck(false);

	/* EXAMPLE from:
	http://tmrh20.github.io/RF24/classRF24.html#a6253607ac2a1995af91a35cea6899c31
	radio.enableDynamicAck();
	radio.write(&data,32,1);  // Sends a payload with no acknowledgement requested
	radio.write(&data,32,0);  // Sends a payload using auto-retry/autoACK
	*/

	// Might need to update this??
	//  rf24.setPayloadSize(sizeof(NTPMessage));
//	rf24.enableDynamicPayloads() ;

	// Open a writing and reading pipe on each radio, with opposite addresses
	if(getAddress() == 0) {
		rf24.openWritingPipe(radioAddresses[SERVER_EVENT_MESSAGE]);
		rf24.openReadingPipe(NTP_CLIENT_REQUEST, radioAddresses[NTP_CLIENT_REQUEST]);
	} else {
		rf24.openWritingPipe(radioAddresses[NTP_CLIENT_REQUEST]);
		rf24.openReadingPipe(NTP_CLIENT_REQUEST, radioAddresses[NTP_CLIENT_REQUEST]);
		rf24.openReadingPipe(NTP_SERVER_RESPONSE, radioAddresses[NTP_SERVER_RESPONSE]);
		rf24.openReadingPipe(SERVER_EVENT_MESSAGE, radioAddresses[SERVER_EVENT_MESSAGE]);
	}

	// kick off with listening
	rf24.startListening();
}

// http://electronics.stackexchange.com/questions/102715/broadcast-in-nrf24l01
// https://forum.arduino.cc/index.php?topic=261258.0

void RadioManager::setMillisOffset(long newOffset) {
	Serial.print("RadioManager::setMillisOffset    newOffset: ");
	Serial.println(newOffset);
	currentMillisOffset = newOffset;
}

unsigned long RadioManager::getAdjustedMillis() {
	return millis() + currentMillisOffset;
}


void RadioManager::sendEventMessage(EventMessage eventMessage) {
	rf24.stopListening();
	rf24.openWritingPipe(radioAddresses[SERVER_EVENT_MESSAGE]);

	if (rf24.write(&eventMessage, sizeof(EventMessage)) == false ) {
		Serial.println("sendEventMessage :: Error: Write Failed");
		rf24.startListening();
		return;
	}

	rf24.startListening();
}

bool RadioManager::isNewServerMessage() {
	bool returnVal = newServerMessage;

	if(returnVal == false)
		returnVal = checkForServerMessages();

	newServerMessage = false;
	return(returnVal);
}

bool RadioManager::checkForServerMessages() {
	uint8_t pipeRead = 10;
	if (rf24.available(&pipeRead)) {
		rf24.read( &lastServerMessage, sizeof(EventMessage) );
		return true;
	}

	return false;
}

EventMessage& RadioManager::getEventMessage() {
	return(lastServerMessage);
}

void RadioManager::loopNTP(bool *inNTPLoop, unsigned long timeoutMicros) {
	// returns true if the NTPLoop updates the value
	if(*inNTPLoop == true)
	{
		*inNTPLoop = this->loopNTPHelper(timeoutMicros);
	}
}

#define OFFSET_SUCCESSES 7
#define MIDDLE_SUCCESSES 3
bool RadioManager::loopNTPHelper(unsigned long timeoutMicros) {

  static long offsetCollection[OFFSET_SUCCESSES];
  static int currOffsetIndex = 0;

  bool inNTPLoop = true;
  long newOffset = this->blockingGetOffsetFromServer(timeoutMicros);
  if(newOffset != 0)
  {

    Serial.print("Success   ");
    Serial.println(newOffset);

    offsetCollection[currOffsetIndex] = newOffset;
    currOffsetIndex++;

    // Once there are OFFSET_SUCCESSES offsets, average and set it.
    if(currOffsetIndex==OFFSET_SUCCESSES) {

			for(int i=1;i<OFFSET_SUCCESSES;++i)
			{
					for(int j=0;j<(OFFSET_SUCCESSES-i);++j)
							if(offsetCollection[j] > offsetCollection[j+1])
							{
									long temp = offsetCollection[j];
									offsetCollection[j] = offsetCollection[j+1];
									offsetCollection[j+1] = temp;
							}
			}

			int excludeCount = (OFFSET_SUCCESSES - MIDDLE_SUCCESSES) / 2;

      long long summedOffset = 0;
      for(int i=excludeCount; i<excludeCount+MIDDLE_SUCCESSES;i++)
        summedOffset += offsetCollection[i];

      this->setMillisOffset(summedOffset/MIDDLE_SUCCESSES);

      currOffsetIndex = 0;
      inNTPLoop = false;
    }
  }

  return(inNTPLoop);
}

long RadioManager::blockingGetOffsetFromServer(unsigned long maxListenTimeout)
{
/*
	static int getOffsetCount=0;
  Serial.print("RadioManager::blockingGetOffsetFromServer    ");
	Serial.println(getOffsetCount++);
*/

	// First, stop listening so we can talk.
  rf24.stopListening();

  // Take the time, and send it.  This will block until complete
  NTPMessage ntpOut;
  ntpOut.client_start = micros();
	ntpOut.client_end = 0;
	ntpOut.server_start = 0;
	ntpOut.server_end = 0;

	rf24.closeReadingPipe(NTP_CLIENT_REQUEST);
	rf24.openWritingPipe(radioAddresses[NTP_CLIENT_REQUEST]);
  if (rf24.write(&ntpOut, sizeof(NTPMessage)) == false ) {
     Serial.println("blockingGetOffsetFromServer :: Error: Write Failed");

		 // Restart Listening or no idea what state we're in
	   rf24.startListening();
		 return 0;
  }

  // Now, continue listening
	rf24.openReadingPipe(NTP_CLIENT_REQUEST, radioAddresses[NTP_CLIENT_REQUEST]);
  rf24.startListening();

  // Set up a timeout period, get the current microseconds
  unsigned long started_waiting_at = micros();

  // Set up a variable to indicate if a response was received or not
  boolean timeout = false;

  // While nothing is received
	uint8_t pipeAvail = 10;
  while (rf24.available(&pipeAvail) == false) {
		if (micros() - started_waiting_at > maxListenTimeout ){
      // If waited longer than maxListenTimeout, indicate timeout
      timeout = true;
      break;
    }
  }

	if ( timeout ) {
      // Handle the timeout
      Serial.println("Error: Response timed out.");
			return 0;
	}

	NTPMessage timeData;
//	if(pipeAvail == NTP_SERVER_RESPONSE) {
		rf24.read(&timeData, sizeof(NTPMessage) );
		timeData.client_end = micros();
//	}	else if(pipeAvail == SERVER_EVENT_MESSAGE) {
//		rf24.read( &lastServerMessage, sizeof(EventMessage) );
//		newServerMessage = true;
//	}

	// Spew Debug Info
/*
	Serial.print("VagueTxRxTime: ");
	Serial.print(timeData.client_end - timeData.client_start);
	Serial.print("    client_start: ");
	Serial.print(timeData.client_start);
	Serial.print("    client_end: ");
	Serial.print(timeData.client_end);
	Serial.print("    server_start: ");
	Serial.print(timeData.server_start);
	Serial.print("    server_end: ");
	Serial.println(timeData.server_end);
*/

  /* Finally have enough data to Do The Math
   		https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
		offSet = ((t1 - t0) + (t2-t3)) / 2
		halfRtripDelay = ((t3-t0) - (t2-t1)) / 2
				t0 == client_start
				t1 == server_start
				t2 == server_end
				t3 == client_end
	*/
	//  long offSet = ((long)(timeData.server_start - timeData.client_start) + (long)(timeData.server_end - timeData.client_end)) / (long)2;
	long t1_t0 = timeData.server_start - timeData.client_start;
	long t2_t3 = timeData.server_end - timeData.client_end;
	long long offset_LL = (t1_t0 + t2_t3);
	long offset = (long) (offset_LL / 2);

	//	long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;
	long t3_t0 = timeData.client_end - timeData.client_start;
	long t2_t1 = timeData.server_end - timeData.server_start;
 	long halfRtripDelay = (t3_t0 - t2_t1) / 2;

	// Update offset to use the delay
	offset = offset + halfRtripDelay;

	// Need to convert from Micros to Millis
	offset = ((offset+500)/1000);

	// return
	return(offset);

/*
	// Math for Server running ahead
	long offSet = ((t1 - t0) + (t2-t3)) / 2;
	long rtripDelay = (t3-t0) - (t2-t1)
			t0 == client_start			1000
			t1 == server_start			2000
			t2 == server_end				2010
			t3 == client_end				1110

			2000-1000   +   2010-1110
			       1000 + 900
						 			1900/2 == 950

			1110-1000 -  2010-2000
			110 -  10  == 100

	// Math for Server running behind
	long offSet = ((t1 - t0) + (t2-t3)) / 2;
	long rtripDelay = (t3-t0) - (t2-t1)
			t0 == client_start			2000
			t1 == server_start			1000
			t2 == server_end				1010
			t3 == client_end				2110

			1000-2000  +   1010-2110
			-1000 +   -1100
						-2100/2 == -1050

			2110-2000 - 1010-1000
			110 - 10 == 100
*/
}


void RadioManager::checkForNTPRequest() {
	static int getOffsetCount=0;

	uint8_t pipeAvail = 10;
  if (rf24.available(&pipeAvail)) {

		Serial.print("RadioManager::checkForNTPRequest :: pipeAvail   ");
	  Serial.println(pipeAvail);

		if(pipeAvail == NTP_CLIENT_REQUEST)
		{
	    unsigned long receive_time = micros();

      // Get the payload
			NTPMessage clientTime;
      rf24.read( &clientTime, sizeof(NTPMessage) );
	    clientTime.server_start = receive_time;

			// stop listening so we can talk
	    rf24.stopListening();

			// ensure we write to the NTP responsePipe
			rf24.openWritingPipe(radioAddresses[NTP_SERVER_RESPONSE]);

	    clientTime.server_end = micros();
	    rf24.write( &clientTime, sizeof(NTPMessage) );

	    // Resume listening so we catch the next packets.
	    rf24.startListening();

	    Serial.print("Sent response    #");
	    Serial.println(getOffsetCount++);
		}
 }

}
