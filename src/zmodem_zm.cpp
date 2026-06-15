/*
 *    ZMODEM protocol primitives
 *    05-09-88  Chuck Forsberg Omen Technology Inc
 *
 * Entry point Functions:
 *      zsbhdr(type, hdr) send binary header
 *      zshhdr(type, hdr) send hex header
 *      zgethdr(hdr, eflag) receive header - binary or hex
 *      zsdata(buf, len, frameend) send data
 *      zrdata(buf, len) receive data
 *      stohdr(pos) store position data in Txhdr
 *      long rclhdr(hdr) recover position offset from header
 */

#include "zmodem.h"

#ifdef ARDUINO
#include "zmodem_config.h"
#include "zmodem_zm.h"
#include "zmodem.h"
#else
#ifndef CANFDX
#include "zmodem.h"
#endif
#endif

// Shared globals
long Bytesleft; // from rz - Shared with sz bytcnt
long rxbytes;   // from rz - Shared with sz Lrxpos
int Blklen;     // from rz - Shared with sz blklen

#define Rxtimeout 100            /* Tenths of seconds to wait for something */


// This buffer blends Txb (from sz) and secbuf (from rz) into a single buffer, saving 1K
// of memory.
char oneKbuf[1025];

/* Globals used by ZMODEM functions */
uint8_t Rxframeind;         /* ZBIN ZBIN32, or ZHEX type of frame received */
uint8_t Rxtype;             /* Type of header received */
int Rxcount;            /* Count of data bytes received */
char Rxhdr[4];          /* Received header */
char Txhdr[4];          /* Transmitted header */
long Rxpos;             /* Received file position */
long Txpos;             /* Transmitted file position */
int8_t Txfcs32;            /* TRUE means send binary frames with 32-bit FCS */
int8_t Crc32tx;             /* Display flag indicating 32-bit CRC being sent */
int8_t Crc32rx;              /* Display flag indicating 32-bit CRC being received */
//int Znulls;             /* Number of nulls to send at beginning of ZDATA hdr */
char Attn[ZATTNLEN+1];  /* Attention string rx sends to tx on err */

char zconv;             /* ZMODEM file conversion request */
char zmanag;            /* ZMODEM file management request */
char ztrans;            /* ZMODEM file transport request */
uint8_t Zctlesc;            /* Encode control characters */
#define Zrwindow 1400    /* RX window size (controls garbage count) */
//int Nozmodem = 0;       /* If invoked as "rb" */
int lastsent;           /* Last char we sent */
uint8_t Not8bit;            /* Seven bits seen on header */
//char Lzmanag;           /* Local ZMODEM file management request */
//int Restricted = 0;       /* restricted; no /.. or ../ in filenames */
//int Quiet=0;            /* overrides logic that would otherwise set verbose */
uint8_t Eofseen;            /* EOF seen on input set by zfilbuf */


uint8_t firstsec;
char Lastrx;
char Crcflg;
uint8_t errors;

#define badcrc F("Bad CRC");





ZModem::ZModem() = default;

/* Send ZMODEM HEX header hdr of type type */
void ZModem::sendHexHeader(const int type, const char *hdr)
{
  static ZMCRC16 crc16;
  int n;
  crc16.begin(0);
  Crc32tx = 0; // hex frames always use 16-bit CRC

  vfile(F("zshhdr: %s %lx"), frametypes[type+FTOFFSET], rclhdr(hdr));
  _serial->write(ZPAD);
  _serial->write(ZPAD);
  _serial->write(ZDLE);
  _serial->write(ZHEX);
  zputhex(type);
  crc16.update(type);

  for (n=4; --n >= 0; ++hdr) {
    zputhex(*hdr); 
    crc16.update(*hdr);
  }
  crc16.update(0);
  crc16.update(0);
  zputhex((uint8_t)(crc16.get()>>8));
  zputhex((uint8_t)crc16.get());

  /* Make it printable on remote machine */
  _serial->write(CR);
  _serial->write(0x8a);
  /*
         * Uncork the remote in case a fake XOFF has stopped data flow
   */
  if (type != ZFIN && type != ZACK)
    _serial->write(XON);
  _serial->flush();
}

