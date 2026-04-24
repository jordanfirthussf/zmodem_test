/*
	Keep everything for ANSI prototypes.
From: http://stackoverflow.com/questions/2607853/why-prototype-is-used-header-files
*/

#pragma once

#define SERIAL_TX_BUFFER_SIZE 128

////////////////////////////////////////////////////////


#define _PROTOTYPE(function, params) function params

#include <SdFat.h>
#include "zmodem.h"
#include "zmodem_zm.h"

extern SdFat sd;


// Dylan (monte_carlo_ecm, bitflipper, etc.) - changed serial read/write to macros to try to squeeze 
// out higher speed

#define READCHECK
#define TYPICAL_SERIAL_TIMEOUT 1200

#define readline(timeout) ({ byte _c; ZSERIAL.readBytes(&_c, 1) > 0 ? _c : TIMEOUT; })
int zdlread2(int);
#define zdlread(void) ({ int _z; ((_z = readline(Rxtimeout)) & 0140) ? _z : zdlread2(_z); })

template <typename T>
void zsendline(T z) {
	if (z & 0140) {
		sendline(z);
	}
	else {
		zsendline2(z);
	}
}

inline void sendline(int c){
	// Check if buffer is more than half full
	if (ZSERIAL.availableForWrite() < (SERIAL_TX_BUFFER_SIZE / 2)) {
		ZSERIAL.flush(); // Wait for outgoing data to complete
	}
	ZSERIAL.write(c);
}

inline void sendline(char c){
	// Check if buffer is more than half full
	if (ZSERIAL.availableForWrite() < (SERIAL_TX_BUFFER_SIZE / 2)) {
		ZSERIAL.flush(); // Wait for outgoing data to complete
	}
	ZSERIAL.write(c);
}

void sendzrqinit(void);
int wctxpn(const char *name);
#define ARDUINO_RECV
//int wcrx(void);

int wcreceive(int argc, char **argp);

extern int Filcnt;

#define register int

// If this is not defined the default is 1024.
// It must be a power of 2

#ifdef ARDUINO_SMALL_MEMORY
#define TXBSIZE 1024
#else
#define TXBSIZE 1024
#endif

#define sleep(x) delay((x)*1000L)
#define signal(x,y)

// Handle the calls to exit - one has SS_NORMAL
#define SS_NORMAL 0
#define exit(n)

// For now, evaluate it to zero so that it does not
// enter the "if" statement's clause
#define setjmp(...)

#define printf(s, ... ) DSERIAL_PRINTLN(s);
#define fprintf(...)

// fseek(in, Rxpos, 0)
//#define fseek(fd, pos, rel) sdfile->seekSet(pos)
//#define fclose(c)

// ignore calls to mode() function in rbsb.cpp
#define mode(a)

#define sendbrk()

//extern int Fromcu;
void purgeline(void);

#ifndef UNSL
#define UNSL unsigned
#endif

void flushmo(void);


