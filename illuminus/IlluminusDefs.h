#ifndef __ILLUMINUSDEFS_H__
#define __ILLUMINUSDEFS_H__

//#define LOG_TIMING_DEFINED

enum Radio_Message_Type:byte {
	UNDEFINED_MESSAGE,
	NEW_ADDRESS_REQUEST,
	NEW_ADDRESS_RESPONSE,
	NTP_COORD_MESSAGE,
	NTP_RESET_MESSAGE,
	NTP_CLIENT_REQUEST,
	NTP_SERVER_RESPONSE,
	NTP_CLIENT_FINISHED,
	COLOR_MESSAGE_TO_SENTRY,
	KEEP_ALIVE_FROM_SENTRY
};

enum OUTPUT_LOG_TYPES:byte {
	LOG_ERROR,
	LOG_CLI,
	LOG_INFO,
	LOG_DEBUG,
#ifdef LOG_TIMING_DEFINED
	LOG_TIMING,
#endif
	LOG_RADIO,
	LOG_INPUTS
};

enum LightPatternTimingOptions:byte {
	PATTERN_TIMING_SYNC,
	PATTERN_TIMING_STAGGER,
	PATTERN_TIMING_ALTERNATE,
	PATTERN_TIMING_NONE
};

enum LightPatternInsideColor:byte {
	INSIDE_SOLID,
	INSIDE_CANDY_CANE,
	INSIDE_HALF_HALF,
	INSIDE_RAINBOW
};

enum NTP_state:byte {
	NTP_DONE,
	NTP_WAITING_FOR_RESPONSE,
	NTP_SEND_REQUEST
};


//////////////// DIGITAL PINS
// #define serialRX 0     // Used for serial debugging
// #define serialTX 1     // Used for serial debugging
#define RADIO_INT_PIN 2   // Digital || INT_1=0 future change to migrate this to IRQ for RF24
#define RGB_STRIP_PIN 4   // Digital
#define ZONE_PIN 5        // Digital || PWM
#define HAS_CONTROL_BOX_PIN 6     // Digital || PWM
#define RADIO_CHIP_ENABLE_PIN 7   // Digital
#define RADIO_CHIP_SELECT_PIN 8   // Digital || CLOCK_0
#define BIG_LED_PIN 9     // Digital || PWM
#define TIMER1_PIN 10     // Digital || PWM		[note; moved to timer1 as is 16bit and more useful for LED timing]
#define RADIO_MOSI_PIN 11 // Digital || MOSI
#define RADIO_MISO_PIN 12 // Digital || MISO
#define RADIO_SCK_PIN 13  // Digital || SCK
/// http://playground.arduino.cc/Code/Timer1
/// http://www.instructables.com/id/Arduino-Timer-Interrupts/?ALLSTEPS
//// Inputs
#define INPUT1_PIN 3      // Digital || PWM || INT_1
#define INPUT2_PIN 10     // Shared with Timer1 so can only be used as digital and not PWM
#define INPUT_A5_PIN A5   // Analog
#define INPUT_A4_PIN A4   // Analog
#define INPUT_A1_PIN A1   // Analog; pin 15, use as digital

//////////////// ANALOG PINS as Analog
#define INPUT_A0_PIN A0   // Analog, used as digital
#define INPUT_A2_PIN A2   // Analog; pin 16
#define INPUT_A3_PIN A3   // Analog; pin 17

#endif // __ILLUMINUSDEFS_H__
