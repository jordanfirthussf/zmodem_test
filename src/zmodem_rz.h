#pragma once

#include <SdFat.h>
#include "zmodem.h"

void zmodem_receive_file();

static void ackbibi();
static int wcreceive(int argc, char **argp);
static int wcrxpn(char *rpn);
static int wcgetsec(char *rxbuf, int maxtime);
static int procheader(const char *name);
static void canit();
static int rzfiles();
static int rzfile();

static int zrbhdr(char *hdr);
static int zrbhdr32(char *hdr);

// moved to ZModem class:
// int wcreceive(int argc, char** argp);
// int wcrx();
// void bibi(int n);
// int procheader(const char* name);
