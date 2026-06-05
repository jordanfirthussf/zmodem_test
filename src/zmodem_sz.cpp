/*
 * A program for Unix to send files and commands to computers running
 *  Professional-YAM, PowerCom, YAM, IMP, or programs supporting Y/XMODEM.
 *
 *  Sz uses buffered I/O to greatly reduce CPU time compared to UMODEM.
 *
 *  USG UNIX (3.0) ioctl conventions courtesy Jeff Martin
 */


#include "zmodem_config.h"
#include "zmodem_zm.h"
#include "zmodem_sz.h"


#include "zmodem.h"
//#include "zmodem_crc16.cpp"

#include <stdio.h>

ZModem::ZModem() {}

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

//FILE *in;

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
//int Filcnt=0;           /* count of number of files opened */
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

// Pete (El Supremo)
void purgeline(void);
void canit(void);

//void zperr();
// #define zperr(a, ... )

int sendzsinit(void);
// void saybibi(void);
//void bttyout(int c);
int zsendcmd(char *buf, int blen);


// #ifndef ARDUINO
// FILE *fout;
// #else
// extern FsFile fout;
// #endif

int ZModem::wcs(const char *oname)
{
//  char name[PATHLEN];

//  strcpy(name, oname);
  
  Eofseen = 0;  
//  vpos = 0;
  switch (ZModem::wctxpn(oname)) {
   case ERROR:
    DSERIAL_PRINT("error");
     return ERROR;
   case ZModem::ZSKIP:
    DSERIAL_PRINT("ok");
     return OK;
  }

//  ++Filcnt;
  if(!Zmodem && ZModem::wctx(fout.fileSize())==ERROR) {
    return ERROR;
  }
    return 0;
}


/*
 * generate and transmit pathname block consisting of
 *  pathname (null terminated),
 *  file length, mode time and file mode in octal
 *  as provided by the Unix fstat call.
 *  N.B.: modifies the passed name, may extend it!
 */
int ZModem::wctxpn(const char *name)
{

  char *p, *q;

  strcpy(txbuf,name);
  p = q = txbuf + strlen(txbuf)+1;
  //Pete (El Supremo) fix bug - was 1024, should be TXBSIZE??
  while (q < (txbuf + TXBSIZE)) {
    *q++ = 0;
  }
//  if (!Ascii && (in!=stdin) && *name && fstat(fileno(in), &f)!= -1)
  // if (!Ascii)
    // I will have to figure out how to convert the uSD date/time format to a UNIX epoch
    // sprintf(p, "%lu %lo %o 0 %d %ld", fout.fileSize(), 0L,0600, Filesleft, Totalleft);
// Avoid sprintf to save memory for small boards.  This sketch doesn't know what time it is anyway
    // ultoa(fout.fileSize(), p, 10);
    // strcat_P(p, PSTR(" 0 0 0 "));
    // q = p + strlen(p);
    // ultoa(Filesleft, q, 10);
    // strcat_P(q, PSTR(" "));
    // q = q + strlen(q);
    // ultoa(Totalleft, q, 10);

  Totalleft -= fout.fileSize();

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
    txbuf[127] = (fout.fileSize() + 127) >>7;
    txbuf[126] = (fout.fileSize() + 127) >>15;
  }


  return ZModem::zsendfile(txbuf, 1+strlen(p)+(p-txbuf));
}


int ZModem::wctx(long flen)
{
  int thisblklen;
  int sectnum, attempts, firstch;
  long charssent;

DSERIAL_PRINTLN("\nwctx");

  charssent = 0;
  firstsec=TRUE;
  thisblklen = blklen;

  while ((firstch=readline(Rxtimeout))!= ZModem::NAK && firstch != WANTCRC
    && firstch != WANTG && firstch!=TIMEOUT && firstch!= ZModem::CAN)
    ;
  if (firstch== ZModem::CAN) {
    zperr("Receiver CANcelled");
    return ERROR;
  }
  if (firstch==WANTCRC)
    Crcflg=TRUE;
  if (firstch==WANTG)
    Crcflg=TRUE;
  sectnum=0;
  for (;;) {
    if (flen <= (charssent + 896L))
      thisblklen = 128;
    if ( !ZModem::filbuf(txbuf, thisblklen))
      break;
    if (ZModem::wcputsec(txbuf, ++sectnum, thisblklen)==ERROR)
      return ERROR;
    charssent += thisblklen;
  }
  //fclose(in);
  fout.close();
  attempts=0;
  do {
    purgeline();
    sendline(ZModem::EOT);
    ++attempts;
  }
  while ((firstch=(readline(Rxtimeout)) != ZModem::ACK) && attempts < Tx_RETRYMAX);
  if (attempts == Tx_RETRYMAX) {
    zperr("No ACK on EOT");
    return ERROR;
  }
  else
    return OK;
}



