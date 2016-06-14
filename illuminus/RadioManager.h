#ifndef __RADIOMANAGER_H__
#define __RADIOMANAGER_H__

#include <Arduino.h>
#include "RF24.h"

class NTPMessage
{
  public:
    unsigned long client_start;
    unsigned long client_end;
    unsigned long server_start;
    unsigned long server_end;
};

enum Radio_Message { NO_MESSAGE, NTP_MESSAGE, COLOR_MESSAGE };
class EventMessage
{
  public:
    byte message;
    byte byteParam1;
    byte byteParam2;
};

enum PIPE_NAMES { NTP_CLIENT_REQUEST=1, NTP_SERVER_RESPONSE=2, SERVER_EVENT_MESSAGE=3 };

class RadioManager
{
  public:
    RadioManager(uint8_t radio_ce_pin, uint8_t radio__cs_pin);
    void init();
    unsigned long getAdjustedMillis();
    void loopNTP(bool *inNTPLoop, unsigned long timeoutMicros);
    void checkForNTPRequest();
    void sendEventMessage(EventMessage eventMessage);
    bool isNewServerMessage();
    EventMessage& getEventMessage();

  private:
    RF24 rf24;
    long currentMillisOffset;
    byte radioAddresses[4][6];
    EventMessage lastServerMessage;
    bool newServerMessage;

    bool checkForServerMessages();
    void setMillisOffset(long newOffset);
    bool loopNTPHelper(unsigned long timeoutMicros);
    long blockingGetOffsetFromServer(unsigned long timeoutMicros);
};

#endif // __RADIOMANAGER_H__
