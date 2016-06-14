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

void loop() {

  // LED control is handled by interrupt on timer2

  // Now handle server types
  if (getAddress() == 0)
    serverLoop();
  else
    sentryLoop();
}


#define timeBetweenNTPUpdates 15000
#define timeBetweenLEDUpdates 1000
#define NUMBER_SENTRIES 2
void serverLoop() {

   static unsigned long lastNTPCheck = 0;
   static unsigned long lastLEDUpdateCheck = 0;

   if(millis() > lastNTPCheck + timeBetweenNTPUpdates)
   {
      static int nextSentryToRunNTP = 1;

      // Fire off a message to the next sentry to run an NTPloop
      EventMessage messageOut;
    	messageOut.message = NTP_MESSAGE;
    	messageOut.byteParam1 = nextSentryToRunNTP++;
    	messageOut.byteParam2 = 0;

      radioMan->sendEventMessage(messageOut);

      if(nextSentryToRunNTP > NUMBER_SENTRIES)
         nextSentryToRunNTP = 1;

      // Update lastNTPCheck
      lastNTPCheck = millis();
   } else if(millis() > lastLEDUpdateCheck + timeBetweenLEDUpdates) {
      lightMan->pattern = 1;
      lightMan->pattern_param1++;

      EventMessage lightMessage;
      lightMessage.message = COLOR_MESSAGE;
      lightMessage.byteParam1 = lightMan->pattern;
      lightMessage.byteParam2 = lightMan->pattern_param1;

      // Fire off a message to the next sentry to run an NTPloop
      radioMan->sendEventMessage(lightMessage);

      // Update lastLEDUpdateCheck
      lastLEDUpdateCheck = millis();
   }
   else
   {
     radioMan->checkForNTPRequest();
   }

   delay(10);
}

#define SENTRY_NTP_TIMEOUT 30000
void sentryLoop() {
   static bool inNTPLoop = false;

   if(radioMan->isNewServerMessage()) {
      EventMessage& serverMessage = radioMan->getEventMessage();
      byte messageType = serverMessage.message;

      // get the messageType
      if(messageType == NTP_MESSAGE)
      {
        Serial.print("sentryLoop :: NTP_MESSAGE   ");
        Serial.println(serverMessage.byteParam1);

         // If sentry address matches the NTP message is its turn to update
         if(serverMessage.byteParam1 == getAddress())
            inNTPLoop = true;
      }
      else if(messageType == COLOR_MESSAGE)
      {
         lightMan->pattern = serverMessage.byteParam1;
         lightMan->pattern_param1 = serverMessage.byteParam2;
      }
   }

   // if this sentry is supposed to be updating its time
   if(inNTPLoop)
   {
      radioMan->loopNTP(&inNTPLoop, SENTRY_NTP_TIMEOUT);
   }

   delay(100);
}
