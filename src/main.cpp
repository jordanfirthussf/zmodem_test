#include "Arduino.h"
//#include <avr/pgmspace.h>

// #define SERIAL_TX_BUFFER_SIZE 128

#include <SPI.h>

#include "zmodem_config.h"
#include "zmodem_fixes.h"

#include "zmodem.h"
#include "zmodem_zm.h"
#include <SdFat.h>
//#include <SdFatUtil.h>

SdFs sd;


#define error(s) sd.errorHalt(s)

SdFile fout;

void setup() {
  
  ZSERIAL.begin(9600);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);

  DSERIAL_BEGIN(9600);
  DSERIAL_SET_TIMEOUT(1200);

  ASERIAL.println(Progname);
  ASERIAL.print(F("Transfer rate: "));
  ASERIAL.println(ZMODEM_SPEED);

  ASERIAL.println(F("Regular SD Card\n"));

  //Initialize the SdCard.
ASERIAL.println(F("About to initialize SdCard"));
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) {sd.initErrorHalt(&ASERIAL);}
  // depending upon your SdCard environment, SPI_HALF_SPEED may work better.
ASERIAL.println(F("About to change directory"));
  if(!sd.chdir((const char *)("/"))) sd.errorHalt(F("sd.chdir"));
ASERIAL.println(F("SdCard setup complete"));

  help();
}

void loop() {
  char *cmd = oneKbuf;

  *cmd = 0;
  while (ASERIAL.available()) ASERIAL.read();
  
  char c = 0;
  while(true) {
    if (ASERIAL.available() > 0) {
      c = ASERIAL.read();
      if ((c == 8 or c == 127) && strlen(cmd) > 0){ cmd[strlen(cmd)-1] = 0;}
      if (c == '\n' || c == '\r') {break;}
      ASERIAL.write(c);
      if (c != 8 && c != 127) strncat(cmd, &c, 1);
    } else {
      // Dylan (monte_carlo_ecm, bitflipper, etc.) -
      // This delay is required because I found that if I hard loop with DSERIAL.available,
      // in certain circumstances the Arduino never sees a new character.  Various forum posts
      // seem to confirm that a short delay is required when using this style of reading
      // from Serial
      delay(20);
    }
  }
   
  char* param = strchr(cmd, 32);
  if (param > 0) {
    *param = 0;
    param = param + 1;
  } else {
    param = &cmd[strlen(cmd)];
  }

  strupr(cmd);
  // DSERIAL_PRINTLN();
  // DSERIAL_PRINTLN(command);
  // DSERIAL_PRINTLN(parameter);

  if (!strcmp_P(cmd, PSTR("HELP"))) {
    
    help();
    
  } else if (!strcmp_P(cmd, PSTR("DIR")) || !strcmp_P(cmd, PSTR("LS"))) {
    directory_listing();
 
  }

  else if (!strcmp_P(cmd, PSTR("PWD"))) {
    print_working_directory();
  
  } else if (!strcmp_P(cmd, PSTR("CD"))) {
    change_directory(param);

#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_FILE_MGR
  } else if (!strcmp_P(cmd, PSTR("DEL")) || !strcmp_P(cmd, PSTR("RM"))) {
    remove_file(param);
  } else if (!strcmp_P(cmd, PSTR("MD")) || !strcmp_P(cmd, PSTR("MKDIR"))) {
    mkdir(param);
  } else if (!strcmp_P(cmd, PSTR("RD")) || !strcmp_P(cmd, PSTR("RMDIR"))) {
    remove_directory(param);
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_SZ
  } else if (!strcmp_P(cmd, PSTR("SZ"))) {
//    Filcnt = 0;
    zmodem_send_file(param);
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_RZ
  } else if (!strcmp_P(cmd, PSTR("RZ"))) {
    zmodem_receive_file();
#endif
  }

}
