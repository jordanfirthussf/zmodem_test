// See this page for all code: http://www.raspberryginger.com/jbailey/minix/html/dir_acf1a49c3b8ff2cb9205e4a19757c0d6.html
// From: http://www.raspberryginger.com/jbailey/minix/html/zm_8c-source.html
// docs at: http://www.raspberryginger.com/jbailey/minix/html/zm_8c.html

// Look at all the files:
// http://www.raspberryginger.com/jbailey/minix/html/files.html

#ifndef ZMODEM_ZM_CPP
#define ZMODEM_ZM_CPP

/*
 *   Z M . C
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
//
// using ZModem::zsbhdr;
// using ZModem::zshhdr;
// using ZModem::zsdata;
// using ZModem::zrdata;
// using ZModem::zgethdr;
// using ZModem::zrbhdr;
// using ZModem::zrbhdr32;
// using ZModem::zrhhdr;
// using ZModem::zputhex;
// using ZModem::zsendline;
// using ZModem::sendline;
// using ZModem::zgethex;
// using ZModem::zdlread;
// using ZModem::noxrd7;
// using ZModem::stohdr;
// using ZModem::rclhdr;
// using ZModem::purgeline;
// using ZModem::bttyout;
// using ZModem::canit;
// using ZModem::readline;
// using ZModem::updcrc;

// Shared globals
long Bytesleft; // from rz - Shared with sz bytcnt
long rxbytes;   // from rz - Shared with sz Lrxpos
int Blklen;     // from rz - Shared with sz blklen

#define Rxtimeout 100            /* Tenths of seconds to wait for something */
#define Verbose 0

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





/* Send ZMODEM binary header hdr of type type */
void ZModem::zsbhdr(int type, char *hdr)
{


  vfile(F("zsbhdr: %s %lx"), frametypes[type+FTOFFSET], rclhdr(hdr));
/*  if (type == ZDATA)
    for (n = Znulls; --n >=0; )
      sendline(0);
*/
  sendline(ZModem::ZPAD);
  sendline(ZModem::ZDLE);
//Pete (El Supremo) This looks wrong but it is correct - the code fails if == is used
  if ((Crc32tx = Txfcs32)) {
    int n;
    unsigned long crc;
  
    sendline(ZModem::ZBIN32);
    zsendline(type);
    crc = 0xFFFFFFFFL; 
    crc = UPDC32(type, crc);
  
    for (n=4; --n >= 0; ++hdr) {
      crc = UPDC32((0377 & *hdr), crc);
      zsendline(*hdr);
    }
    crc = ~crc;
    for (n=4; --n >= 0;) {
      zsendline((int)crc);
      crc >>= 8;
    }
  } else {
    int n;
    unsigned short crc=0;
    
    sendline(ZModem::ZBIN);
    zsendline(type); 
    crc = updcrc(type, crc);

    for (n=4; --n >= 0; ++hdr) {
      zsendline(*hdr);
      crc = updcrc((0377& *hdr), crc);
    }
    crc = updcrc(0,crc);
    crc = updcrc(0,crc);
    zsendline(crc>>8);
    zsendline(crc);
  }
  if (type != ZModem::ZDATA)
    ZModem::flushmo();
}


/* Send ZMODEM HEX header hdr of type type */
void ZModem::zshhdr(int type,char *hdr)
{
  int n;
  unsigned short crc=0;

  vfile(F("zshhdr: %s %lx"), frametypes[type+FTOFFSET], rclhdr(hdr));
  sendline(ZModem::ZPAD);
  sendline(ZModem::ZPAD);
  sendline(ZModem::ZDLE);
  sendline(ZModem::ZHEX);
  zputhex(type);
  Crc32tx = 0;

  crc = updcrc(type, crc);
  for (n=4; --n >= 0; ++hdr) {
    zputhex(*hdr); 
    crc = updcrc((0377 & *hdr), crc);
  }
  crc = updcrc(0,crc);
  crc = updcrc(0,crc);
  zputhex(crc>>8); 
  zputhex(crc);

  /* Make it printable on remote machine */
  sendline(015); 
  sendline(0212);
  /*
         * Uncork the remote in case a fake XOFF has stopped data flow
   */
  if (type != ZModem::ZFIN && type != ZModem::ZACK)
    sendline(021);
  ZModem::flushmo();
}

