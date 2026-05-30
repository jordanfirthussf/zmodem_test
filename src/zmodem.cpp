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


// Dylan (monte_carlo_ecm, bitflipper, etc.) - This function was added because I found
// that SERIAL_TX_BUFFER_SIZE was getting overrun at higher baud rates.  This modified
// Serial.print() function ensures we are not overrunning the buffer by flushing if
// it gets more than half full.

// size_t DSERIAL_PRINT(const __FlashStringHelper *ifsh)
// {
//   PGM_P p = reinterpret_cast<PGM_P>(ifsh);
//   size_t n = 0;
//   while (1) {
//     \
//     unsigned char c = pgm_read_byte(p++);
//     if (c == 0) break;
//     if (DSERIAL_AVAILABLE_FOR_WRITE() > SERIAL_TX_BUFFER_SIZE / 2) ASERIAL.flush();
//     if (DSERIAL_WRITE(c)) n++;
//     else break;
//   }
//   return n;
// }

// #define DSERIAL_PRINTLN(_p) ({ DSERIAL_PRINT(_p); DSERIAL_WRITE("\r\n"); })
