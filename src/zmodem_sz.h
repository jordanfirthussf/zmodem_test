#pragma once

#include <SdFat.h>
#include "zmodem.h"

void zmodem_send_file(char* param);

int wcs(const char* oname);
int wctxpn(const char* name);
void sendzrqinit();
void saybibi();

extern int Filesleft;
extern long Totalleft;