/// @brief update CRC (cyclic redundancy check), the error correction used by ZModem
/// @param cp character pointer (new byte)
/// @param crc running crc
/// @return crc (updated with new byte)
unsigned short ZModem::updcrc(uint8_t cp, uint16_t& crc) {
  crc = (crctab[((crc >> 8) & 255)] ^ (crc << 8)) ^ cp;
  return crc;
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

void ZModem::zsdata(char *buf,int length,int frameend)
{

  // vfile(F("zsdata: %d %s"), length, Zendnames[(frameend-ZCRCE)&3]);

  if (Crc32tx) {
    int c;
    unsigned long crc;
  
    crc = 0xFFFFFFFFL;
    for (;--length >= 0; ++buf) {
      c = *buf & 0377;
      if (c & 0140)
        sendline(lastsent = c);
      else
        zsendline(c);
      crc = UPDC32(c, crc);
    }
    sendline(ZModem::ZDLE);
    sendline(frameend);
    crc = UPDC32(frameend, crc);
  
    crc = ~crc;
    for (length=4; --length >= 0;) {
      zsendline((int)crc);  
      crc >>= 8;
    }
  } else {
    // Serial.print("not crc32 "); delay(5); // good
    unsigned short crc = 0;
    for (;--length >= 0; ++buf) {
      zsendline(*buf);

      crc = updcrc((0377 & *buf), crc);
    }

    sendline(ZModem::ZDLE);
    sendline(frameend);
    crc = updcrc(frameend, crc);

    crc = updcrc(0, crc);
    crc = updcrc(0, crc);

    zsendline(crc>>8); 
    zsendline(crc);
  }
  if (frameend == ZModem::ZCRCW) {
    sendline(ZModem::XON);
    ZModem::flushmo();
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

  if (Rxframeind == ZModem::ZBIN32) {
    unsigned long crc;
  
    crc = 0xFFFFFFFFL;  
    Rxcount = 0;  
    end = buf + length;
    while (buf <= end) {
      if ((c = zdlread()) & ~0377) {
  crcfoo32:
        switch (c) {
        case ZModem::GOTCRCE:
        case ZModem::GOTCRCG:
        case ZModem::GOTCRCQ:
        case ZModem::GOTCRCW:
          d = c;  
          c &= 0377;
          crc = UPDC32(c, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo32;
          crc = UPDC32(c, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo32;
          crc = UPDC32(c, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo32;
          crc = UPDC32(c, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo32;
          crc = UPDC32(c, crc);
          if (crc != 0xDEBB20E3) {
            zperr(badcrc);
            return ERROR;
          }
          Rxcount = length - (end - buf);
          vfile(F("zrdat32: %d %s"), Rxcount,
          Zendnames[(d-GOTCRCE)&3]);
          return d;
        case ZModem::GOTCAN:
          zperr("Sender Canceled");
          return ZModem::ZCAN;
        case TIMEOUT:
          zperr("TIMEOUT");
          return c;
        default:
          zperr("Bad data subpacket");
          return c;
        }
      }
      *buf++ = c;
      crc = UPDC32(c, crc);
    }
    zperr("Data subpacket too long");
    return ERROR;
  } else {
    unsigned short crc;

    crc = Rxcount = 0;  
    end = buf + length;
    while (buf <= end) {
      if ((c = zdlread()) & ~0377) {
  crcfoo16:
        switch (c) {
        case ZModem::GOTCRCE:
        case ZModem::GOTCRCG:
        case ZModem::GOTCRCQ:
        case ZModem::GOTCRCW:
          crc = updcrc((d=c)&0377, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo16;
          crc = updcrc(c, crc);
          if ((c = zdlread()) & ~0377)
            goto crcfoo16;
          crc = updcrc(c, crc);
          if (crc & 0xFFFF) {
            zperr(badcrc);
            return ERROR;
          }
          Rxcount = length - (end - buf);
          vfile(F("zrdata: %d  %s"), Rxcount,
          Zendnames[(d-GOTCRCE)&3]);
          return d;
        case ZModem::GOTCAN:
          zperr("Sender Canceled");
          return ZModem::ZCAN;
        case TIMEOUT:
          zperr("TIMEOUT");
          return c;
        default:
          zperr("Bad data subpacket");
          return c;
        }
      }
      *buf++ = c;
      crc = updcrc(c, crc);
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
  ZSERIAL.setTimeout(Rxtimeout * 100);
  c = readline(Rxtimeout);
  ZSERIAL.setTimeout(TYPICAL_SERIAL_TIMEOUT);
  
  switch (c) {
  case RCDO:
  case TIMEOUT:
    goto fifi;
  case ZModem::CAN:
gotcan:
    if (--cancount <= 0) {
      c = ZModem::ZCAN;
      goto fifi;
    }
    switch (c = readline(1)) {
    case TIMEOUT:
      goto again;
    case ZModem::ZCRCW:
      c = ERROR;
      /* **** FALL THRU TO **** */
    case RCDO:
      goto fifi;
    default:
      break;
    case ZModem::CAN:
      if (--cancount <= 0) {
        c = ZModem::ZCAN;
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
    if (eflag && ((c &= 0177) & 0140))
      bttyout(c);
    else if (eflag > 1)
      bttyout(c);
#ifdef UNIX
    fflush(stderr);
#endif
    goto startover;
  case ZModem::ZPAD|0200:         /* This is what we want. */
    Not8bit = c;
  case ZModem::ZPAD:              /* This is what we want. */
    break;
  }
  cancount = 5;
splat:
  switch (c = noxrd7()) {
  case ZModem::ZPAD:
    goto splat;
  case RCDO:
  case TIMEOUT:
    goto fifi;
  default:
    goto agn2;
  case ZModem::ZDLE:              /* This is what we want. */
    break;
  }

  switch (c = noxrd7()) {
  case RCDO:
  case TIMEOUT:
    goto fifi;
  case ZModem::ZBIN:
    Rxframeind = ZModem::ZBIN;
    Crc32rx = FALSE;
    c =  zrbhdr(hdr);
    break;
  case ZModem::ZBIN32:
    Crc32rx = Rxframeind = ZModem::ZBIN32;
    c =  zrbhdr32(hdr);
    break;
  case ZModem::ZHEX:
    Rxframeind = ZModem::ZHEX;
    Crc32rx = FALSE;
    c =  zrhhdr(hdr);
    break;
  case ZModem::CAN:
    goto gotcan;
  default:
    goto agn2;
  }
  Rxpos = hdr[ZP3] & 0377;
  Rxpos = (Rxpos<<8) + (hdr[ZP2] & 0377);
  Rxpos = (Rxpos<<8) + (hdr[ZP1] & 0377);
  Rxpos = (Rxpos<<8) + (hdr[ZP0] & 0377);
fifi:

  switch (c) {
  case ZModem::GOTCAN:
    c = ZModem::ZCAN;
    /* **** FALL THRU TO **** */
  case ZModem::ZNAK:
  case ZModem::ZCAN:
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

/* Receive a binary style header (type and position) */
int ZModem::zrbhdr(char *hdr)
{
  int c, n;
  unsigned short crc=0;

  if ((c = zdlread()) & ~0377)
    return c;
  Rxtype = c;
  crc = updcrc(c, crc);

  for (n=4; --n >= 0; ++hdr) {
    if ((c = zdlread()) & ~0377)
      return c;
    crc = updcrc(c, crc);
    *hdr = c;
  }
  if ((c = zdlread()) & ~0377)
    return c;
  crc = updcrc(c, crc);
  if ((c = zdlread()) & ~0377)
    return c;
  crc = updcrc(c, crc);
  if (crc & 0xFFFF) {        
    zperr(badcrc);
    return ERROR;
  }
#ifdef ZMODEM
  Protocol = ZMODEM;
#endif
//  Zmodem = 1;
  return Rxtype;
}



/* Receive a binary style header (type and position) with 32 bit FCS */
int ZModem::zrbhdr32(char *hdr)
{
  int c, n;
  unsigned long crc;

  if ((c = zdlread()) & ~0377)
    return c;
  Rxtype = c;
  crc = 0xFFFFFFFFL; 
  crc = UPDC32(c, crc);
#ifdef DEBUGZ
  vfile(F("zrbhdr32 c=%X  crc=%lX"), c, crc);
#endif

  for (n=4; --n >= 0; ++hdr) {
    if ((c = zdlread()) & ~0377)
      return c;
    crc = UPDC32(c, crc);
    *hdr = c;
#ifdef DEBUGZ
    vfile(F("zrbhdr32 c=%X  crc=%lX"), c, crc);
#endif
  }
  for (n=4; --n >= 0;) {
    if ((c = zdlread()) & ~0377)
      return c;
    crc = UPDC32(c, crc);
#ifdef DEBUGZ
    vfile(F("zrbhdr32 c=%X  crc=%lX"), c, crc);
#endif
  }
  if (crc != 0xDEBB20E3) {
    zperr(badcrc);
    return ERROR;
  }
#ifdef ZMODEM
  Protocol = ZMODEM;
#endif
//  Zmodem = 1;
  return Rxtype;
}




/* Receive a hex style header (type and position) */
int ZModem::zrhhdr(char *hdr)
{
  int c;
  unsigned short crc=0;
  int n;

  if ((c = zgethex()) < 0)
    return c;
  Rxtype = c;
  crc = updcrc(c, crc);

  for (n=4; --n >= 0; ++hdr) {
    if ((c = zgethex()) < 0)
      return c;
    crc = updcrc(c, crc);
    *hdr = c;
  }
  if ((c = zgethex()) < 0)
    return c;
  crc = updcrc(c, crc);
  if ((c = zgethex()) < 0)
    return c;
  crc = updcrc(c, crc);
  if (crc & 0xFFFF) {
    zperr(badcrc); 
    return ERROR;
  }
  switch ( c = readline(1)) {
  case 0215:
    Not8bit = c;
    /* **** FALL THRU TO **** */
  case 015:
    /* Throw away possible cr/lf */
    switch (c = readline(1)) {
    case 012:
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
  sendline(pgm_read_byte(digits+((c&0xF0)>>4)));
  sendline(pgm_read_byte(digits+((c)&0xF)));
}

void ZModem::sendline(char c) {
  sendline((int) c);
}

void ZModem::sendline(int c){
  // Check if buffer is more than half full
  if (ZSERIAL.availableForWrite() < (SERIAL_TX_BUFFER_SIZE / 2)) {
    ZSERIAL.flush(); // Wait for outgoing data to complete
  }
  ZSERIAL.write(c);
}


/*
 * Send character c with ZMODEM escape sequence encoding.
 *  Escape XON, XOFF. Escape CR following @ (Telenet net escape)
 */
void ZModem::zsendline(char c) {
  zsendline((int)c);
}

void ZModem::zsendline(int c)
{
  /* check for non-control characters */
  if (c & 0140)
    sendline(lastsent = c);

  else {
    switch (c &= 0377) {
    case ZModem::ZDLE:
      sendline(ZModem::ZDLE);
      sendline (lastsent = (c ^= 0100));
      break;
    case 015: // CR
    case 0215: //
      if (!Zctlesc && (lastsent & 0177) != '@') {
        sendline(lastsent = c);
      }
        break;

      /* **** FALL THRU TO **** */
    case 020: // DLE
    case 021: // DC1
    case 023: // DC3
    case 0220: // hex 8D
    case 0221: // '
    case 0223: // "
      sendline(ZModem::ZDLE);
      c ^= 0100;
// sendit:
      sendline(lastsent = c);
      break;

    default:
      if (Zctlesc && ! (c & 0140)) {
        sendline(ZModem::ZDLE);
        c ^= 0100;
      }
      sendline(lastsent = c);
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
  case 023:
  case 0223:
  case 021:
  case 0221:
    goto again;
  default:
    if (Zctlesc && !(c & 0140)) {
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
    return 0177;
  case ZModem::ZRUB1:
    return 0377;
  case 023:
  case 0223:
  case 021:
  case 0221:
    goto again2;
  default:
    if (Zctlesc && ! (c & 0140)) {
      goto again2;
    }
    if ((c & 0140) ==  0100)
      return (c ^ 0100);
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
    switch (c &= 0177) {
    case XON:
    case XOFF:
      continue;
    default:
      if (Zctlesc && !(c & 0140))
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

  l = (hdr[ZP3] & 0377);
  l = (l << 8) | (hdr[ZP2] & 0377);
  l = (l << 8) | (hdr[ZP1] & 0377);
  l = (l << 8) | (hdr[ZP0] & 0377);
  return l;
}
#endif


/*
 * Purge the modem input queue of all characters
 */
void ZModem::purgeline(void)
{
  while(ZSERIAL.available())ZSERIAL.read();
}

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

void ZModem::flushmo(void)
{
  ZSERIAL.flush();
}


/* send cancel string to get the other end to shut up */
void ZModem::canit(void)
{
  for (int i=0; i < 10; ++i) {
    ZSERIAL.write(24);
  }
  for (int i=0; i < 10; ++i) {
    ZSERIAL.write(8);
  }
  ZSERIAL.flush();
}

int ZModem::readline(int timeout) {
  long then;
  unsigned char c;

  then = millis();
  while(ZSERIAL.available() < 1) {
    if(millis() - then > (unsigned int)timeout*10UL) {
      //DSERIAL.println("");
      return(TIMEOUT);
    }
  }
  c = ZSERIAL.read();
  //DSERIAL.write(c);
  //DSERIAL.print(" ");
  return(c);
}


/* End of zm.c */
#endif

