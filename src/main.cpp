#include "Arduino.h"
//#include <avr/pgmspace.h>

// #define SERIAL_TX_BUFFER_SIZE 128

#include <SdFat.h>

SdFs sd;
FsFile fout;

#include <SPI.h>

#include "zmodem_config.h"
#include "zmodem_sz.h"
// #include "zmodem_rz.h"
#define SD_CS_PIN SD_SEL
// SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI1);

// #define TXBSIZE 1024 // tx buffer size (default 1024); must be a power of 2

#define error(s) sd.errorHalt(s)

#if defined(ARDUINO_ARCH_RP2040)
  #define SD_CONFIG SdSpiConfig(SD_SEL, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI1)
#else
  #define SD_CONFIG SdSpiConfig(SD_SEL, DEDICATED_SPI, SD_SCK_MHZ(16))
#endif


ZModemSend zModemSend;

void setup() {

  
  ZSERIAL.begin(115200);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);
  delay(2000);

  ZSERIAL.println("beginning");

  DSERIAL_BEGIN(9600);
  DSERIAL_SET_TIMEOUT(1200);

  ASERIAL.println(Progname);
  ASERIAL.print(F("Transfer rate: "));
  ASERIAL.println(ZMODEM_SPEED);

  ASERIAL.println(F("Regular SD Card\n"));

  // Initialize the SdCard.
  ASERIAL.println(F("About to initialize SdCard"));
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&ASERIAL);
  }
  // depending upon your SdCard environment, SPI_HALF_SPEED may work better.
  ASERIAL.println(F("About to change directory"));
  // if(!sd.chdir((const char *)("/"))) sd.errorHalt(F("sd.chdir"));
  ASERIAL.println(F("SdCard setup complete"));

  // fout  = sd.open("test.txt", FILE_WRITE);
  // myFile.println("testing");
  // myFile.close();

  zModemSend.begin(ZSERIAL);

  delay(5000);

  char cmd[14] = "flightA.csv";
  fout = sd.open(cmd, O_READ);

  ZModemSend::zmodem_send_file(fout);
}

void loop() {

}
