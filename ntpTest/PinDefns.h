#ifndef __PINDEFNS_H__
#define __PINDEFNS_H__

//////////////// DIGITAL PINS
// #define serialRX 0		// Used for serial debugging
// #define serialTX 1		// Used for serial debugging
#define ADDR_0_PIN 2		// Digital || INT_1=0 future change to migrate this to IRQ for RF24
#define ADDR_1_PIN 3		// Digital || PWM || INT_1
#define ADDR_2_PIN 4		// Digital
#define ADDR_3_PIN 5		// Digital || PWM
#define ADDR_4_PIN 6		// Digital || PWM
#define RADIO_CHIP_ENABLE_PIN 7		// Digital
#define RADIO_CHIP_SELECT_PIN 8		// Digital || CLOCK_0
#define RGB_STRIP_PIN 9			// Digital || PWM
#define BIG_LED_PIN 10 			// Digital || PWM
#define RADIO_MOSI_PIN 11		// Digital || MOSI
#define RADIO_MISO_PIN 12		// Digital || MISO
#define RADIO_SCK_PIN 13		// Digital || SCK


//////////////// ANALOG PINS as Analog
#define UNUSED_A0_PIN A0		// Analog
#define UNUSED_A1_PIN A1		// Analog
#define UNUSED_A2_PIN A2		// Analog
#define UNUSED_A3_PIN A3		// Analog


//////////////// ANALOG PINS as Digital
#define UNUSED_A0_PIN 14		// Digital
#define UNUSED_A1_PIN 15		// Digital
#define UNUSED_A2_PIN 16		// Digital
#define UNUSED_A3_PIN 17		// Digital




#endif // __PINDEFNS_H__