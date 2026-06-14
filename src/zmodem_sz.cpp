
int Filesleft;
long int Totalleft;

#include "zmodem_config.h"
#include "zmodem_zm.h"
#include "zmodem_sz.h"

#include "zmodem.h"

#include <cstdio>
 
FsFile *ZModemSend::_fout = nullptr;
 


#define PATHLEN 64

#define HOWMANY 2

#define Txwindow 0      /* Control the size of the transmitted window */
#define Txwspac 0       /* Spacing between zcrcq requests */
unsigned Txwcnt;        /* Counter used to space ack requests */
#define Lrxpos rxbytes
extern long Lrxpos;            /* Receiver's last reported offset */


/*
 * Attention string to be executed by receiver to interrupt streaming data
 *  when an error is detected.  A pause (0336) may be needed before the
 *  ^C (03) or after it.
 */
#ifdef READCHECK
char Myattn[] = { 
  0 };
#else
#ifdef USG
char Myattn[] = { 
  03, 0336, 0 };
#else
char Myattn[] = { 
  0 };
#endif
#endif

#ifdef BADSEEK
#define Canseek 0        /* 1: Can seek 0: only rewind -1: neither (pipe) */
#ifndef TXBSIZE
#define TXBSIZE 16384           /* Must be power of two, < MAXINT */
#endif
#else
#define Canseek 1        /* 1: Can seek 0: only rewind -1: neither (pipe) */
#endif

#ifdef TXBSIZE
#define TXBMASK (TXBSIZE-1)
#define Txb oneKbuf              /* Circular buffer for file reads */
#define txbuf Txb              /* Pointer to current file segment */
// Force the user to specify a buffer size
//#else
//char txbuf[1024];
#endif

//long vpos = 0;                  /* Number of bytes read from file */

#define Modem2 0           /* XMODEM Protocol - don't send pathnames */

#define Ascii 0            /* Add CR's for brain damaged programs */
#define Fullname 0         /* transmit full pathname */
#define Unlinkafter 0      /* Unlink file after it is sent */
#define Dottoslash 0       /* Change foo.bar.baz to foo/bar/baz */

int errcnt=0;           /* number of files unreadable */
#define blklen Blklen
extern int blklen;         /* length of transmitted records */
#define Optiong 0            /* Let it rip no wait for sector ACK's */

int Totsecs;            /* total number of sectors this file */
//int Filcnt=0;           /* count of number of files unreadable */
uint8_t Lfseen=0;
#define Rxbuflen 16384      /* Receiver's max buffer length */
int Tframlen = 0;       /* Override for tx frame length */
#define blkopt 0           /* Override value for zmodem blklen */
//int Rxflags = 0;
#define bytcnt Bytesleft
extern long bytcnt;
//int Wantfcs32 = TRUE;   /* want to send 32 bit FCS */
// #define Lzconv 0    /* Local ZMODEM file conversion request */
#define Lzconv ZModem::ZF0_ZCBIN   /* Local ZMODEM file conversion request */

#define  Lskipnocor 0
#define Lztrans 0

//int Command;            /* Send a command, then exit. */
//char *Cmdstr;           /* Pointer to the command string */
//int Cmdtries = 11;
//int Cmdack1;            /* Rx ACKs command, then do it */
//int Exitcode = 0;
#define Test 0               /* 1= Force receiver to send Attn, etc with qbf. */
/* 2= Character transparency test */

//char qbf[] = "The quick brown fox jumped over the lazy dog's back 1234567890\r\n";

long Lastsync;          /* Last offset to which we got a ZRPOS */
uint8_t Beenhereb4;         /* How many times we've been ZRPOS'd same place */

ZModemSend::ZModemSend() = default;

/// @brief Initiates the transfer of a specific named file.
/// @param oname
/// @return 0 (OK)
/// @return -1 (ERROR)
int ZModemSend::wcs(const char *oname)
{

  Eofseen = 0;
  switch (wctxpn(oname)) {
   case ERROR:
    DSERIAL_PRINT("error");
     return ERROR;
    case OK:
    case ZSKIP:
    DSERIAL_PRINT("ok");
     return OK;
  }

    return ERROR;
}