/*
 * Send binary array buf of length length, with ending ZDLE sequence frameend
 */
/*
static char *Zendnames[] = { 
  (char *)"ZCRCE", 
  (char *)"ZCRCG",
  (char *)"ZCRCQ",
  (char *)"ZCRCW"
};
*/

/**
 *  Sends a ZModem data sub-packet with CRC.
 */
void ZModem::sendData(char *buf,int length,int frameend) {

    static ZMCRC32 crc32;
    crc32.begin(0xFFFFFFFF);

    static ZMCRC16 crc16;
    crc16.begin(0);            // initialize to 0

  // send all characters in buf
    for (;--length >= 0; ++buf) {
      zSendChar(*buf);
      if (Crc32tx) {
        crc32.update(*buf & 255);
      }
      else {
        crc16.update(*buf & 255);
      }
    } // end for (send all characters in buf)
    _serial->write(ZDLE);
    _serial->write(frameend);


  if (Crc32tx) {
    crc32.update(frameend);
    crc32.invert();
    for (length=4; --length >= 0;) {
      zSendChar((uint8_t)crc32.get());
      crc32.rightshift(8);
    }
  }
  else {                        // (16 bit CRC)
      crc16.update(frameend);
      crc16.update(0);
      crc16.update(0);
      zSendChar((uint8_t)(crc16.get()>>8));
      zSendChar((uint8_t)crc16.get());
    }

  if (frameend == ZCRCW) {
    _serial->write(XON);
    _serial->flush();
  }
}

/*
 * Receive array buf of max length with ending ZDLE sequence
 *  and CRC.  Returns the ending character or error code.
 *  NB: On errors may store length+1 bytes!
 */
int ZModem::zrdata(char *buf,int length)
{
  int c;
  char *end;
  int d;

  static ZMCRC16 crc16;
  static ZMCRC32 crc32;

  if (Rxframeind == ZBIN32) {

    crc32.begin(0xFFFFFFFF);
    Rxcount = 0;  
    end = buf + length;
    while (buf <= end) {
      if ((c = zdlread()) & ~255) {
  crcfoo32:
        switch (c) {
        case GOTCRCE:
        case GOTCRCG:
        case GOTCRCQ:
        case GOTCRCW:
          d = c;  
          c &= 255;
          crc32.update(c);
          if ((c = zdlread()) & ~255)
            goto crcfoo32;
          crc32.update(c);
          if ((c = zdlread()) & ~255)
            goto crcfoo32;
          crc32.update(c);
          if ((c = zdlread()) & ~255)
            goto crcfoo32;
          crc32.update(c);
          if ((c = zdlread()) & ~255)
            goto crcfoo32;
          crc32.update(c);
          if (crc32.get() != 0xDEBB20E3) {
            zperr(badcrc);
            return ERROR;
          }
          Rxcount = length - (end - buf);
          vfile(F("zrdat32: %d %s"), Rxcount,
          Zendnames[(d-GOTCRCE)&3]);
          return d;
        case GOTCAN:
          zperr("Sender Canceled");
          return ZCAN;
        case TIMEOUT:
          zperr("TIMEOUT");
          return c;
        default:
          zperr("Bad data subpacket");
          return c;
        }
      }
      *buf++ = c;
      crc32.update(c);
    }
    zperr("Data subpacket too long");
    return ERROR;
  } else {
    unsigned short crc;

    Rxcount = 0;
    crc16.begin(0);
    end = buf + length;
    while (buf <= end) {
      if ((c = zdlread()) & ~255) {
  crcfoo16:
        switch (c) {
        case GOTCRCE:
        case GOTCRCG:
        case GOTCRCQ:
        case GOTCRCW:
          crc16.update((d=c)&255);
          if ((c = zdlread()) & ~255)
            goto crcfoo16;
          crc16.update(c);
          if ((c = zdlread()) & ~255)
            goto crcfoo16;
          crc16.update(c);
          if (crc & 0xFFFF) {
            zperr(badcrc);
            return ERROR;
          }
          Rxcount = length - (end - buf);
          vfile(F("zrdata: %d  %s"), Rxcount,
          Zendnames[(d-GOTCRCE)&3]);
          return d;
        case GOTCAN:
          zperr("Sender Canceled");
          return ZCAN;
        case TIMEOUT:
          zperr("TIMEOUT");
          return c;
        default:
          zperr("Bad data subpacket");
          return c;
        }
      }
      *buf++ = c;
      crc16.update(c);
    }
    zperr("Data subpacket too long");
    return ERROR;
  }
}

