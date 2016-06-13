#include <Arduino.h>

#include "PinDefns.h"
#include "Utils.h"

#include "RadioManager.h"
#include "LightManager.h"

RadioManager *radioMan = NULL;
LightManager *lightMan = NULL;

void setup() {
  Serial.begin(9600);

  // Setup the addressing pins
  pinMode(ADDR_0_PIN, INPUT);
  pinMode(ADDR_1_PIN, INPUT);
  pinMode(ADDR_2_PIN, INPUT);
  pinMode(ADDR_3_PIN, INPUT);
  pinMode(ADDR_4_PIN, INPUT);

  // Initialize the Radio handler
  radioMan = new RadioManager(RADIO_CHIP_ENABLE_PIN, RADIO_CHIP_SELECT_PIN);
  radioMan->init();
  delay(5); // Wait for radio to init before continuing

  // Initilize the LightHandler
  lightMan = new LightManager(*radioMan);
  lightMan->init();
  delay(5); // Wait for light to init before continuing

  // Start interrupt handler for LightManagement
  init_TIMER2_irq();

  // Log that setup is complete
  Serial.print("Setup complete, Address: ");
  Serial.println(getAddress());
}

// initialize timer2 to redraw the LED strip and BigLight
void init_TIMER2_irq()
{
  // disable all interrupts while changing them
  noInterrupts();

  // Select clock source: internal I/O clock
  ASSR &= ~(1<<AS2);

  // Configure timer2 in normal mode (pure counting, no PWM etc.)
  TCCR2A = 0;
  TCCR2B = 0;

  // Set maximum divisor of 1/1024
  TCCR2B |= (1 << CS20);
  TCCR2B |= (1 << CS21);
  TCCR2B |= (1 << CS22);

  // Disable Compare Match A interrupt enable (only want overflow)
  TIMSK2 = 0;
  TIMSK2 |= (1<<TOIE2);

  // Finally load the timer start counter; lowest is where to count
  // start as interrupt happens at wrap around
  TCNT2 = 0; // <-- maximum time possible between interrupts

  // enable all interrupts now that things are ready to go
  interrupts();
}

// interrupt service routine for
ISR(TIMER2_OVF_vect)
{
  lightMan->updateLights();

  // load timer last to maximize time until next call
  TCNT2 = 0; // <-- maximum time possible between interrupts
}

#define timeDelay 500
#define timeBetweenNTPLoops 30000
void loop() {

  // LED control is handled by interrupt on timer2

  if (getAddress() == 0) {
    radioMan->blockingListenForRadioRequest(timeDelay);
  } else {
    static bool inNTPLoop = true;

    bool setValue = radioMan->NTPLoop(&inNTPLoop, timeDelay, timeBetweenNTPLoops);
    if(inNTPLoop == false && setValue == false)
      delay(timeDelay);
  }

}