/* @brief  Sends the file name and metadata (size, time, etc.) to the receiver.
 * @param name
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
int ZModemSend::wctxpn(const char *name) {

  // *p points to beginning of txbuf?
  // *q points to end of txbuf
  char *p, *q;

  strcpy(txbuf,name);
  p = q = txbuf + strlen(txbuf)+1;
  while (q < (txbuf + TXBSIZE)) {
    // set all of txbuf to '0'
    *q++ = 0;
  }

  Totalleft -= _fout->fileSize();

DSERIAL_PRINT(F("  length = ")); DSERIAL_PRINTLN(Totalleft);

  if (--Filesleft <= 0)
    Totalleft = 0;
  if (Totalleft < 0)
    Totalleft = 0;

  /* force 1k blocks if name won't fit in 128 byte block */
  if (txbuf[125])
    blklen = TXBSIZE;
  else {          /* A little goodie for IMP/KMD */
    blklen = 128;
    txbuf[127] = (_fout->fileSize() + 127) >>7;
    txbuf[126] = (_fout->fileSize() + 127) >>15;
  }


  return sendZFILE(txbuf, 1+strlen(p)+(p-txbuf));
}





/* fill buf with count chars padding with ^Z for CPM */
int ZModemSend::filbuf(char *buf,int count)
{
  int c, m;

DSERIAL_PRINTLN("\nfilbuf");

  if ( !Ascii) {
//    m = read(fileno(in), buf, count);
    m = _fout->read(buf, count);
DSERIAL_PRINTLN(F("filbuf: '"));
//for(int i=0;i<m;i++) {
//  DSERIAL_PRINT(buf[i]);
//}
DSERIAL_PRINTLN(F("'"));
    if (m <= 0)
      return 0;
    while (m < count)
      buf[m++] = 032;
    return count;
  }
  m=count;
  if (Lfseen) {
    *buf++ = 012;
    --m;
    Lfseen = 0;
  }
//  while ((c=getc(in))!=EOF) {
  while((c = _fout->read()) != -1) {
    if (c == 012) {
      *buf++ = 015;
      if (--m == 0) {
        Lfseen = TRUE;
        break;
      }
    }
    *buf++ =c;
    if (--m == 0)
      break;
  }
  if (m==count)
    return 0;
  else
    while (--m>=0)
      *buf++ = CPMEOF;
  return count;
}

/* Fill buffer with blklen chars */
int ZModemSend::zfilbuf(void)
{
  int n;

// This code works:
//  n = fread(txbuf, 1, blklen, in);
  n = _fout->read(txbuf,blklen);

  if (n < blklen)
    Eofseen = 1;
  return n;
}



/* Send a ZFILE frame
 * https://github.com/TeraTermProject/teraterm/wiki/ZMODEM-Protocol#zfile0x04
 #### ZFILE(0x04)

- Binary/Hex Header
- Sender→Receiver

| offset   | length   | means               | remark                                     |
| -------- | -------- | ------------------- | ------------------------------------------ |
| 0        | 1        | ZPAD (0x2A, "*")    |                                            |
| 1        | 1        | ZDLE (0x18)         |                                            |
| 2        | 1        | ZBIN (0x41, "A")    | Bin Header                                 |
| 3        | 1        | ZFILE (0x04)        |                                            |
| 4        | 1        | ZF3(0x??)           |                                            |
| 5        | 1        | ZF2(0x??)           |                                            |
| 6        | 1        | ZF1(0x??)           |                                            |
| 7        | 1        | ZF0(0x??)           |                                            |
| 8        | 2        | 16bit CRC           | 4byte = 16bit                              |
| -------- | -------- | ------------------- | ------------------------------------------ |
| 10       | ?        | PATHNAME            | UTF-8?                                     |
| ?        | ?        | 0x00                |                                            |
| ?        | ?        | LENGTH              | "[0-9]+" %lu                               |
| ?        | ?        | 0x20, " "           |                                            |
| ?        | ?        | MODIFICATION DATE   | "[0-7]+" %lo (seconds from 1970/1/1 UTC)   |
| ?        | ?        | 0x20, " "           |                                            |
| ?        | ?        | FILE MODE           | "[0-7]+" %lo (UNIX由来)                    |
| ?        | ?        | 0x00                | terminator                                 |

- MODIFICATION DATE and FILE MODE is option

flags (ZF0...ZF3)

| flag byte | byte/bit | means                                  |
| --------- | -------- | -------------------------------------- |
| ZF0       | 0x01     | ZCBIN (Tera Term : only 1)             |
| ZF0       | 0x02     | ZCNL                                   |
| ZF0       | 0x04     | ZCRECOV (Not implimented in Tera Term) |
| ZF1       | 0x01     | ZMNEWL  (Not implimented in Tera Term) |
| ZF1       | 0x02     | ZMCRC   (Not implimented in Tera Term) |
| ZF1       | 0x04     | ZMAPND  (Not implimented in Tera Term) |
| ZF1       | 0x08     | ZMCLOB  (Not implimented in Tera Term) |
| ZF1       | 0x10     | ZMDIFF  (Not implimented in Tera Term) |
| ZF1       | 0x20     | ZMPROT  (Not implimented in Tera Term) |
| ZF1       | 0x40     | ZMNEW   (Not implimented in Tera Term) |
| ZF2       | 0x01     | ZTLZW   (Not implimented in Tera Term) |
| ZF2       | 0x02     | ZTCRYPT (Not implimented in Tera Term) |
| ZF2       | 0x04     | ZTRLE   (Not implimented in Tera Term) |
| ZF3       | 0x01     | ZTSPARS (Not implimented in Tera Term) |
 *
 */