/*
 * Read a ZMODEM header to hdr, either binary or hex.
 *  eflag controls local display of non zmodem characters:
 *      0:  no display
 *      1:  display printing characters only
 *      2:  display all non ZMODEM characters
 *  On success, set Zmodem to 1, set Rxpos and return type of header.
 *   Otherwise return negative on error.
 *   Return ERROR instantly if ZCRCW sequence, for fast error recovery.
 */
int ZModem::zgethdr(char *hdr,int eflag)
{
  int c, n, cancount;

  n = Zrwindow; //+ Baudrate;        /* Max bytes before start of frame */
  Rxframeind = Rxtype = 0;

startover:
  cancount = 5;
again:
  /* Return immediate ERROR if ZCRCW sequence seen */
  _serial->setTimeout(Rxtimeout * 100);
  c = readline(Rxtimeout);
  _serial->setTimeout(TYPICAL_SERIAL_TIMEOUT);
  
  switch (c) {
  case RCDO:
  case TIMEOUT:
    goto fifi;
  case CAN:
gotcan:
    if (--cancount <= 0) {
      c = ZCAN;
      goto fifi;
    }
    switch (c = readline(1)) {
    case TIMEOUT:
      goto again;
    case ZCRCW:
      c = ERROR;
      /* **** FALL THRU TO **** */
    case RCDO:
      goto fifi;
    default:
      break;
    case CAN:
      if (--cancount <= 0) {
        c = ZCAN;
        goto fifi;
      }
      goto again;
    }
    /* **** FALL THRU TO **** */
  default:
agn2:
    if ( --n == 0) {
      zperr("Garbage count exceeded");
      return(ERROR);
    }
    if (eflag && ((c &= 0x7f) & 0x60))
      bttyout(c);
    else if (eflag > 1)
      bttyout(c);
#ifdef UNIX
    fflush(stderr);
#endif
    goto startover;
  case ZPAD|0x80:         /* This is what we want. */
    Not8bit = c;
  case ZPAD:              /* This is what we want. */
    break;
  }
  cancount = 5;
splat:
  switch (c = noxrd7()) {
  case ZPAD:
    goto splat;
  case RCDO:
  case TIMEOUT:
    goto fifi;
  default:
    goto agn2;
  case ZDLE:              /* This is what we want. */
    break;
  }

  switch (c = noxrd7()) {
  case RCDO:
  case TIMEOUT:
    goto fifi;
  case ZBIN:
    return(ERROR);
    // Rxframeind = ZModem::ZBIN;
    // Crc32rx = FALSE;
    // c =  zrbhdr(hdr);
    // break;
  case ZBIN32:
    return(ERROR);
    // Crc32rx = Rxframeind = ZModem::ZBIN32;
    // c =  zrbhdr32(hdr);
    // break;
  case ZHEX:
    Rxframeind = ZHEX;
    Crc32rx = FALSE;
    c =  zrhhdr(hdr);
    break;
  case CAN:
    goto gotcan;
  default:
    goto agn2;
  }
  Rxpos = hdr[ZP3] & 255;
  Rxpos = (Rxpos<<8) + (hdr[ZP2] & 255);
  Rxpos = (Rxpos<<8) + (hdr[ZP1] & 255);
  Rxpos = (Rxpos<<8) + (hdr[ZP0] & 255);
fifi:

  switch (c) {
  case GOTCAN:
    c = ZCAN;
    /* **** FALL THRU TO **** */
  case ZNAK:
  case ZCAN:
  case ERROR:
  case TIMEOUT:
  case RCDO:
//    zperr("Got %s", frametypes[c+FTOFFSET]);
    /* **** FALL THRU TO **** */
//  default:
//    if (c >= -3 && c <= FRTYPES)
//      vfile(F("zgethdr: %s %lx"), frametypes[c+FTOFFSET], Rxpos);
//    else
//      vfile(F("zgethdr: %d %lx"), c, Rxpos);
break;
  }
  return c;
}

