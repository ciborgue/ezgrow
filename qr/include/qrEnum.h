/*
 * QR code size (or "version") is always expressed as the "version minus one"
 *  so the v1 code (21 pixels, or "modules") is expressed as ver==0
 */
#ifndef __QRENUM_H__
#define __QRENUM_H__
/*
 * it's here for int8_t/u_int8_t/size_t
 * If not present on the target system they MUST be defined as follows:
 * int8_t   - signed 8bit integer (normally 'char')
 * u_int8_t - unsigned 8bit integer (normally 'unsigned char')
 * size_t   - integer
 */
#include <sys/types.h>
/*
 * Only use assertions in the debugging (desktop) version of the library.
 * According to Mihn failed assertion in the verifier freezes whole STB
 */
#ifdef USE_ASSERTS
#include <assert.h>
#endif

#ifndef QRMAX
#define QRMAX       40  /* Max version allowed plus one */
#endif

/*
 * user-accessible macro to convert version [0..39] to the image size
 */
#define QRSIZE(x)   (21 + ((x) << 2))
#define VERSIZE(x)  (QRSIZE((x).ver))

#define trueXY(ver,xy)  (xy + ((xy < 0) ? QRSIZE(ver) : 0))

/*
 * QRBUFFER is the size of the internal (single-threaded) or user-provided
 * (multi-threaded) storage. This doesn't include QR matrix and is EXTRA
 * memory required. So total memory needed is QR matrix + QRBUFFER.
 */
#define QRBUFFER   (256+68*2) /* ABSOLUTE MINIMUM IS 256+(2*68) */

/* 
 * QRMEM is the size of the buffer that client must provide. Initially it
 * holds the binary part of the message that has to be encoded.
 * Upon exit this buffer holds the prepared QR code that user can decode
 * using qrGetMod(..) function and the returned version number.
 */
#ifdef _REENTRANT
    /*
     * We're building the thread-safe version; client must provide bigger
     * buffer with each call as library can't hold any static data. Note
     * that the internal buffer won't be statically allocated with this flavor.
     * TODO test it
     */
#define QRMEM(ver) \
    (QRSIZE(ver)*((QRSIZE(ver)>>3)+(QRSIZE(ver) & 7 ? 1 : 0))+QRBUFFER)
#else
    /*
     * Single-threaded application has an internal buffer (see qrMisc.c)
     * that is declared as 'static'
     */
#define QRMEM(ver) \
    (QRSIZE(ver)*((QRSIZE(ver)>>3)+(QRSIZE(ver) & 7 ? 1 : 0)))
#endif

typedef	enum {
    endOfMessageMI	    = 0,    /* not a mode indicator as such (see ISO) */
    numericMI		        = 1,
    alphanumericMI	    = 2,
    structuredAppendMI	= 3,
    eightBitMI		      = 4,
    fnc1firstMI		      = 5,
    eciMI		            = 7,
    kanjiMI		          = 8,
    fnc1secondMI	      = 9
} modeIndicator;

typedef enum {
    eccM		            = 0,
    eccL		            = 1,
    eccH		            = 2,
    eccQ		            = 3
} eccLevel;

typedef struct {
    /* version: requested QR Code version: [0..39]
     *  0 == 'version 1'
     *  ...
     *  39 == 'version 40'
     */
    unsigned ver : 6;
    unsigned useVer : 1;    /* 1=use as minimal/exact version             */
    unsigned strictVer : 1; /* 1='ver' is an exact version if (useVer==1) */
    /*
     * ecc: requested ECC level: [eccL, eccM, eccQ or eccH]
     */
    unsigned ecc : 2;
    unsigned useEcc : 1;    /* 1=use as minimal/exact ECC                 */
    unsigned strictEcc : 1; /* 1='ecc' is an exact ECC if (useEcc==1)     */
    /*
     * 'dry' & 'xor': used internally by the library; value is ignored
     */
    unsigned xor : 3;
    unsigned dry : 1;
    /*
     * return flag (if function returns hint_t)
     *  0: result is NOT valid
     *  1: result is valid
     */
    unsigned valid: 1;
} hint_t;
#endif