int ZModemSend::sendZFILE(char *buf, int blen)
{
  int c;

DSERIAL_PRINTLN(F("\nzsendfile"));

  while (true) {
    Txhdr[ZF0] = Lzconv;    /* file conversion request */
    Txhdr[ZF1] = Lzmanag;   /* file management request */
    if constexpr (Lskipnocor){
      Txhdr[ZF1] |= ZF1_ZMSKNOLOC;
      }
    Txhdr[ZF2] = Lztrans;   /* file transport request */
    Txhdr[ZF3] = 0; // should be 0 !!
    zsbhdr(ZFILE, Txhdr);
    sendData(buf, blen, ZCRCW);
again:
    c = zgethdr(Rxhdr, 1);
    switch (c) {
    case ZRINIT:
      while ((c = readline(50)) > 0)
        if (c == ZPAD) {
          goto again;
        }
      /* **** FALL THRU TO **** */
    default:
      continue;
    case ZCAN:
    case TIMEOUT:
    case ZABORT:
    case ZFIN:
DSERIAL_PRINTLN(F("\nzsendfile - ZFIN"));

      return ERROR;
    case ZCRC:
      ZMCRC32 crc32;
      crc32.begin(0xFFFFFFFF);
      if (Canseek >= 0) {
        _fout->seekSet(0);
        while (((c = _fout->read()) != -1)) {
          // && --Rxpos)
          crc32.update(c);
        }
        crc32.invert();

          _fout->seekSet(0);
      }
      stohdr(crc32.get());
      zsbhdr(ZCRC, Txhdr);
      goto again;
    case ZSKIP:
      _fout->close();
DSERIAL_PRINTLN(F("\nzsendfile - ZSKIP"));
      return c;
    case ZRPOS:
      /*
       * Suppress zcrcw request otherwise triggered by
       * lastyunc==bytcnt
       */

      if(Rxpos && !_fout->seekSet(Rxpos))
        return ERROR;
      Lastsync = (bytcnt = Txpos = Rxpos) -1;
      int ret = sendFileData();
DSERIAL_PRINT(F("\nzsendfile - exit - "));
DSERIAL_PRINTLN(ret);
      return(ret);
    }
  } //
}