//#endif



/* Receive a hex style header (type and position) */
int ZModem::zrhhdr(char *hdr)
{
  int c;
  static ZMCRC16 crc16;
  crc16.begin(0);
  int n;

  if ((c = zgethex()) < 0)
    return c;
  Rxtype = c;
  crc16.update(c);

  for (n=4; --n >= 0; ++hdr) {
    if ((c = zgethex()) < 0)
      return c;
    crc16.update(c);
    *hdr = c;
  }
  if ((c = zgethex()) < 0)
    return c;
  crc16.update(c);
  if ((c = zgethex()) < 0)
    return c;
  crc16.update(c);
  if (crc16.get() & 0xFFFF) {
    zperr(badcrc); 
    return ERROR;
  }
  switch ( c = readline(1)) {
  case 0x8d:
    Not8bit = c;
    /* **** FALL THRU TO **** */
  case CR:
    /* Throw away possible cr/lf */
    switch (c = readline(1)) {
    case LF:
      Not8bit |= c;
    }
  }
#ifdef ZMODEM
  Protocol = ZMODEM;
#endif
  return Rxtype;
}


/* Send a byte as two hex digits */
/*void zputhex(int c) */
void ZModem::zputhex(int c)
{
  static constexpr char digits[17] = "0123456789abcdef";

  if constexpr (Verbose>8)
    vfile(F("zputhex: %02X"), c);
  _serial->write(pgm_read_byte(digits+((c&0xF0)>>4)));
  _serial->write(pgm_read_byte(digits+((c)&0xF)));
}

/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telnet net escape)
 */
void ZModem::zSendCharCRC(char c) {
  zSendCharCRC((uint8_t)c); // turn into int
}

void ZModem::zSendCharCRC(uint8_t c) {
  static ZMCRC32  crc32;
  static ZMCRC16  crc16;
  if (Crc32tx) {
    crc32.update(c);
  }
  else {
    crc16.update(c);
  }
  zSendChar(c); // turn into int
}


void ZModem::zSendChar(char c) {
  zSendChar((uint8_t)c); // turn into int
}

void ZModem::zSendChar(uint8_t c)
{
  /* check for non-control characters */

  if (c & 96) // (c >= 0x60; control codes are all lower than this)
    _serial->write(lastsent = c);

  else {
    switch (c &= 255) {
    case ZDLE:
      _serial->write(ZDLE);
      _serial->write(lastsent = (c ^= 0x40));
      break;
    case CR: // CR
    case 0x8d: //
      if (!Zctlesc && (lastsent & 0x7f) != '@') {
        _serial->write(lastsent = c);
      }
        break;

      /* **** FALL THRU TO **** */
    case 0x10: // DLE
    case XON: // DC1
    case XOFF: // DC3
    case 0x90: // hex 8D
    case 0x91: // '
    case 0x93: // "
      _serial->write(ZModem::ZDLE);
      c ^= 0x40;
// sendit:
      _serial->write(lastsent = c);
      break;

    default:
      if (Zctlesc && ! (c & 0x60)) {
        _serial->write(ZModem::ZDLE);
        c ^= 0x40;
      }
      _serial->write(lastsent = c);
    }
  }
}


/* Decode two lower case hex digits into an 8 bit byte value */

int ZModem::zgethex(void)
{
  int c, n;

  if ((c = noxrd7()) < 0)
    return c;
  n = c - '0';
  if (n > 9)
    n -= ('a' - ':');
  if (n & ~0xF)
    return ERROR;
  if ((c = noxrd7()) < 0)
    return c;
  c -= '0';
  if (c > 9)
    c -= ('a' - ':');
  if (c & ~0xF)
    return ERROR;
  c += (n<<4);
  return c;
}



/*
 * Read a byte, checking for ZMODEM escape encoding
 *  including CAN*5 which represents a quick abort
 */
