#include "zmodem_config.h"
#include "zmodem.h"



#include "zmodem_sz.h"

int Filesleft;
long Totalleft;

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

#include "zmodem_rz.h"

void zmodem_receive_file() {
  ASERIAL.println(F("Receiving file..."));
  if (wcreceive(0, 0)) {
    ASERIAL.println(F("zmodem transfer failed"));
  } else {
    ASERIAL.println(F("zmodem transfer successful"));
  }
  //fout.flush();
  fout.sync();
  fout.close();
}