int ZModem::wcputsec(char *buf,int sectnum,int cseclen)
{
  int checksum, wcj;
  char *cp;
  uint16_t oldcrc;
  int firstch;
  uint8_t attempts;

  firstch=0;      /* part of logic to detect CAN CAN */

  if (Verbose>2)
    fprintf(stderr, "Sector %3d %2dk\n", Totsecs, Totsecs/8 );
  else if (Verbose>1)
    fprintf(stderr, "\rSector %3d %2dk ", Totsecs, Totsecs/8 );
  for (attempts=0; attempts <= Tx_RETRYMAX; attempts++) {
    Lastrx= firstch;
    sendline(cseclen==1024?ZModem::STX:ZModem::SOH);
    sendline(sectnum);
    sendline(-sectnum -1);
    oldcrc=checksum=0;
    for (wcj=cseclen,cp=buf; --wcj>=0; ) {
      sendline(*cp);
      oldcrc=updcrc((0377& *cp), oldcrc);
      checksum += *cp++;
    }
    if (Crcflg) {
      oldcrc = updcrc(0, oldcrc);
      oldcrc = updcrc(0,oldcrc);
      sendline((int)oldcrc>>8);
      sendline((int)oldcrc);
    }
    else
      sendline(checksum);

    if (Optiong) {
      firstsec = FALSE;
      return OK;
    }
    firstch = readline(Rxtimeout);
gotnak:
    switch (firstch) {
    case ZModem::CAN:
      if(Lastrx == ZModem::CAN) {
cancan:
        zperr("Cancelled");
        return ERROR;
      }
      break;
    case TIMEOUT:
      zperr("Timeout on sector ACK");
      continue;
    case WANTCRC:
      if (firstsec)
        Crcflg = TRUE;
    case ZModem::NAK:
      zperr("NAK on sector");
      continue;
    case ZModem::ACK:
      firstsec=FALSE;
      Totsecs += (cseclen>>7);
      return OK;
    case ERROR:
      zperr("Got burst for sector ACK");
      break;
    default:
      zperr("Got %02x for sector ACK", firstch);
      break;
    }
    for (;;) {
      Lastrx = firstch;
      if ((firstch = readline(Rxtimeout)) == TIMEOUT)
        break;
      if (firstch == ZModem::NAK || firstch == WANTCRC)
        goto gotnak;
      if (firstch == ZModem::CAN && Lastrx == ZModem::CAN)
        goto cancan;
    }
  }
  zperr("Retry Count Exceeded");
  return ERROR;
}



