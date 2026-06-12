#pragma once

#include <SdFat.h>
#include "zmodem.h"


class ZModemSend: public virtual ZModem {
    public:
    ZModemSend();
    static void zmodem_send_file(FsFile &file);

private:

    static int wctxpn(const char *name);
    static int wcs(const char *oname);
    static int zsendfile(char *buf, int blen);
    static void sendzrqinit();
    static void saybibi();
    static int getinsync(int flag);
    static int filbuf(char *buf, int count);
    static int zfilbuf();
    static int zsendfdata();
    static void zsbhdr(int type, char *hdr);
    static int wcputsec(char *buf, int sectnum, int cseclen);


    // not implemented
    // static int sendzsinit(void);
    // static int zsendcmd(char *buf, int blen);


protected:


};

extern int Filesleft;
extern long Totalleft;

extern FsFile fout;
