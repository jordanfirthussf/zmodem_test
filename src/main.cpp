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

#ifdef SFMP3_SHIELD
#include <SFEMP3Shield.h>
#include <SFEMP3ShieldConfig.h>
#include <SFEMP3Shieldmainpage.h>

SFEMP3Shield mp3;
#endif

#define error(s) sd.errorHalt(s)

extern int Filesleft;
extern long Totalleft;

extern SdFile fout;
// extern HardwareSerial DSERIAL;

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

void help(void)
{
  ASERIAL.print(Progname);
  ASERIAL.print(F(" - Transfer rate: ")); ASERIAL.flush();
    ASERIAL.println(ZMODEM_SPEED); ASERIAL.flush();
  ASERIAL.println(F("Available Commands:")); ASERIAL.flush();
  ASERIAL.println(F("HELP     - Print this list of commands")); ASERIAL.flush();
  ASERIAL.println(F("DIR      - List files in current working directory - alternate LS")); ASERIAL.flush();
  ASERIAL.println(F("PWD      - Print current working directory")); ASERIAL.flush();
  ASERIAL.println(F("CD       - Change current working directory")); ASERIAL.flush();
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_FILE_MGR
  ASERIAL.println(F("DEL file - Delete file - alternate RM")); ASERIAL.flush();
  ASERIAL.println(F("MD  dir  - Create dir - alternate MKDIR")); ASERIAL.flush();
  ASERIAL.println(F("RD  dir  - Delete dir - alternate RMDIR")); ASERIAL.flush();
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_SZ
  ASERIAL.println(F("SZ  file - Send file from Arduino to terminal (* = all files)")); ASERIAL.flush();
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_RZ  
  ASERIAL.println(F("RZ       - Receive a file from terminal to Arduino (Hyperterminal sends this")); ASERIAL.flush();
  ASERIAL.println(F("              automatically when you select Transfer->Send File...)")); ASERIAL.flush();
#endif
}

SdFile fout;
//dir_t *dir ;

// Dylan (monte_carlo_ecm, bitflipper, etc.) - The way I made this sketch in any way operate on
// a board with only 2K of RAM is to borrow the SZ/RZ buffer for the buffers needed by the main
// loop(), in particular the file name parameter and the SdFat directory entry.  This is very
// unorthodox, but now it works on an Uno.  Please see notes in zmodem_config.h for limitations

#define name (&oneKbuf[512])
#define dir ((FsFile *)&oneKbuf[256])
#define file ((FsFile*)&oneKbuf[768])

void setup() {
  
  ZSERIAL.begin(9600);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);

  DSERIAL_BEGIN(9600);
  DSERIAL_SET_TIMEOUT(1200);

  ASERIAL.println(Progname);
  ASERIAL.print(F("Transfer rate: "));
  ASERIAL.println(ZMODEM_SPEED);

#ifdef SFMP3_SHIELD
  ASERIAL.println(F("SparkFun MP3!\n"));
#else
  ASERIAL.println(F("Regular SD Card\n"));
#endif

  //Initialize the SdCard.
ASERIAL.println(F("About to initialize SdCard"));
  if(!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt(&ASERIAL);
  // depending upon your SdCard environment, SPI_HALF_SPEED may work better.
ASERIAL.println(F("About to change directory"));
  if(!sd.chdir((const char *)("/"))) sd.errorHalt(F("sd.chdir"));
ASERIAL.println(F("SdCard setup complete"));

  #ifdef SFMP3_SHIELD
  mp3.begin();
  #endif

  help();
 
}

int count_files(int *file_count, long *byte_count)
{
  *file_count = 0;
  *byte_count = 0;

  dir->openCwd();
  dir->rewindDirectory();

  while (file->openNext(dir)) {
    // read next directory entry in current working directory

    if (!file->isDir()) {
      *file_count = *file_count + 1;
      *byte_count = *byte_count + file->fileSize();
    }
     
    file->close();
  }
  dir->close();
  return 0;
}

