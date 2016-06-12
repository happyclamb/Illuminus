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

  // initialize timer1 to toggle the LED strip (and BigLight?)
  noInterrupts();           // disable all interrupts

  // Select clock source: internal I/O clock
  ASSR &= ~(1<<AS2);

  // Configure timer2 in normal mode (pure counting, no PWM etc.)
  TCCR2A = 0;
  TCCR2B = 0;

  // Set maximum divisor of 1/1024
  TCCR2B |= (1 << CS20);
  TCCR2B |= (1 << CS21);
  TCCR2B |= (1 << CS22);

  /* Disable Compare Match A interrupt enable (only want overflow) */
  TIMSK2 = 0;
  TIMSK2 |= (1<<TOIE2);

  // Finally load the timer start counter; lowest is where to count
  // start as interrupt happens at wrap around
  TCNT2 = 0; // <-- maximum time possible between interrupts

  interrupts();             // enable all interrupts

  // Log that setup is complete
  Serial.print("Setup complete, Address: ");
  Serial.println(getAddress());
}

// interrupt service routine for
ISR(TIMER2_OVF_vect)
{
  // preload timer
  TCNT2 = 0; // <-- maximum time possible between interrupts
  WalkingLED();
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


#define timeDelay 500
#define timeBetweenNTPLoops 30000
void loop() {

  // LED control is handled by interrupt on timer2

  if (getAddress() == 0) {
    radioMan.blockingListenForRadioRequest(timeDelay);
  } else {
    static bool inNTPLoop = true;

    bool setValue = radioMan.NTPLoop(&inNTPLoop, timeDelay, timeBetweenNTPLoops);
    if(inNTPLoop == false && setValue == false)
      delay(timeDelay);
  }

}
