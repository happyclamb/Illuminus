#include <FastLED.h>

#include <SPI.h>
#include "RF24.h"

// #define serialRX 0
// #define serialTX 1
#define ADDR_0 2
#define ADDR_1 3
#define ADDR_2 4
#define ADDR_3 5
#define ADDR_4 6
#define RADIO_CHIP_ENABLE 7
#define RADIO_CHIP_SELECT 8
#define RGB_STRIP_PIN 9
#define BIG_LED_PIN 10

// interesting read
// https://hallard.me/nrf24l01-real-life-range-test/
// http://www.wayengineer.com/index.php?main_page=product_info&products_id=876
// http://www.instructables.com/id/Enhanced-NRF24L01/

#define NUM_RGB_LEDS 6
 
// LED strip declaraion
CRGB ledstrip[NUM_RGB_LEDS];

// Init Radio
RF24 radio(RADIO_CHIP_ENABLE, RADIO_CHIP_SELECT);
byte addresses[][6] = {"1Node","2Node"};

void setup() {
  Serial.begin(9600);

  pinMode(BIG_LED_PIN, OUTPUT);

  pinMode(ADDR_0, INPUT);
  pinMode(ADDR_1, INPUT);
  pinMode(ADDR_2, INPUT);
  pinMode(ADDR_3, INPUT);
  pinMode(ADDR_4, INPUT);

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
  radio.setPALevel(RF24_PA_HIGH);  

  // Set the data rate to the slowest (and most reliable) speed
  radio.setDataRate(RF24_1MBPS);

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

  // kick off with listening
  radio.startListening();
}

int getAddress() {
  int returnValue = 0;

  if(digitalRead(ADDR_0) == LOW)
    returnValue += 1;
  if(digitalRead(ADDR_1) == LOW)
    returnValue += 2;
  if(digitalRead(ADDR_2) == LOW)
    returnValue += 4;
  if(digitalRead(ADDR_3) == LOW)
    returnValue += 8;
  if(digitalRead(ADDR_4) == LOW)
    returnValue += 16;

  return (returnValue); 
}

void loop() {

  // Handle radio communications
  if (getAddress() == 0)
    handleRadioPingOut();
  else
    handleRadioResponse();
}

void handleRadioPingOut()
{    
  // First, stop listening so we can talk.
  Serial.println(F("Now sending"));
  radio.stopListening();

  // Take the time, and send it.  This will block until complete
  unsigned long start_time = micros();
  if (radio.write(&start_time, sizeof(unsigned long)) == false ) {
     Serial.println(F("failed"));
  }

  // Now, continue listening
  radio.startListening();                                    

  // Set up a timeout period, get the current microseconds
  unsigned long started_waiting_at = micros();               

  // Set up a variable to indicate if a response was received or not
  boolean timeout = false;                                   

  // While nothing is received
  while (radio.available() == false) {
    // If waited longer than 200ms, indicate timeout and exit while loop
    if (micros() - started_waiting_at > 200000 ){            
        timeout = true;
        break;
    }      
  }

  static int successCount = 0;

  if ( timeout ){
      // Handle the timeout
      Serial.println(F("Failed, response timed out."));

      successCount--;
  } else {
      // Grab the response, compare, and send to debugging spew
      unsigned long got_time;                                 
      radio.read( &got_time, sizeof(unsigned long) );
      unsigned long end_time = micros();
      
      // Spew it
      Serial.print(F("Sent "));
      Serial.print(start_time);
      Serial.print(F(", Got response "));
      Serial.print(got_time);
      Serial.print(F(", Round-trip delay "));
      Serial.print(end_time-start_time);
      Serial.println(F(" microseconds"));

      successCount++;
  }

  if(successCount < 0) successCount = 0;
  if(successCount > 6) successCount = 6;
  for(int i=0; i<6; i++)
  {
      ledstrip[i] = i<successCount ? CRGB(0,75,0) : CRGB(75,0,0);
  }
  FastLED.show(); 

  // Try again 1s later
  delay(1000);
}


int responseLoopCount = -1;
void handleRadioResponse() {

  if (radio.available()){
    unsigned long got_time;                                       // Variable for the received timestamp
    while (radio.available()) {                                   // While there is data ready
      radio.read( &got_time, sizeof(unsigned long) );             // Get the payload
    }
   
    radio.stopListening();                                        // First, stop listening so we can talk   
    radio.write( &got_time, sizeof(unsigned long) );              // Send the final one back.      
    radio.startListening();                                       // Now, resume listening so we catch the next packets.     

    Serial.print(F("Sent response "));
    Serial.println(got_time);  

    responseLoopCount++;
 }

  int currPoint = responseLoopCount%6;
  int prevPoint = currPoint-1;
  if(prevPoint == -1)
    prevPoint = 5;
  ledstrip[currPoint] = CRGB(0, 60, 0); 
  ledstrip[prevPoint] = CRGB(0, 0, 60); 
  FastLED.show(); 

   // Try again 200ms later
  delay(200);
}


