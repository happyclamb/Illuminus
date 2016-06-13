#include "Utils.h"

#include "PinDefns.h"

int getAddress(bool forceReread) {
  int static returnAddress = -1;

  if (returnAddress == -1 || forceReread == true)
  {
    returnAddress = 0;

    if(digitalRead(ADDR_0_PIN) == LOW)
      returnAddress += 1;
    if(digitalRead(ADDR_1_PIN) == LOW)
      returnAddress += 2;
    if(digitalRead(ADDR_2_PIN) == LOW)
      returnAddress += 4;
    if(digitalRead(ADDR_3_PIN) == LOW)
      returnAddress += 8;
    if(digitalRead(ADDR_4_PIN) == LOW)
      returnAddress += 16;
  }

  return (returnAddress);
}
