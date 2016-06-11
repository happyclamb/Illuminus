#ifndef __UTILS_H__
#define __UTILS_H__

#include <Arduino.h>

void setMillisOffset(long newOffset);
unsigned long getAdjustedMillis();
int getAddress();

class TimeCounter
{
  public:
    unsigned long  client_start;
    unsigned long  client_end;
    unsigned long  server_start;
    unsigned long  server_end;
};

#endif // __UTILS_H__
