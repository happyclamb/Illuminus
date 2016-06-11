#include <Arduino.h>

#include <FastLED.h>

#include <SPI.h>
#include "RF24.h"

#include "PinDefns.h"
#include "Utils.h"

// interesting reads for timing
// http://www.gammon.com.au/forum/?id=12127
// http://playground.arduino.cc/Code/StopWatchClass

// LED strip declaraion
#define NUM_RGB_LEDS 6
CRGB ledstrip[NUM_RGB_LEDS];

// Init Radio
RF24 radio(RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
byte addresses[][6] = { "Chan1", "Chan2", "Chan3"};

void setup() {
  Serial.begin(9600);

  pinMode(BIG_LED_PIN, OUTPUT);

  pinMode(ADDR_0_PIN, INPUT);
  pinMode(ADDR_1_PIN, INPUT);
  pinMode(ADDR_2_PIN, INPUT);
  pinMode(ADDR_3_PIN, INPUT);
  pinMode(ADDR_4_PIN, INPUT);

  // Init RTG
  FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);

  for(int i=0; i<NUM_RGB_LEDS; i++)
      ledstrip[i] = CRGB(0,0,0);
  FastLED.show();

  // Turn on the big LED at 50%
  int bigLEDBrightness = 255 * 0.5;
  analogWrite(BIG_LED_PIN, bigLEDBrightness);

  // Init Radio
  radio.begin();

  // RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);

  // Set the data rate to the slowest (and most reliable) speed
  radio.setDataRate(RF24_1MBPS);

//  radio.setPayloadSize(sizeof(TimeCounter));

  // Open a writing and reading pipe on each radio, with opposite addresses
  if(getAddress() == 0) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }

  Serial.print("Setup complete, Address: ");
  Serial.println(getAddress());

  setMillisOffset(0);

  // kick off with listening
  radio.startListening();
}


void WalkingLED() {
  unsigned long currTime = getAdjustedMillis();

  // Over 2000ms break into '10' segments
  int litIndex = (currTime%2000)/200;

  // If the index is higher than 5, minus the offset
  // ie; 7-> 7-5*2 == 4 ... 3 should be lit
  if (litIndex > 5)
    litIndex -= (litIndex-5)*2;

  for(int i=0; i<NUM_RGB_LEDS; i++)
  {
    if(litIndex == i)
      ledstrip[i] = CRGB(0,25,25);
    else
      ledstrip[i] = CRGB(0,0,0);
  }
  FastLED.show();
}

void loop() {
  WalkingLED();

  int timeDelay = 100;
  if (getAddress() == 0) {
    // Handle radio communications
    static unsigned long lastClockSync = 0;
    if(millis() > lastClockSync + 5000)
    {
      lastClockSync = millis();
      unsigned long timingTook = handleTimingRequest(20000);
      Serial.print("    timingTook: ");
      Serial.println(timingTook);

      if(timingTook < timeDelay)
        delay(timeDelay - timingTook);
    } else {
      delay(timeDelay);
    }
  } else {
    unsigned long startTime = millis();
    while(millis() < startTime + timeDelay)
      handleTimingResponse();
  }
}


unsigned long handleTimingRequest(unsigned long maxListenTimeout)
{
  unsigned long functionStart = millis();

  // First, stop listening so we can talk.
  Serial.println("Sending Timing Request...");
  radio.stopListening();

  // Take the time, and send it.  This will block until complete
  TimeCounter pingOut;
  pingOut.client_start = micros();
  if (radio.write(&pingOut, sizeof(TimeCounter)) == false ) {
     Serial.println("failed");
  }

  // Now, continue listening
  radio.startListening();

  // Set up a timeout period, get the current microseconds
  unsigned long started_waiting_at = micros();

  // Set up a variable to indicate if a response was received or not
  boolean timeout = false;

  // While nothing is received
  while (radio.available() == false) {
    // If waited longer than 20ms, indicate timeout and exit while loop
    if (micros() - started_waiting_at > maxListenTimeout ){
        timeout = true;
        break;
    }
  }

  if ( timeout ){
      // Handle the timeout
      Serial.println("Failed, response timed out.");
  } else {
      // Grab the response, compare, and send to debugging spew
      TimeCounter timeData;
      radio.read( &timeData, sizeof(TimeCounter) );
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
      long offSet = ((timeData.server_start - timeData.client_start) + (timeData.server_end-timeData.client_end)) / 2;
      long rtripDelay = (timeData.client_end-timeData.client_start) - (timeData.server_end-timeData.server_start);

      offSet = offSet / 1000;
      rtripDelay = rtripDelay / 1000;
/*
      Serial.print("    offSet: ");
      Serial.print(offSet);
      Serial.print("    rtripDelay: ");
      Serial.print(rtripDelay);
*/
      setMillisOffset(offSet+rtripDelay);
/*
      long offSet = ((t1 - t0) + (t2-t3)) / 2;
      long rtripDelay = (t3-t0) - (t2-t1)
  t0 == client_start
  t1 == server_start
  t2 == server_end
  t3 == client_end
*/
  }

  return( millis()-functionStart );
}


void handleTimingResponse() {

  if (radio.available()) {

    unsigned long receive_time = micros();                         // Variable for the received timestamp
    TimeCounter clientTime;
    while (radio.available()) {                                   // While there is data ready
      radio.read( &clientTime, sizeof(TimeCounter) );             // Get the payload
    }
    clientTime.server_start = receive_time;

    radio.stopListening();                                        // First, stop listening so we can talk
    clientTime.server_end = micros();
    radio.write( &clientTime, sizeof(TimeCounter) );              // Send the final one back.

    radio.startListening();                                       // Now, resume listening so we catch the next packets.

    Serial.println("Sent response");
 }

}
