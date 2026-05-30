#pragma once

#define Progname F("Arduino ZModem V3.0")

#define SD_SEL 5 // Thing Plus C
// #define SD_SEL 21 // XIAO ESP32S3

/*
 * can function with
 * - single serial port: combined ZERIAL (control and data)
 * - OR
 * - 2 serial ports: separate DSERIAL (control and debug) AND ZERIAL (data)
 *
 * ZSERIAL (data) will always go over the data channel
 * DSERIAL (debug messages) will go over the debug channel or disappear
 * ASERIAL (control) will go over the debug channel if available, otherwise over the data channel
 **/


// #define SEPARATE_DEBUG_SERIAL 1 // comment out if DSERIAL and ZSERIAL are the same

	#ifdef SEPARATE_DEBUG_SERIAL
		#include <HardwareSerial.h>
		inline HardwareSerial DSERIAL(2);

		#define ASERIAL DSERIAL

		#define DSERIAL_BEGIN(...) DSERIAL.begin(__VA_ARGS__);
		#define DSERIAL_PRINT(...) DSERIAL.print((String)__VA_ARGS__);
		#define DSERIAL_PRINTLN(...) DSERIAL.println((String)__VA_ARGS__);
		#define DSERIAL_PRINTF(...) DSERIAL.printf((String)__VA_ARGS__)
		#define DSERIAL_WRITE(...) DSERIAL.write(__VA_ARGS__)
		#define DSERIAL_AVAILABLE_FOR_WRITE(...) DSERIAL.availableForWrite(__VA_ARGS__)
		#define DSERIAL_FLUSH(...) DSERIAL.flush(__VA_ARGS__)
		#define DSERIAL_SET_TIMEOUT(...) DSERIAL.setTimeout(__VA_ARGS__)
		#define DSERIAL_READ(...) DSERIAL.read(__VA_ARGS__)
		#define DSERIAL_AVAILABLE(...) DSERIAL.available(__VA_ARGS__)
	#else // no separate serial—remove all references to DSERIAL
		#define ASERIAL ZSERIAL

		#define DSERIAL_BEGIN(...)
		#define DSERIAL_PRINT(...)
		#define DSERIAL_PRINTLN(...)
		#define DSERIAL_PRINTF(...)
		#define DSERIAL_WRITE(...)
		#define DSERIAL_AVAILABLE_FOR_WRITE(...)
		#define DSERIAL_FLUSH(...)
		#define DSERIAL_SET_TIMEOUT(...)
		#define DSERIAL_READ(...)
		#define DSERIAL_AVAILABLE(...)
	#endif

#define ZSERIAL Serial
#define ZMODEM_SPEED 9600 // adjust for your board and needs

#include "Arduino.h"