/* Send the data in the file */
int ZModemSend::sendFileData(void)
{
  int c, n;
  uint8_t e;
  int newcnt;
  uint8_t junkcount = 0;          /* Counts garbage chars received by TX */

DSERIAL_PRINT(F("\nzsendfdata: "));
DSERIAL_PRINT(F("number = "));
DSERIAL_PRINT(Filesleft+1);
DSERIAL_PRINT(F("   length = "));
DSERIAL_PRINTLN(Totalleft);
  Lrxpos = 0;
  Beenhereb4 = FALSE;
somemore:
  //if (setjmp(intrjmp)) {
  if (0) {
waitack:
    junkcount = 0;
    c = getinsync(0);
gotack:
    switch (c) {
    default:
    case ZCAN:
      _fout->close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - error - 1"));
      return ERROR;
    case ZSKIP:
      _fout->close();
      //fclose(in);
      return c;
    case ZACK:
    case ZRPOS:
      break;
    case ZRINIT:
      return OK;
    }
#ifdef READCHECK
    /*
                 * If the reverse channel can be tested for data,
     *  this logic may be used to detect error packets
     *  sent by the receiver, in place of setjmp/longjmp
     *  rdchk(fdes) returns non 0 if a character is available
     */
    while (_serial->available()) {
#ifdef SV
      switch (checked)
#else
        switch (readline(1))
#endif
        {
        case CAN:
        case ZPAD:
          c = getinsync(1);
          goto gotack;
        case XOFF:              /* Wait a while for an XON */
        case XOFF |0x80:
          readline(100);
        }
    }
#endif
  }

DSERIAL_PRINTLN("zsendfdata - 1");

//  if ( !Fromcu)
//    signal(SIGINT, onintr);
  newcnt = Rxbuflen;
  Txwcnt = 0;
  stohdr(Txpos);
  zsbhdr(ZDATA, Txhdr);

DSERIAL_PRINTLN("zsendfdata - 2");

  do {
    n = zfilbuf();
// AHA - it reads the 18 chars here
DSERIAL_PRINTLN(n);
    if (Eofseen)
      e = ZCRCE;
    else if (junkcount > 3)
      e = ZCRCW;
    else if (bytcnt == Lastsync)
      e = ZCRCW;
    else if (Rxbuflen && (newcnt -= n) <= 0)
      e = ZCRCW;
    else if (Txwindow && (Txwcnt += n) >= Txwspac) {
      Txwcnt = 0;
      e = ZCRCQ;
    }
    else
      e = ZCRCG;
    if (Verbose>1)
      fprintf(stderr, "\r%7ld ZMODEM%s    ",Txpos, Crc32t?" CRC-32":"");
    sendData(txbuf, n, e);
    bytcnt = Txpos += n;
    if (e == ZCRCW)
      goto waitack;
#ifdef READCHECK
    /*
     * If the reverse channel can be tested for data,
     *  this logic may be used to detect error packets
     *  sent by the receiver, in place of setjmp/longjmp
     *  rdchk(fdes) returns non 0 if a character is available
     */
//    fflush(stdout);
    while (_serial->available()) {
#ifdef SV
      switch (checked)
#else
        switch (readline(1))
#endif
        {
        case CAN:
        case ZPAD:
          c = ZModemSend::getinsync(1);
          if (c == ZACK)
            break;
#ifdef TCFLSH
          ioctl(iofd, TCFLSH, 1);
#endif
          /* zcrce - dinna wanna starta ping-pong game */
          sendData(txbuf, 0, ZCRCE);
          goto gotack;
        case XOFF:              /* Wait a while for an XON */
        case XOFF|0200:
          readline(100);
        default:
          ++junkcount;
        }
    }
#endif  /* READCHECK */

  } while (!Eofseen);

DSERIAL_PRINTLN("zsendfdata - 4");


  for (;;) {
    stohdr(Txpos);
    zsbhdr(ZEOF, Txhdr);
    switch (getinsync(0)) {
    case ZACK:
DSERIAL_PRINTLN(F("zsendfdata - ZAK"));
      continue;
    case ZRPOS:
DSERIAL_PRINTLN(F("zsendfdata - ZRPOS"));
      goto somemore;
    case ZRINIT:
DSERIAL_PRINTLN(F("zsendfdata - OK"));
      return OK;
    case ZSKIP:
      _fout->close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - ZSKIP"));
      return c;
    default:
      _fout->close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - error - 2"));
      return ERROR;
    }
  }
}




/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int ZModemSend::getinsync(int flag)
{
  int c;

  for (;;) {
    if (Test) {
DSERIAL_PRINTLN(F("***** Signal Caught *****"));
      Rxpos = 0;
      c = ZRPOS;
    }
    else
      c = zgethdr(Rxhdr, 0);
    switch (c) {
    case ZCAN:
    case ZABORT:
    case ZFIN:
    case TIMEOUT:
DSERIAL_PRINTLN(F("getinsync - timeout"));
      return ERROR;
    case ZRPOS:
      /* ************************************* */
      /*  If sending to a buffered modem, you  */
      /*   might send a break at this point to */
      /*   dump the modem's buffer.            */
//      clearerr(in);   /* In case file EOF seen */
//      if (fseek(in, Rxpos, 0)) {
      // seekSet returns true on success
      if(!_fout->seekSet(Rxpos)) {
DSERIAL_PRINTLN(F("getinsync - fseek"));
        return ERROR;
      }
      Eofseen = 0;
      bytcnt = Lrxpos = Txpos = Rxpos;
      if (Lastsync == Rxpos) {
        if (++Beenhereb4 > 4)
          if (blklen > 32)
            blklen /= 2;
      }
      Lastsync = Rxpos;
      return c;
    case ZACK:
      Lrxpos = Rxpos;
      if (flag || Txpos == Rxpos)
        return ZACK;
      continue;
    case ZRINIT:
    case ZSKIP:
      _fout->close();      
      //fclose(in);
      return c;
    case ERROR:
    default:
      zsbhdr(ZNAK, Txhdr);
      continue;
    }
  }
}

