#ifndef zmodem_h
#define zmodem_h

#include "Arduino.h"

// zmodem constants



// #include "zmodem_config.h"
// #include "zmodem_fixes.h"
// #include <SdFat.h>

/*
 *   Z M O D E M . H     Manifest constants for ZMODEM
 *    application to application file transfer protocol
 *    05-23-87  Chuck Forsberg Omen Technology Inc
 */




/* zdlread return values (internal) */
/* -1 is general error, -2 is timeout */
#define GOTOR 0400
#define GOTCRCE (ZCRCE|GOTOR)   /* ZDLE-ZCRCE received */
#define GOTCRCG (ZCRCG|GOTOR)   /* ZDLE-ZCRCG received */
#define GOTCRCQ (ZCRCQ|GOTOR)   /* ZDLE-ZCRCQ received */
#define GOTCRCW (ZCRCW|GOTOR)   /* ZDLE-ZCRCW received */
#define GOTCAN  (GOTOR|030)     /* CAN*5 seen */

/* Byte positions within header array */
#define ZF0     3       /* First flags byte */
#define ZF1     2
#define ZF2     1
#define ZF3     0
#define ZP0     0       /* Low order 8 bits of position */
#define ZP1     1
#define ZP2     2
#define ZP3     3       /* High order 8 bits of file position */


/* Bit Masks for ZSINIT flags byte ZF0 */
#define TESCCTL 0100    /* Transmitter expects ctl chars to be escaped */
#define TESC8   0200    /* Transmitter expects 8th bit to be escaped */

/* Parameters for ZFILE frame */


/* Transport options, one of these in ZF2 */
#define ZTLZW   1       /* Lempel-Ziv compression */
#define ZTCRYPT 2       /* Encryption */
#define ZTRLE   3       /* Run Length encoding */
/* Extended options for ZF3, bit encoded */
#define ZXSPARS 64      /* Encoding for sparse file operations */

/* Parameters for ZCOMMAND frame ZF0 (otherwise 0) */
#define ZCACK1  1       /* Acknowledge, then do command */


class ZModem {
    public:

    enum FrameType
    {
        /* Frame types (see array "frametypes" in zm.c) */
        ZRQINIT   = 0 ,      /* Request receive init */
        ZRINIT    = 1 ,      /* Receive init */
        ZSINIT    = 2 ,      /* Send init sequence (optional) */
        ZACK      = 3 ,      /* ACK to above */
        ZFILE     = 4 ,      /* File name from sender */
        ZSKIP     = 5 ,      /* To sender: skip this file */
        ZNAK      = 6 ,      /* Last packet was garbled */
        ZABORT    = 7 ,      /* Abort batch transfers */
        ZFIN      = 8 ,      /* Finish session */
        ZRPOS     = 9 ,      /* Resume data trans at this position */
        ZDATA     = 10,      /* Data packet(s) follow */
        ZEOF      = 11,      /* End of file */
        ZFERR     = 12,      /* Fatal Read or Write error Detected */
        ZCRC      = 13,      /* Request for file CRC and response */
        ZCHALLENGE= 14,      /* Receiver's Challenge */
        ZCOMPL    = 15,      /* Request is complete */
        ZCAN      = 16,      /* Other end canned session with CAN*5 */
        ZFREECNT  = 17,      /* Request for free bytes on filesystem */
        ZCOMMAND  = 18,      /* Command from sending program */
        ZSTDERR   = 19,      /* Output to standard error, data follows */
    }; // end enum FrameType

    enum ZmodemConstants {
        ZPAD    = 0x2a,      /* 052 Padding character begins frames */
        ZDLE    = 0x18,         /* (␘, \x18 Zmodem escape - `ala BISYNC DLE */
        ZDLEE   = 0x58,     //(ZDLE^0100)  /* Escaped ZDLE as transmitted */
        ZBIN    = 0x41,      /* (A) Binary frame indicator */
        ZHEX    = 0x42,      /* (B) HEX frame indicator */
        ZBIN32  = 0x43,      /* (C) Binary frame with 32 bit FCS */
        ZBINR32	= 0x44,      /* run length encoded binary frame (CRC32) */
        ZVBIN	= 0x61,     /* binary frame indicator (CRC16) */
        ZVHEX	= 0x62,     /* hex frame indicator */
        ZVBIN32	= 0x63,     /* binary frame indicator (CRC32) */
        ZVBINR32= 0x64,     /* run length encoded binary frame (CRC32) */
        ZRESC	= 0x7e,     /* run length encoding flag / escape character */
    }; // end ZmodemConstants

