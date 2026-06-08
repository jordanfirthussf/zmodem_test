#pragma once

#include <SdFat.h>
#include "zmodem.h"


class ZModemSend: public ZModem {
    public:
    ZModemSend();

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
    static void zsbhdr(int type, char *hdr);
    static int wcputsec(char *buf, int sectnum, int cseclen);
    static void zmodem_send_file(char* param);
    // static void bibi(int n);

};

extern int Filesleft;
extern long Totalleft;

extern FsFile fout;