void loop(void) {
  char *cmd = oneKbuf;
  char *param;

  *cmd = 0;
  while (ASERIAL.available()) ASERIAL.read();
  
  char c = 0;
  while(1) {
    if (ASERIAL.available() > 0) {
      c = ASERIAL.read();
      if ((c == 8 or c == 127) && strlen(cmd) > 0) cmd[strlen(cmd)-1] = 0;
      if (c == '\n' || c == '\r') break;
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
   
  param = strchr(cmd, 32);
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
    ASERIAL.println(F("Directory Listing:"));

    dir->openCwd();
    dir->rewindDirectory();

    while (file->openNext(dir)) {
      // read next directory entry in current working directory
  
      // format file name
      file->getName(name, 64);

      ASERIAL.flush(); ASERIAL.print(name); ASERIAL.flush();
      for (uint8_t i = 0; i < 64 - strlen(name); ++i) ASERIAL.print(F(" "));
      if (!(file->isDir())) {
        ultoa(file->fileSize(), name, 10);
        ASERIAL.flush(); ASERIAL.println(name); ASERIAL.flush();
      } else {
        ASERIAL.println(F("DIR"));
      }
      ASERIAL.flush();
      file->close();
    }
    ASERIAL.println(F("End of Directory"));
 
  } else if (!strcmp_P(cmd, PSTR("PWD"))) {
    dir->openCwd();
    dir->getName(name, 256);
    dir->close();
    ASERIAL.print(F("Current working directory is "));
    ASERIAL.flush(); ASERIAL.println(name); ASERIAL.flush();
  
  } else if (!strcmp_P(cmd, PSTR("CD"))) {
    if(!sd.chdir(param)) {
      ASERIAL.print(F("Directory "));
      ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
      ASERIAL.println(F(" not found"));
    } else {
      ASERIAL.print(F("Current directory changed to "));
      ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
    }
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_FILE_MGR
  } else if (!strcmp_P(cmd, PSTR("DEL")) || !strcmp_P(cmd, PSTR("RM"))) {
    if (!sd.remove(param)) {
      ASERIAL.print(F("Failed to delete file "));
      ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
    } else {
      ASERIAL.print(F("File "));
      ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
      ASERIAL.println(F(" deleted"));
    }
  } else if (!strcmp_P(cmd, PSTR("MD")) || !strcmp_P(cmd, PSTR("MKDIR"))) {
    if (!sd.mkdir(param, true)) {
      ASERIAL.print(F("Failed to create directory "));
      ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
    } else {
      ASERIAL.print(F("Directory "));
      ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
      ASERIAL.println(F(" created"));
    }
  } else if (!strcmp_P(cmd, PSTR("RD")) || !strcmp_P(cmd, PSTR("RMDIR"))) {
    if (!sd.rmdir(param)) {
      ASERIAL.print(F("Failed to remove directory "));
      ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
    } else {
      ASERIAL.print(F("Directory "));
      ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
      ASERIAL.println(F(" removed"));
    }
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_SZ
  } else if (!strcmp_P(cmd, PSTR("SZ"))) {
//    Filcnt = 0;
    if (!strcmp_P(param, PSTR("*"))) {
      count_files(&Filesleft, &Totalleft);

      if (Filesleft > 0) {
        ZSERIAL.print(F("rz\n"));
        ZSERIAL.flush();

        sendzrqinit();
        delay(200);

        // Cannot use the "shared 1K memory" block with the latest SDFat because the file transfer will corrupt the directory object.
        FsFile dirsz;

        dirsz.openCwd();
        dirsz.rewindDirectory();
        
        while (fout.openNext(&dirsz)) {
          // read next directory entry in current working directory
                           
          // open file
          fout.getName(name, 256);
          //if (!fout.open(name, O_READ)) error(F("file.open failed"));
        
          //else 
          if (!fout.isDir()) {

            if (wcs(name) == ERROR) {
              delay(500);
              fout.close();
              break;
            }
            else delay(500);
          }
          fout.close();

        }

        dirsz.close();

        saybibi();
      } else {
        ASERIAL.println(F("No files found to send"));
      }
    } else if (!fout.open(param, O_READ)) {
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
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_RZ
  } else if (!strcmp_P(cmd, PSTR("RZ"))) {
//    ASERIAL.println(F("Receiving file..."));
    if (wcreceive(0, 0)) {
      ASERIAL.println(F("zmodem transfer failed"));
    } else {
      ASERIAL.println(F("zmodem transfer successful"));
    }
    //fout.flush();
    fout.sync();
    fout.close();
#endif
  }
}