    enum ZDLECodes {
        ZCRCE   = 0x68, 		/* (h) CRC next, frame ends, header packet follows */
        ZCRCG   = 0x69, 		/* (i) CRC next, frame continues nonstop */
        ZCRCQ   = 0x6a, 		/* (j) CRC next, frame continuous, ZACK expected */
        ZCRCW   = 0x6b, 		/* (k) CRC next, frame ends,       ZACK expected */
        ZRUB0   = 0x6c, 		/* (l) translate to rubout 0x7f */
        ZRUB1   = 0x6d, 		/* (m) translate to rubout 0xff */
    }; // end ZDLECodes

    enum ASCIIConstants    {
        SOH	    = 	0x01,
        STX	    = 	0x02,
        EOT	    = 	0x04,
        ENQ	    = 	0x05,
        ACK	    = 	0x06,
        LF		=   0x0a,
        CR		=   0x0d,
        XON	    = 	0x11,
        XOFF    = 	0x13,
        NAK	    = 	0x15,
        CAN	    = 	0x18,
    }; // end enum ASCIIConstants

    enum ZRInitFrame    {
        // RX capabilities
        /* Bit Masks for ZRINIT flags byte ZF0 */
        ZF0_CANFDX  = 0x01 ,      /* Rx can send and receive true full duplex */
        ZF0_CANOVIO = 0x02 ,      /* Rx can receive data during disk I/O */
        ZF0_CANBRK  = 0x04 ,      /* Rx can send a break signal */
        ZF0_CANCRY  = 0x08,      /* Receiver can decrypt DON'T USE */
        ZF0_CANLZW  = 0x10,      /* Receiver can uncompress DON'T USE */
        ZF0_CANFC32 = 0x20,      /* Receiver can use 32 bit Frame Check */
        ZF0_ESCCTL  = 0x40,     /* Receiver expects ctl chars to be escaped */
        ZF0_ESC8    = 0x80,     /* Receiver expects 8th bit to be escaped */

        ZF1_CANVHDR = 0x01,     /* variable headers OK */
    }; // end enum ZRInitFrame

    enum ZSInitFrame  {
        ZF0_TESCCTL =	0x40,	/* Transmitter expects ctl chars to be escaped */
        ZF0_TESC8   =	0x80,	/* Transmitter expects 8th bit to be escaped */

        ZATTNLEN	=   0x20,	/* Max length of attention string */
        ALTCOFF		=	ZF1		/* Offset to alternate canit string, 0 if not used */

    }; // end ZSInitFrame


    enum ZFileFrame  {
        // Conversion options one of these in ZF0
        ZF0_ZCBIN     = 0x01,       /* Binary transfer - inhibit conversion */
        ZF0_ZCNL      = 0x02,       /* Convert NL to local end of line convention */
        ZF0_ZCRESUM   = 0x03,       /* Resume interrupted file transfer */

        /* Management include options, one of these ored in ZF1 */
        ZF1_ZMSKNOLOC   =   0x80,    /* Skip file if not present at rx */

        ZF1_ZMMASK      =   0x1f,     /* Mask for the choices below */
        ZF1_ZMNEWL      =   1,       /* Transfer if source newer or longer */
        ZF1_ZMCRC       =   2,       /* Transfer if different file CRC or length */
        ZF1_ZMAPND      =   3,       /* Append contents to existing file (if any) */
        ZF1_ZMCLOB      =   4,       /* Replace existing file */
        ZF1_ZMNEW       =   5,       /* Transfer if source newer */
        ZF1_ZMDIFF      =   6,       /* Transfer if dates or lengths different */
        ZF1_ZMPROT      =   7,       /* Protect destination file */
        ZF1_ZMCHNG      =   8,      /* change filename if destination exists */

        // Transport options, one of these in ZF2
        ZF2_ZTNOR		= 0,		/* no compression */
        ZF2_ZTLZW		= 1,		/* Lempel-Ziv compression */
        ZF2_ZTRLE		= 3,		/* Run Length encoding */

