#include <Arduino.h>

#include <FastLED.h>

#include "PinDefns.h"
#include "Utils.h"

#include "RadioManager.h"

// interesting reads for timing
// http://www.gammon.com.au/forum/?id=12127
// http://playground.arduino.cc/Code/StopWatchClass

// LED strip declaraion
#define NUM_RGB_LEDS 6
CRGB ledstrip[NUM_RGB_LEDS];

// Radio Manager
RadioManager radioMan(RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);

// Init Radio
// RF24 radio(RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
// byte addresses[][6] = { "Chan1", "Chan2", "Chan3"};

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

  // Initialize the Radio handler
  radioMan.init();

  // Log that setup is complete
  Serial.print("Setup complete, Address: ");
  Serial.println(getAddress());
}


void WalkingLED() {
  unsigned long currTime = radioMan.getAdjustedMillis();

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

    unsigned long preSendTime = millis();

    unsigned long maxSendTimeMicros = 20000;
    long newOffset = radioMan.blockingGetOffsetFromServer(maxSendTimeMicros);
    radioMan.setMillisOffset(newOffset);

    long remainingTime = (preSendTime + timeDelay) - millis();
    if(remainingTime > 0)
      delay(remainingTime);
  } else {
    radioMan.blockingListenForRadioRequest(timeDelay);
  }
}
