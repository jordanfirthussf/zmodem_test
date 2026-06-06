#pragma once

#include <SdFat.h>
#include "zmodem.h"

class ZModemSend: public ZModem {
    public:
    ZModemSend();

    void send_file(char* param);
    static int wctxpn(const char *name);
    static int wcs(const char *oname);
    static int zsendfile(char *buf, int blen);
    static void sendzrqinit();
    static int wctx(long flen);
    static void saybibi();
    static int getinsync(int flag);
    static int filbuf(char *buf, int count);
    static int zfilbuf();
    static int zsendfdata();

    static void zmodem_send_file(char* param);
};

// void zmodem_send_file(char* param);

// moved to ZModem class
// int wcs(const char* oname);
// int wctxpn(const char* name);
// void sendzrqinit();
// void ZModem::saybibi();

extern int Filesleft;
extern long Totalleft;

extern FsFile fout;