        /*
     * Extended options for ZF3, bit encoded
     */
    ZF3_ZCANVHDR	= 0x01,	/* Variable headers OK */
                             /* Receiver window size override */
    ZF3_ZRWOVR 		= 0x04,	/* byte position for receive window override/256 */
    ZF3_ZXSPARS		= 0x40,	/* encoding for sparse file operations */
    }; // end enum ZFileFrame

    enum ZCommandFrame    {
        ZF0_ZCACK1 = 0x01	/* Acknowledge, then do command */
    }; // end enum ZCommandFrame

    ZModem();

    void begin(Stream &serial);

private:
    Stream *_serial;

}; // end class ZModem




// #define CPMEOF 032
// #define WANTCRC 0103    /* send C not NAK to get crc not checksum */
// #define WANTG 0107      /* Send G not NAK to get nonstop batch xmsn */
// #define TIMEOUT (-2)
// #define RCDO (-3)
// #define Tx_RETRYMAX 10
// #define Rx_RETRYMAX 5


//#ifdef NOTDEF
// Pete (El Supremo) - fix up extern int
/* Globals used by ZMODEM functions */
extern uint8_t Rxframeind;      /* ZBIN ZBIN32, or ZHEX type of frame received */
extern uint8_t Rxtype;          /* Type of header received */
extern int Rxcount;         /* Count of data bytes received */
//extern int Zrwindow;        /* RX window size (controls garbage count) */
extern int Rxtimeout;       /* Tenths of seconds to wait for something */
extern char Rxhdr[4];   /* Received header */
extern char Txhdr[4];   /* Transmitted header */
extern long Rxpos;      /* Received file position */
extern long Txpos;      /* Transmitted file position */
extern int8_t Txfcs32;         /* TURE means send binary frames with 32 bit FCS */
extern int8_t Crc32t;          /* Display flag indicating 32 bit CRC being sent */
extern int8_t Crc32;           /* Display flag indicating 32 bit CRC being received */
//extern int Znulls;          /* Number of nulls to send at beginning of ZDATA hdr */
extern char Attn[ZATTNLEN+1];   /* Attention string rx sends to tx on err */
//#endif

/* crctab.c */
long UPDC32(int b, long c);

/* rbsb.c */
#ifndef ARDUINO
void from_cu(void);
void cucheck(void);
int rdchk(int f);
int rdchk(int f);
void sendbrk(void);
#endif
/* zm.c */

void zsbhdr(int type, char *hdr);
void zshhdr(int type, char *hdr);
void zsdata(char *buf, int length, int frameend);
int zrdata(char *buf, int length);
int zgethdr(char *hdr, int eflag);
int zrbhdr(char *hdr);
int zrbhdr32(char *hdr);
int zrhhdr(char *hdr);
void zputhex(int c);
void zsendline2(int c);
int zgethex(void);
int zgeth1(void);
//int zdlread(void);
int noxrd7(void);
void stohdr(long pos);
long rclhdr(char *hdr);

/* rz.c sz.c */
#ifndef ARDUINO
void vfile();
#else
#define vfile(a, ... )
#endif

void bibi(int n);
int wcs(const char *oname);
void saybibi(void);

int wctxpn(char *name,SdFile *file);
int wcrx();


// #define CPMEOF 032
// #define WANTCRC 0103    /* send C not NAK to get crc not checksum */
// #define WANTG 0107      /* Send G not NAK to get nonstop batch xmsn */
// #define TIMEOUT (-2)
// #define RCDO (-3)
// #define Tx_RETRYMAX 10
// #define Rx_RETRYMAX 5

void zmodem_send_file(char* param);


// Dylan (monte_carlo_ecm, bitflipper, etc.) - The way I made this sketch in any way operate on
// a board with only 2K of RAM is to borrow the SZ/RZ buffer for the buffers needed by the main
// loop(), in particular the file name parameter and the SdFat directory entry.  This is very
// unorthodox, but now it works on an\\\ Uno.  Please see notes in zmodem_config.h for limitations
#define file_name (&oneKbuf[512])
#define dir ((FsFile *)&oneKbuf[256])
#define file ((FsFile*)&oneKbuf[768])

extern int Filesleft;
extern long Totalleft;
extern SdFile fout;

#endif