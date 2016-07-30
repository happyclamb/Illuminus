#ifndef __ILLUMINUSDEFS_H__
#define __ILLUMINUSDEFS_H__

//#define INFO
//#define DEBUG
//#define TIMING

#ifdef INFO
 #define info_print(x)     Serial.print(x)
 #define info_println(x)   Serial.println(x)
#else
 #define info_print(x)
 #define info_println(x)
#endif

#ifdef DEBUG
 #define debug_print(x)     Serial.print(x)
 #define debug_println(x)   Serial.println(x)
#else
 #define debug_print(x)
 #define debug_println(x)
#endif

#ifdef TIMING
 #define timing_print(x)     Serial.print(x)
 #define timing_println(x)   Serial.println(x)
#else
 #define timing_print(x)
 #define timing_println(x)
#endif

const unsigned long TIME_BETWEEN_NTP_MSGS = 5000;
const unsigned long TIME_BETWEEN_LED_MSGS = 500;
#define NEW_ADDRESS_RETRIES 7

#define MAX_STORED_MSG_IDS 64
#define NTP_OFFSET_SUCCESSES_REQUIRED 7
#define NTP_OFFSET_SUCCESSES_USED 3

#define FORCE_PATTERN_CHANGE 30000
#define PATTERN_CHANGE_DELAY 250
#define COLOR_STEPS_IN_WHEEL 255
#define LIGHT_PATTERNS_DEFINED 5
#define NUM_RGB_LEDS 6

//////////////// DIGITAL PINS
// #define serialRX 0		// Used for serial debugging
// #define serialTX 1		// Used for serial debugging
#define RADIO_INT_PIN 2		// Digital || INT_1=0 future change to migrate this to IRQ for RF24
#define UNUSED_3_PIN 3		// Digital || PWM || INT_1
#define RGB_STRIP_PIN 4		// Digital
#define ZONE_0_PIN 5		// Digital || PWM
#define ZONE_1_PIN 6		// Digital || PWM
#define RADIO_CHIP_ENABLE_PIN 7		// Digital
#define RADIO_CHIP_SELECT_PIN 8		// Digital || CLOCK_0
#define BIG_LED_PIN 9			// Digital || PWM
#define TIMER1_PIN	 10 		// Digital || PWM		[note; should move to timer2 PWM as timer1 is 16bit and more useful for LED timing]
#define RADIO_MOSI_PIN 11		// Digital || MOSI
#define RADIO_MISO_PIN 12		// Digital || MISO
#define RADIO_SCK_PIN 13		// Digital || SCK


//////////////// ANALOG PINS as Analog
#define UNUSED_A0_PIN A0					// Analog
#define LIGHT_SENSOR_A1_PIN A1		// Analog; pin 15
#define MOTION_SENSOR_A2_PIN A2		// Analog; pin 16
#define SOUND_SENSOR_A3_PIN A3		// Analog; pin 17


#endif // __ILLUMINUSDEFS_H__
