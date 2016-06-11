#include <FastLED.h>

#define BIG_LED_PIN 10

#define RGB_STRIP_PIN 9
#define NUM_RGB_LEDS 6
 
#define ADDR_0 2
#define ADDR_1 3
#define ADDR_2 4
#define ADDR_3 5
#define ADDR_4 6

// simpleLoop count
int loopCount;

// LED strip declaraion
CRGB ledstrip[NUM_RGB_LEDS];


void setup() {
  Serial.begin(9600);

  loopCount = 0;
  
  pinMode(BIG_LED_PIN, OUTPUT);

  pinMode(ADDR_0, INPUT);
  pinMode(ADDR_1, INPUT);
  pinMode(ADDR_2, INPUT);
  pinMode(ADDR_3, INPUT);
  pinMode(ADDR_4, INPUT);

  // Init RTG
  FastLED.addLeds<NEOPIXEL, RGB_STRIP_PIN>(ledstrip, NUM_RGB_LEDS);

  Serial.print("Setup complete, Address: ");
  Serial.println(getAddress());
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
  Serial.println("***");

  // https://github.com/FastLED/FastLED/wiki/Pixel-reference
  int currPoint = loopCount%6;
  int prevPoint = currPoint-1;
  if(prevPoint == -1)
    prevPoint = 5;
  ledstrip[currPoint] = CRGB(0, 75, 0); 
  ledstrip[prevPoint] = CRGB::Black; 
  FastLED.show(); 

  
  // fade in from min to max in increments of 5 points:
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(BIG_LED_PIN, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
  // fade out from max to min in increments of 5 points:
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    // sets the value (range from 0 to 255):
    analogWrite(BIG_LED_PIN, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }


  loopCount++;
}
