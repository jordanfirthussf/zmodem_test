#include "zmodem.h"

void zmodem_send_file(char* param) {
    if (!fout.open(param, O_READ)) {
    ASERIAL.println(F("file.open failed"));
  } else {
    // Start the ZMODEM transfer
    Filesleft = 1;
    Totalleft = fout.fileSize();
    ZSERIAL.print(F("rz\n"));
    ZSERIAL.flush();
    sendzrqinit();
    delay(200);
    wcs(param);
    saybibi();
    fout.close();
  }
}
