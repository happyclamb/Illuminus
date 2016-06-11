#include "Utils.h"

#include "PinDefns.h"

long millisOffset;

void setMillisOffset(long newOffset) {
	millisOffset = newOffset;
}

unsigned long getAdjustedMillis() {
	return millis() + millisOffset;
}

int getAddress() {
  int returnValue = 0;

  if(digitalRead(ADDR_0_PIN) == LOW)
    returnValue += 1;
  if(digitalRead(ADDR_1_PIN) == LOW)
    returnValue += 2;
  if(digitalRead(ADDR_2_PIN) == LOW)
    returnValue += 4;
  if(digitalRead(ADDR_3_PIN) == LOW)
    returnValue += 8;
  if(digitalRead(ADDR_4_PIN) == LOW)
    returnValue += 16;

  return (returnValue);
}
