#include "RadioManager.h"

#include "Utils.h"

#include <SPI.h>
#include "RF24.h"

RadioManager::RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin):
		currentMillisOffset(0),
		rf24(RF24(radio_ce_pin, radio__cs_pin)),
		radioAddresses{ "Chan1", "Chan2", "Chan3"}
{
}

void RadioManager::init() {
	// Init Radio
	rf24.begin();

	// RF24_PA_MAX is default.
	rf24.setPALevel(RF24_PA_MAX);

	// Set the data rate to the slowest (and most reliable) speed
	rf24.setDataRate(RF24_1MBPS);

	// Might need to update this??
	//  rf24.setPayloadSize(sizeof(TimeCounter));

	// Open a writing and reading pipe on each radio, with opposite addresses
	if(getAddress() == 0) {
		rf24.openWritingPipe(radioAddresses[0]);
		rf24.openReadingPipe(1,radioAddresses[1]);
	} else {
		rf24.openWritingPipe(radioAddresses[1]);
		rf24.openReadingPipe(1,radioAddresses[0]);
	}

	// kick off with listening
	rf24.startListening();
}


void RadioManager::setMillisOffset(long newOffset) {
	Serial.print("RadioManager::setMillisOffset    newOffset: ");
	Serial.println(newOffset);
	currentMillisOffset = newOffset;
}

unsigned long RadioManager::getAdjustedMillis() {
	return millis() + currentMillisOffset;
}

long RadioManager::blockingGetOffsetFromServer(unsigned long maxListenTimeout)
{
	static int getOffsetCount=0;

  // First, stop listening so we can talk.
  Serial.print("RadioManager::blockingGetOffsetFromServer    ");
	Serial.println(getOffsetCount++);

  rf24.stopListening();

  // Take the time, and send it.  This will block until complete
  TimeCounter pingOut;
  pingOut.client_start = micros();
  if (rf24.write(&pingOut, sizeof(TimeCounter)) == false ) {
     Serial.println("Error: Write Failed");
		 return 0;
  }

  // Now, continue listening
  rf24.startListening();

  // Set up a timeout period, get the current microseconds
  unsigned long started_waiting_at = micros();

  // Set up a variable to indicate if a response was received or not
  boolean timeout = false;

  // While nothing is received
  while (rf24.available() == false) {
    // If waited longer than 20ms, indicate timeout and exit while loop
    if (micros() - started_waiting_at > maxListenTimeout ){
        timeout = true;
        break;
    }
  }

  if ( timeout ){
      // Handle the timeout
      Serial.println("Error: Response timed out.");
			return 0;
  }

	// Grab the response, compare, and send to debugging spew
	TimeCounter timeData;
	rf24.read( &timeData, sizeof(TimeCounter) );
	timeData.client_end = micros();

  // Spew it
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
	Serial.print(timeData.server_end);
*/
  // Finally have enough data to Do The Math
  // https://en.wikipedia.org/wiki/Network_Time_Protocol#Clock_synchronization_algorithm
  long offSet = ((long)(timeData.server_start - timeData.client_start) + (long)(timeData.server_end - timeData.client_end)) / (long)2;
  long halfRtripDelay = ((timeData.client_end - timeData.client_start) - (timeData.server_end - timeData.server_start)) / 2;

/*
	Serial.print("    offSet: ");
  Serial.print(offSet);
  Serial.print("    halfRtripDelay: ");
  Serial.print(halfRtripDelay);
*/
	// Need to convert from Micros to Millis
  offSet = ( offSet + halfRtripDelay ) / 1000;

/*
      long offSet = ((t1 - t0) + (t2-t3)) / 2;
      long rtripDelay = (t3-t0) - (t2-t1)
  t0 == client_start
  t1 == server_start
  t2 == server_end
  t3 == client_end

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

  return(offSet);
}


void RadioManager::blockingListenForRadioRequest(unsigned long listenLength) {
	static int getOffsetCount=0;
	unsigned long startTime = millis();

	while(millis() < startTime + listenLength)
	{
	  if (rf24.available()) {

	    unsigned long receive_time = micros();                         // Variable for the received timestamp
	    TimeCounter clientTime;
	    while (rf24.available()) {                                   // While there is data ready
	      rf24.read( &clientTime, sizeof(TimeCounter) );             // Get the payload
	    }
	    clientTime.server_start = receive_time;

	    rf24.stopListening();                                        // First, stop listening so we can talk
	    clientTime.server_end = micros();
	    rf24.write( &clientTime, sizeof(TimeCounter) );              // Send the final one back.

	    rf24.startListening();                                       // Now, resume listening so we catch the next packets.

	    Serial.print("Sent response   ");
			Serial.println(getOffsetCount++);
	 }
 }

}
