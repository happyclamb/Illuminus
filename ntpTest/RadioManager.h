#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

class TimeCounter
{
  public:
    unsigned long  client_start;
    unsigned long  client_end;
    unsigned long  server_start;
    unsigned long  server_end;
};

class RadioManager
{
  public:
    RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin);
    void init();

    unsigned long getAdjustedMillis();
    void setMillisOffset(long newOffset);
    long blockingGetOffsetFromServer(unsigned long maxListenTimeout);
    void blockingListenForRadioRequest(unsigned long listenLength);

    RF24 rf24;
  private:
    long currentMillisOffset;
    byte radioAddresses[3][6];
};

#endif // __RADIOMANAGER_H__
