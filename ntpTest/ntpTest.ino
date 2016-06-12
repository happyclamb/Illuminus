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


#define timeDelay 100
#define timeBetweenNTPLoops 30000
void loop() {

  WalkingLED();

  if (getAddress() == 0) {
    radioMan.blockingListenForRadioRequest(timeDelay);
  } else {
    static bool inNTPLoop = true;
unsigned long ntpStart = millis();

    bool setValue = radioMan.NTPLoop(&inNTPLoop, timeDelay, timeBetweenNTPLoops);
    if(inNTPLoop == false && setValue == false)
      delay(timeDelay);

unsigned long ntpEnd = millis();
Serial.print("ntpLoop time is  ");
Serial.println(ntpEnd - ntpStart);

  }

}