/* fill buf with count chars padding with ^Z for CPM */
int ZModem::filbuf(char *buf,int count)
{
  int c, m;

DSERIAL_PRINTLN("\nfilbuf");

  if ( !Ascii) {
//    m = read(fileno(in), buf, count);
    m = fout.read(buf, count);
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
  while((c = fout.read()) != -1) {
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
int ZModem::zfilbuf(void)
{
  int n;

// This code works:
//  n = fread(txbuf, 1, blklen, in);
  n = fout.read(txbuf,blklen);

  if (n < blklen)
    Eofseen = 1;
  return n;
}



/* Send file name and related info */
int ZModem::zsendfile(char *buf, int blen)
{
  int c;
  unsigned long crc;

DSERIAL_PRINTLN(F("\nzsendfile"));

  while (true) {
    Txhdr[ZF0] = Lzconv;    /* file conversion request */
    Txhdr[ZF1] = Lzmanag;   /* file management request */
    if constexpr (Lskipnocor){
      Txhdr[ZF1] |= ZModem::ZF1_ZMSKNOLOC;
      }
    Txhdr[ZF2] = Lztrans;   /* file transport request */
    Txhdr[ZF3] = 0; // should be 0 !!
    zsbhdr(ZModem::ZFILE, Txhdr);
    zsdata(buf, blen, ZModem::ZCRCW);
again:
    c = zgethdr(Rxhdr, 1);
    switch (c) {
    case ZModem::ZRINIT:
      while ((c = readline(50)) > 0)
        if (c == ZModem::ZPAD) {
          goto again;
        }
      /* **** FALL THRU TO **** */
    default:
      continue;
    case ZModem::ZCAN:
    case TIMEOUT:
    case ZModem::ZABORT:
    case ZModem::ZFIN:
DSERIAL_PRINTLN(F("\nzsendfile - ZFIN"));

      return ERROR;
    case ZModem::ZCRC:
      crc = 0xFFFFFFFFL;
      if (Canseek >= 0) {
        fout.seekSet(0);
        while (((c = fout.read()) != -1)) // && --Rxpos)
          crc = UPDC32(c, crc);
        crc = ~crc;
//        clearerr(in);   /* Clear EOF */
//>>> Need to implement the seek
//        fseek(in, 0L, 0);
          fout.seekSet(0);
      }
      stohdr(crc);
      zsbhdr(ZModem::ZCRC, Txhdr);
      goto again;
    case ZModem::ZSKIP:
      fout.close();
      //fclose(in);
DSERIAL_PRINTLN(F("\nzsendfile - ZSKIP"));
      return c;
    case ZModem::ZRPOS:
      /*
       * Suppress zcrcw request otherwise triggered by
       * lastyunc==bytcnt
       */
//>>> Need to implement the seek
//      if (Rxpos && fseek(in, Rxpos, 0))
      if(Rxpos && !fout.seekSet(Rxpos))
        return ERROR;
      Lastsync = (bytcnt = Txpos = Rxpos) -1;
      int ret = ZModem::zsendfdata();
DSERIAL_PRINT(F("\nzsendfile - exit - "));
DSERIAL_PRINTLN(ret);
      return(ret);
    }
  } //
}



/* Send the data in the file */
int ZModem::zsendfdata(void)
{
  int c, n;
  uint8_t e;
  int newcnt;
  uint8_t junkcount;          /* Counts garbage chars received by TX */

DSERIAL_PRINT(F("\nzsendfdata: "));
DSERIAL_PRINT(F("number = "));
DSERIAL_PRINT(Filesleft+1);
DSERIAL_PRINT(F("   length = "));
DSERIAL_PRINTLN(Totalleft);
  Lrxpos = 0;
  junkcount = 0;
  Beenhereb4 = FALSE;
somemore:
  //if (setjmp(intrjmp)) {
  if (0) {
waitack:
    junkcount = 0;
    c = ZModem::getinsync(0);
gotack:
    switch (c) {
    default:
    case ZModem::ZCAN:
      fout.close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - error - 1"));
      return ERROR;
    case ZModem::ZSKIP:
      fout.close();
      //fclose(in);
      return c;
    case ZModem::ZACK:
    case ZModem::ZRPOS:
      break;
    case ZModem::ZRINIT:
      return OK;
    }
#ifdef READCHECK
    /*
                 * If the reverse channel can be tested for data,
     *  this logic may be used to detect error packets
     *  sent by the receiver, in place of setjmp/longjmp
     *  rdchk(fdes) returns non 0 if a character is available
     */
    while (ZSERIAL.available()) {
#ifdef SV
      switch (checked)
#else
        switch (readline(1))
#endif
        {
        case ZModem::CAN:
        case ZModem::ZPAD:
          c = ZModem::getinsync(1);
          goto gotack;
        case ZModem::XOFF:              /* Wait a while for an XON */
        case ZModem::XOFF |0x80:
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
  zsbhdr(ZModem::ZDATA, Txhdr);

DSERIAL_PRINTLN("zsendfdata - 2");

  do {
    n = ZModem::zfilbuf();
// AHA - it reads the 18 chars here
DSERIAL_PRINTLN(n);
    if (Eofseen)
      e = ZModem::ZCRCE;
    else if (junkcount > 3)
      e = ZModem::ZCRCW;
    else if (bytcnt == Lastsync)
      e = ZModem::ZCRCW;
    else if (Rxbuflen && (newcnt -= n) <= 0)
      e = ZModem::ZCRCW;
    else if (Txwindow && (Txwcnt += n) >= Txwspac) {
      Txwcnt = 0;
      e = ZModem::ZCRCQ;
    }
    else
      e = ZModem::ZCRCG;
    if (Verbose>1)
      fprintf(stderr, "\r%7ld ZMODEM%s    ",Txpos, Crc32t?" CRC-32":"");
    zsdata(txbuf, n, e);
    bytcnt = Txpos += n;
    if (e == ZModem::ZCRCW)
      goto waitack;
#ifdef READCHECK
    /*
                 * If the reverse channel can be tested for data,
     *  this logic may be used to detect error packets
     *  sent by the receiver, in place of setjmp/longjmp
     *  rdchk(fdes) returns non 0 if a character is available
     */
//    fflush(stdout);
    while (ZSERIAL.available()) {
#ifdef SV
      switch (checked)
#else
        switch (readline(1))
#endif
        {
        case ZModem::CAN:
        case ZModem::ZPAD:
          c = ZModem::getinsync(1);
          if (c == ZModem::ZACK)
            break;
#ifdef TCFLSH
          ioctl(iofd, TCFLSH, 1);
#endif
          /* zcrce - dinna wanna starta ping-pong game */
          zsdata(txbuf, 0, ZModem::ZCRCE);
          goto gotack;
        case ZModem::XOFF:              /* Wait a while for an XON */
        case ZModem::XOFF|0200:
          readline(100);
        default:
          ++junkcount;
        }
    }
#endif  /* READCHECK */

  } while (!Eofseen);

DSERIAL_PRINTLN("zsendfdata - 4");

//  if ( !Fromcu)
//    signal(SIGINT, SIG_IGN);

  for (;;) {
    stohdr(Txpos);
    zsbhdr(ZModem::ZEOF, Txhdr);
    switch (ZModem::getinsync(0)) {
    case ZModem::ZACK:
DSERIAL_PRINTLN(F("zsendfdata - ZAK"));
      continue;
    case ZModem::ZRPOS:
DSERIAL_PRINTLN(F("zsendfdata - ZRPOS"));
      goto somemore;
    case ZModem::ZRINIT:
DSERIAL_PRINTLN(F("zsendfdata - OK"));
      return OK;
    case ZModem::ZSKIP:
      fout.close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - ZSKIP"));
      return c;
    default:
      fout.close();
      //fclose(in);
DSERIAL_PRINTLN(F("zsendfdata - error - 2"));
      return ERROR;
    }
  }
}




/*
 * Respond to receiver's complaint, get back in sync with receiver
 */
int ZModem::getinsync(int flag)
{
  int c;

  for (;;) {
    if (Test) {
DSERIAL_PRINTLN(F("***** Signal Caught *****"));
      Rxpos = 0;
      c = ZModem::ZRPOS;
    }
    else
      c = zgethdr(Rxhdr, 0);
    switch (c) {
    case ZModem::ZCAN:
    case ZModem::ZABORT:
    case ZModem::ZFIN:
    case TIMEOUT:
DSERIAL_PRINTLN(F("getinsync - timeout"));
      return ERROR;
    case ZModem::ZRPOS:
      /* ************************************* */
      /*  If sending to a buffered modem, you  */
      /*   might send a break at this point to */
      /*   dump the modem's buffer.            */
//      clearerr(in);   /* In case file EOF seen */
//      if (fseek(in, Rxpos, 0)) {
      // seekSet returns true on success
      if(!fout.seekSet(Rxpos)) {
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
    case ZModem::ZACK:
      Lrxpos = Rxpos;
      if (flag || Txpos == Rxpos)
        return ZModem::ZACK;
      continue;
    case ZModem::ZRINIT:
    case ZModem::ZSKIP:
      fout.close();      
      //fclose(in);
      return c;
    case ERROR:
    default:
      zsbhdr(ZModem::ZNAK, Txhdr);
      continue;
    }
  }
}

// Dylan (monte_carlo_ecm, bitflipper, etc.) - I added this simple ZRQINIT string to trigger
// terminal program's receive auto start feature.  This was missing in the code as I found it.
// All the terminal applications I tried would receive files anyway if I manually started
// the download, but sending the ZRQINIT is the right way to initiate a ZMODEM transfer
// according to the protocol documentation.

#define ZRQINIT_STR F(\
  "\x2a\x2a\x18\x42\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x30\x0d\x0a\x11")
// **␘B (14 zeros) CR-LF-DC1
// <ZPAD><ZPAD><ZDLE><ZHEX>)(14 zeros) CR-LF-<XON>

void ZModem::sendzrqinit(void)
{
  ZSERIAL.print(ZRQINIT_STR);
}

/* Say "bibi" to the receiver, try to do it cleanly */
void ZModem::saybibi() {
  for (;;) {
    stohdr(0L);             /* CAF Was zsbhdr - minor change */
    zshhdr(ZModem::ZFIN, Txhdr);    /*  to make debugging easier */
    switch (zgethdr(Rxhdr, 0)) {
    case ZModem::ZFIN:
      sendline('O'); 
      sendline('O'); 
      ZModem::flushmo();
    case ZModem::ZCAN:
    case TIMEOUT:
      return;
    } // switch
  } // for
} // saybibi

