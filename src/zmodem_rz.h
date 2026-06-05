#pragma once

#include <SdFat.h>
#include "zmodem.h"

void zmodem_receive_file();

// moved to ZModem class:
// int wcreceive(int argc, char** argp);
// int wcrx();
// void bibi(int n);
// int procheader(const char* name);