// Dylan (monte_carlo_ecm, bitflipper, etc.) - I added this simple ZRQINIT string to trigger
// terminal program's receive auto start feature.  This was missing in the code as I found it.
// All the terminal applications I tried would receive files anyway if I manually started
// the download, but sending the ZRQINIT is the right way to initiate a ZMODEM transfer
// according to the protocol documentation.

// #define ZRQINIT_STR F(\
// "\x2a\x2a\x18\x42\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x0d\x0a\x11")
// // **␘B (14 zeros) CR-LF-DC1
// // <ZPAD><ZPAD><ZDLE><ZHEX>)(14 zeros) CR-LF-<XON>

void ZModemSend::sendzrqinit(void)
{
  constexpr char ZRQINIT[22] = "\x2a\x2a\x18\x42\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x0d\x0a\x11";
// **␘B (14 zeros) CR-LF-DC1
// <ZPAD><ZPAD><ZDLE><ZHEX>)(14 zeros) CR-LF-<XON>

    _serial->print(ZRQINIT);
}

/* Say "bibi" to the receiver, try to do it cleanly */
void ZModemSend::saybibi() {
  for (;;) {
    stohdr(0L);             /* CAF Was zsbhdr - minor change */
    zshhdr(ZFIN, Txhdr);    /*  to make debugging easier */
    switch (zgethdr(Rxhdr, 0)) {
    case ZFIN:
      _serial->write('O');
      _serial->write('O');
      _serial->flush();
    case ZCAN:
    case TIMEOUT:
      return;
    } // switch
  } // for
} // saybibi


void ZModemSend::zmodem_send_file(FsFile &file) {
  if (!file.isOpen()) {
    ASERIAL_PRINTLN(F("file not open"));
  } else {
    _fout = &file;
    char name[PATHLEN];
    _fout->getName(name, PATHLEN);
    // Start the ZMODEM transfer
    Filesleft = 1;
    Totalleft = _fout->fileSize();
    _serial->print(F("rz\n"));
    _serial->flush();
    sendzrqinit();
    delay(200);
    wcs(name);
    saybibi();
    _fout->close();
  }
}


/* Send ZMODEM binary header hdr of type type */
void ZModemSend::zsbhdr(uint8_t type, char *hdr)
{
  ZMCRC32 crc32;
  crc32.begin(0xFFFFFFFF);

  static ZMCRC16 crc16;
  crc16.begin(0);            // initialize to 0

  int n;

  vfile(F("zsbhdr: %s %lx"), frametypes[type+FTOFFSET], rclhdr(hdr));
  /*  if (type == ZDATA)
      for (n = Znulls; --n >=0; )
        _serial->write(0);
  */
  _serial->write(ZPAD);
  _serial->write(ZDLE);
  //Pete (El Supremo) This looks wrong but it is correct - the code fails if == is used
  if ((Crc32tx = Txfcs32)) {

    _serial->write(ZBIN32);

    zSendCharCRC(type);

    for (n=4; --n >= 0; ++hdr) {
      zSendCharCRC(*hdr);
    }
    crc32.invert();
    for (n=4; --n >= 0;) {
      zSendChar((uint8_t)crc32.get());
      crc32.rightshift(8);
    }
  } else {

    _serial->write(ZBIN);

    zSendCharCRC(type);

    for (n=4; --n >= 0; ++hdr) {
      zSendCharCRC(*hdr);
    }
    crc16.update(0);
    crc16.update(0);
    zSendChar((uint8_t)(crc16.get()>>8));
    zSendChar((uint8_t)crc16.get());
  }
  if (type != ZDATA)
    _serial->flush();
}

