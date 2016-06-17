#include "Utils.h"

#include "IlluminusDefs.h"

// because of a wiring defect in board #3 that has addr4 always low; just reading the first 3 bits.
// 000 = server
// 001 = sentry_1
// 010 = sentry_2
// 011 = sentry_3
// 100 = sentry_4
// 101 = sentry_5
// 110 = sentry_6
// 111 = sentry_7
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
//		if(digitalRead(ADDR_3_PIN) == LOW)
//			returnAddress += 8;
//		if(digitalRead(ADDR_4_PIN) == LOW)
//			returnAddress += 16;
	}

	return (returnAddress);
}
