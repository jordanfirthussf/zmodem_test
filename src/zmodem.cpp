#include "zmodem.h"

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

void directory_listing() {
  ASERIAL.println(F("Directory Listing:"));

  dir->openCwd();
  dir->rewindDirectory();

  while (file->openNext(dir)) {
    // read next directory entry in current working directory

    // format file name
    file->getName(file_name, 64);

    ASERIAL.flush(); ASERIAL.print(file_name); ASERIAL.flush();
    for (uint8_t i = 0; i < 64 - strlen(file_name); ++i) ASERIAL.print(F(" "));
    if (!(file->isDir())) {
      ultoa(file->fileSize(), file_name, 10);
      ASERIAL.flush(); ASERIAL.println(file_name); ASERIAL.flush();
    } else {
      ASERIAL.println(F("DIR"));
    }
    ASERIAL.flush();
    file->close();
  }
  ASERIAL.println(F("End of Directory"));
}

void print_working_directory() {
  dir->openCwd();
  dir->getName(file_name, 256);
  dir->close();
  ASERIAL.print(F("Current working directory is "));
  ASERIAL.flush(); ASERIAL.println(file_name); ASERIAL.flush();
}

void change_directory(char* param) {
  if(!sd.chdir(param)) {
    ASERIAL.print(F("Directory "));
    ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
    ASERIAL.println(F(" not found"));
  } else {
    ASERIAL.print(F("Current directory changed to "));
    ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
  }
}

void remove_file(char* param) {
  if (!sd.remove(param)) {
    ASERIAL.print(F("Failed to delete file "));
    ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
  } else {
    ASERIAL.print(F("File "));
    ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
    ASERIAL.println(F(" deleted"));
  }
}

void mkdir(char* param) {
  if (!sd.mkdir(param, true)) {
    ASERIAL.print(F("Failed to create directory "));
    ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
  } else {
    ASERIAL.print(F("Directory "));
    ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
    ASERIAL.println(F(" created"));
  }
}

void remove_directory(char* param) {
  if (!sd.rmdir(param)) {
    ASERIAL.print(F("Failed to remove directory "));
    ASERIAL.flush(); ASERIAL.println(param); ASERIAL.flush();
  } else {
    ASERIAL.print(F("Directory "));
    ASERIAL.flush(); ASERIAL.print(param); ASERIAL.flush();
    ASERIAL.println(F(" removed"));
  }
}

void zmodem_send_file(char* param) {
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
        fout.getName(file_name, 256);
        //if (!fout.open(file_name, O_READ)) error(F("file.open failed"));

        //else
        if (!fout.isDir()) {

          if (wcs(file_name) == ERROR) {
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
