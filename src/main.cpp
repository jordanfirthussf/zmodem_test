#include "Arduino.h"
//#include <avr/pgmspace.h>

// #define SERIAL_TX_BUFFER_SIZE 128

#include <SdFat.h>

SdFs sd;

#include <SPI.h>

#include "zmodem_config.h"
#include "zmodem_sz.h"
///// SD Card CS pin
  #if defined(ARDUINO_SEEED_XIAO_ESP32S3) || defined(ARDUINO_XIAO_ESP32S3)
  #define SD_SEL 21 // XIAO ESP32S3
  #elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER) || defined(ARDUINO_ARCH_RP2040)
  #define SD_SEL 23 // Feather RP2040 adalogger
  #elif defined(ARDUINO_ESP32_THING_PLUS_C)
  #define SD_SEL 5 // SparkFun ESP32 Thing Plus C
  #else
  #define SD_SEL 21 // Default
  #endif


// #define TXBSIZE 1024 // tx buffer size (default 1024); must be a power of 2

#define error(s) sd.errorHalt(s)

#if defined(ARDUINO_ARCH_RP2040)
  #define SD_CONFIG SdSpiConfig(SD_SEL, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI1)
#else
  #define SD_CONFIG SdSpiConfig(SD_SEL, DEDICATED_SPI, SD_SCK_MHZ(16))
#endif

void setup() {

  
  ZSERIAL_BEGIN(115200);
  ZSERIAL_SET_TIMEOUT(TYPICAL_SERIAL_TIMEOUT);
  delay(2000);

  ZSERIAL_PRINTLN("beginning");

  DSERIAL_BEGIN(9600);
  DSERIAL_SET_TIMEOUT(1200);

  ASERIAL_PRINTLN(Progname);
  ASERIAL_PRINT(F("Transfer rate: "));
  ASERIAL_PRINTLN(ZMODEM_SPEED);

  ASERIAL_PRINTLN(F("Regular SD Card\n"));

  // Initialize the SdCard.
  ASERIAL_PRINTLN(F("About to initialize SdCard"));
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&ASERIAL);
  }
  // depending upon your SdCard environment, SPI_HALF_SPEED may work better.
  ASERIAL_PRINTLN(F("About to change directory"));
  // if(!sd.chdir((const char *)("/"))) sd.errorHalt(F("sd.chdir"));
  ASERIAL_PRINTLN(F("SdCard setup complete"));

  FsFile fout  = sd.open("test.txt", O_WRONLY | O_CREAT | O_TRUNC);
  fout.println("testing");
  fout.close();

  ZModemSend::begin(Serial);

  delay(5000);

  // char cmd[14] = "flightA.csv";
  // char cmd[9] = "test.txt";
  // FsFile file = sd.open("test.txt", O_READ);

  fout  = sd.open("test.txt", O_READ);
  ZModemSend::zmodem_send_file(fout);

  fout.close();

}

void loop() {

}
