#include "zmodem.h"

String help(void)
{
  String print_line;

  print_line += "\n";
  print_line += Progname;
  print_line += " - Transfer rate: ";
  print_line += ZMODEM_SPEED;
  print_line += "\nAvailable Commands: \n";
  print_line += "HELP     - Print this list of commands \n";
  print_line += "DIR      - List files in current working directory - alternate LS \n";
  print_line += "PWD      - Print current working directory \n";
  print_line += "CD       - Change current working directory \n";
#ifdef   ARDUINO_SMALL_MEMORY_INCLUDE_FILE_MGR
  print_line += "DEL file - Delete file - alternate RM \n";
  print_line += "MD  dir  - Create dir - alternate MKDIR \n";
  print_line += "RD  dir  - Delete dir - alternate RMDIR \n";
#endif
#ifdef ARDUINO_SMALL_MEMORY_INCLUDE_RZ
  print_line += "RZ       - Receive a file from terminal to Arduino (Hyperterminal sends this \n";
  print_line += "              automatically when you select Transfer->Send File...) \n";
#endif
  print_line += "\n";
  return print_line;
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

String directory_listing() { // command: ls
  String print_line;

  print_line += "\nDirectory Listing: \n";
  // ASERIAL.println(F("Directory Listing:"));

  dir->openCwd();
  dir->rewindDirectory();

  while (file->openNext(dir)) {
    // read next directory entry in current working directory

    // format file name
    file->getName(file_name, 64);

    print_line += file_name;
    for (uint8_t i = 0; i < 64 - strlen(file_name); ++i) {
      print_line+=" ";
    }
    if (!(file->isDir())) {
      // here using file_name to store char file_size, which makes no sense
      ultoa(file->fileSize(), file_name, 10);
      print_line+=file_name;
    }
    else {
      print_line+="DIR";
    } // end of if/else
    // ASERIAL.flush();
    file->close();
    print_line += "\n";
  } // end  of while (one line for each file
  print_line+= "End of Directory";

  return print_line;
}

String print_working_directory() {
  String print_line;
  dir->openCwd();
  dir->getName(file_name, 256);
  dir->close();
  print_line += "\n";
  print_line += "Current Directory is: ";
  print_line += file_name;
  print_line += "\n";
  return print_line;
}

String change_directory(char* param) {
  String print_line;

  if(!sd.chdir(param)) {
    print_line += "\nDirectory ";
    print_line += param;
    print_line += "not found\n";
  } else {
    print_line += "Current directory changed to ";
    print_line += param;
  }
  return print_line;
}

String remove_file(char* param) {
  String print_line;

  if (!sd.remove(param)) {
    print_line += "\nFailed to delete file ";
    print_line += param;
  } else {
    print_line += "File ";
    print_line += param;
    print_line += "deleted\n";
  }
  return print_line;
}

String mkdir(char* param) {
  String print_line;
  if (!sd.mkdir(param, true)) {
    print_line += "\nFailed to create directory ";
    print_line += param;
  } else {
    print_line += "\nDirectory ";
    print_line += param;
    print_line += " created\n";
  }
}

String remove_directory(char* param) {
  String print_line;
  if (!sd.rmdir(param)) {
    print_line += "\nFailed to remove directoy ";
    print_line += param;
  } else {
    print_line += "\nDirectory ";
    print_line += param;
    print_line += " removed\n";
  }
  return print_line;
}

String zmodem_send_file(char* param) {
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
      return "No files found to send";
    }
  } else if (!fout.open(param, O_READ)) {
    return "file open failed";
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
    return "file transfer complete";
  }
}

String zmodem_receive_file() {

  String print_line;
    // ASERIAL.println(F("Receiving file..."));
  if (wcreceive(0, 0)) {
    print_line += "zmodem transfer failed";
  } else {
    print_line += "zmodem transfer succeeded";
  }
  //fout.flush();
  fout.sync();
  fout.close();
  return print_line;
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