int ZModem::zdlread(){
  int c;

again:
  // Quick check for non control characters
  if ((c = readline(Rxtimeout)) < 0)
    return c;

  switch (c) {
  case ZModem::ZDLE:
    break;
  case XOFF:
  case 0x93:
  case XON:
  case 0x91:
    goto again;
  default:
    if (Zctlesc && !(c & 0x60)) {
      goto again;
    }
    return c;
  }
again2:
  if ((c = readline(Rxtimeout)) < 0)
    return c;
  if (c == ZModem::CAN && (c = readline(Rxtimeout)) < 0)
    return c;
  if (c == ZModem::CAN && (c = readline(Rxtimeout)) < 0)
    return c;
  if (c == ZModem::CAN && (c = readline(Rxtimeout)) < 0)
    return c;
  switch (c) {
  case ZModem::CAN:
    return ZModem::GOTCAN;
  case ZModem::ZCRCE:
  case ZModem::ZCRCG:
  case ZModem::ZCRCQ:
  case ZModem::ZCRCW:
    return (c | ZModem::GOTOR);
  case ZModem::ZRUB0:
    return 0x7f;
  case ZModem::ZRUB1:
    return 255;
  case XOFF:
  case 0x93:
  case XON:
  case 0x91:
    goto again2;
  default:
    if (Zctlesc && ! (c & 0x60)) {
      goto again2;
    }
    if ((c & 0x60) ==  0x40)
      return (c ^ 0x40);
    break;
  }
  if (Verbose>1)
    zperr("Bad escape sequence %x", c);
  return ERROR;
}

/*
 * Read a character from the modem line with timeout.
 *  Eat parity, XON and XOFF characters.
 */
int ZModem::noxrd7(void)
{
  int c;

  for (;;) {
    if ((c = readline(Rxtimeout)) < 0)
      return c;
    switch (c &= 0x7f) {
    case XON:
    case XOFF:
      continue;
    default:
      if (Zctlesc && !(c & 0x60))
        continue;
    case '\r':
    case '\n':
    case ZModem::ZDLE:
      return c;
    }
  }
}



/* Store long integer pos in Txhdr */
void ZModem::stohdr(long pos)
{
  Txhdr[ZP0] = pos;
  Txhdr[ZP1] = pos>>8;
  Txhdr[ZP2] = pos>>16;
  Txhdr[ZP3] = pos>>24;
}


#ifndef NOTDEF
/* Recover a long integer from a header */
long ZModem::rclhdr(char *hdr)
{
  long l;

  l = (hdr[ZP3] & 255);
  l = (l << 8) | (hdr[ZP2] & 255);
  l = (l << 8) | (hdr[ZP1] & 255);
  l = (l << 8) | (hdr[ZP0] & 255);
  return l;
}
#endif



/*
 * Local console output simulation
 */
void ZModem::bttyout(int c)
{
#ifndef ARDUINO
  if (Verbose || Fromcu)
    putc(c, stderr);
#endif
}


int ZModem::readline(int timeout) {
  long then;
  unsigned char c;

  then = millis();
  while(_serial->available() < 1) {
    if(millis() - then > (unsigned int)timeout*10UL) {
      //DSERIAL.println("");
      return(TIMEOUT);
    }
  }
  c = _serial->read();
  //DSERIAL.write(c);
  //DSERIAL.print(" ");
  return(c);
}







// definitions for CRC16 functions
uint16_t ZMCRC16::crc;
void ZMCRC16::begin() {crc = (uint16_t) 0; }
void ZMCRC16::begin(uint16_t value) {crc = value; }
void ZMCRC16::update(uint8_t cp) {
  crc = (crctab[((crc >> 8) & 255)] ^ (crc << 8)) ^ cp;
}
uint16_t ZMCRC16::get() {return crc;}

// definitions for CRC32 functions
uint32_t ZMCRC32::crc;
void ZMCRC32::begin() {crc = (uint32_t) 0; }
void ZMCRC32::begin(uint32_t value) {crc = value; }
void ZMCRC32::update(uint8_t cp) {
  crc = crctab[(crc^cp) & 0xff] ^ ((crc >> 8) & 0x00FFFFFF);
}
void ZMCRC32::invert() {
  crc = ~crc;
}

void ZMCRC32::rightshift(uint shift) {
  // Perform the right shift on the internal value
  crc >>= shift;
}
uint32_t ZMCRC32::get() {return crc;}


