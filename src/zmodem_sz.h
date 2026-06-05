#pragma once

#include <SdFat.h>
#include "zmodem.h"

ZModem::ZModem() {}

void zmodem_send_file(char* param);

// moved to ZModem class
// int wcs(const char* oname);
// int wctxpn(const char* name);
// void sendzrqinit();
// void ZModem::saybibi();

extern int Filesleft;
extern long Totalleft;

extern FsFile fout;
