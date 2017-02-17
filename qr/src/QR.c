#ifdef DEBUG
#include <stdio.h>
#include <wchar.h>
#endif
#ifdef USE_ASSERTS
#include <assert.h>
#endif

#include <string.h>

#include "qrEnum.h"
#include "qrFIFO.h"
#include "qrBlocks.h"
#include "qrEncode.h"
#include "qrShuffle.h"
#include "qrGaloisField.h"
#include "qrMatrix.h"

/*
 * #######  #######  ######   #######
 *    #     #     #  #     #  #     #
 *    #     #     #  #     #  #     #
 *    #     #     #  #     #  #     #
 *    #     #     #  #     #  #     #
 *    #     #     #  #     #  #     #
 *    #     #######  ######   #######
 *
 * Here's the list of things that can be improved from the current design (from
 * most to least important changes):
 *
 * - More effective FIFO. Adding 8 bits now shifts the entire buffer 8 times
 *   by 1 bit. It really should be vice versa, 1 time by 8 bits.
 * - More effective bitstream optimizations; see qrEncode.c for more details
 * - Range checking for the main buffer; right now I assume it is big enough
 *   to hold the code as big as min(hint2ver(hint),QRMAX). No range checking
 *   is in place so too small buffer WILL cause core dump
 */
static int reverse(int srcLen, u_int8_t *buf) {
    int         index;
    u_int8_t    temp; /* allow gcc to optimize store/load */

    for (index = 0; index < (srcLen >> 1); index++) {
        temp = buf[index];
        buf[index] = buf[srcLen-index-1];
        buf[srcLen-index-1] = temp;
    }

    return srcLen;
}
static hint_t detectEcc(hint_t hint, int bytes) {
    /*
     * Arguments:
     *  - hint
     *      ver         - version
     *      ecc         - user-supplied ECC
     *      useEcc      - use user input?
     *      strictEcc   - use only version supplied?
     *  - bytes   - encoded data length (padded to the whole byte)
     * Return:  maximum possible ECC for this version
     */
    static const int8_t eccOrder[] = { eccL, eccM, eccQ, eccH };
    int     min,max;
    hint_t  value;

    /*
     * calculate start index; we have to use index instead of actual value
     * because, unlike version, ECC codes aren't sequentual
     */
    for (min = 0, max = 4; hint.useEcc && (min < max) ; min++) {
         if (eccOrder[min] == hint.ecc) {
             break;
         }
    }
    /*
     * limit search to just one ECC variation if both 'useEcc' & 'strictEcc'
     * (this could be the case anyway, if hint.ecc == eccH)
     */
    if (hint.useEcc && hint.strictEcc) {
        max = min + 1;
    }
#ifdef USE_ASSERTS
    assert(min < max);
#endif
    for (value.valid = 0; min < max; min++) {
        if (bytes <= qrTotalSize((hint.ecc = eccOrder[min],hint), 0)) {
            value = hint;
            value.valid = 1;
        }
    }
    return value;
}
static hint_t detectVer(hint_t hint, const char *url, size_t srcLen, size_t bufLen, u_int8_t *buf) {
    /*
     * For this function buffer must already be transformed as follows:
     * 1. Base32 encoded
     * 2. Reversed
     * 3. srcLen is adjusted to reflect Base32-expanded size
     */
    int     bytes, min = 0, max = QRMAX;
    hint_t  eccHint; /* save the orig as detectEcc modifies ECC */

    if (hint.useVer) {
        min = hint.ver;
        if (hint.strictVer) {
            max = min+1;
        }
    }

    for (bytes=-1; min<max; min++) {
        if ((bytes==-1)||(min==8)||(min==25)) {
#ifdef DEBUG_DETECT
            bytes = srcLen;
#else
            bytes = qrEncode((hint.dry=1,hint), url, srcLen, bufLen, buf);
#endif
        }
        eccHint = detectEcc((eccHint = hint, eccHint.ver = min, eccHint),bytes);
        if (eccHint.valid) {
            break;
        }
    }
    return eccHint;
}
static int qrEncodeB32(size_t srcLen, size_t bufLen, u_int8_t *buf) {
    /*
     * b32magic is a number of 1-bit word pads / 8-bit word pads for a given
     * string length (expressed as a function of a strlen MOD 5).
     * So for string 'x' (strlen == 1) this will be two one-bit and six
     * eight-bit pads.
     */
    static const int8_t b32magic[] = { 0x00, 0x26, 0x44, 0x13, 0x31 };
    int                 index,temp;

    /*
     * speed hack; don't load bytes as eight-bit values, just store as a whole
     * and set the bit counter; start from the tail of the source buffer to
     * make sure that I don't overrun that tail as I progress
     */
    for (index = 0; index < srcLen; index++) {
        bfrSetOctet(index, buf[srcLen-index-1], bufLen, buf);
    }
    bfrSetSize(srcLen << 3, bufLen, buf); /* set size in bits, not bytes */

    /*
     * Pad bit buffer to the even number of 5bit words; use zeros per RFC 4648
     * Also, save b32magic value in 'index'; this will be used as a EOL
     * ('=' or '8') counter after the translation is complete
     */
    bfrStore((index = b32magic[srcLen % 5]) >> 4, 0, bufLen, buf);

    /* perform 5/8 translation and redefine the srcLen */
    for (srcLen = 0; (temp = bfrLoad(5, bufLen, buf)) != -1; srcLen++) {
        buf[srcLen] = (temp < 10 ? temp + 48 : temp + 55);
    }
#ifdef USE_ASSERTS
    assert(bfrGetSize(bufLen,buf) == 0);
#endif

    /* pad the result with the 'W's (or '=' for RFC compliance) */
    for (index &= 0xf; index > 0; index--) {
#ifdef RFC4648
        buf[srcLen++] = '=';
#else
        buf[srcLen++] = 'W';
#endif
    }

    /* 'buf' is filled with the Base32 encoded data; return srcLen */
    return reverse(srcLen, buf);
}
hint_t QR(hint_t hint, const char *url, size_t srcLen, size_t bufLen, u_int8_t *buf) {
    hint_t      ver;
    int         index;

    /* encode srcLen bytes from the buffer to Base32-hex form */
    srcLen=qrEncodeB32(srcLen,bufLen,buf); /* srcLen gets the encoded length */

    /* select required QR Code size (version) needed */
    if (!(ver=detectVer(hint, url, srcLen, bufLen, buf)).valid) {
        return ver; /* data doesn't fit, return an error */
    }

    /* make the actual bitstream; this will destroy the original */
    if ((ver.dry=0, index=qrEncode(ver, url, srcLen, bufLen, buf)) == -1) {
        return (ver.valid = 0, ver);
    }

    /*
     * pad data with 0xec/0x11 to the full capacity; standard does not say
     * explicitly which one to start with, so I assume it does not matter
     */
    for (; index<qrTotalSize(ver,0); index++) {
        bfrStore(8, (index & 1) ? 0x11 : 0xec, bufLen, buf);
    }

    /*
     * Transfer bit buffer back to the main buffer; it this point it's rounded
     * to 8 bit boundary; also, it must fit to the buffer as the buffer must
     * be big enough to hold data+ECC+markup
     * TODO add range checking in case crazy user gives too small buffer
     */
    for (srcLen = 0; srcLen < index; srcLen++) {
        buf[srcLen] = bfrLoad(8, bufLen, buf);
    }
#ifdef USE_ASSERTS
    assert(bfrGetSize(bufLen,buf) == 0);
#endif
    /*
     * don't reverse yet; adding ECC is simplier with sequential data
     */
    qrEncodeEcc(ver,bufLen,buf);
    qrShuffle(ver,buf); /* shuffle data */

    srcLen = qrTotalSize(ver,1);

    /* transfer to bit-buffer */
    for (index = 0; index < srcLen; index++) {
        bfrSetOctet(index, buf[srcLen-index-1], bufLen, buf);
    }
    bfrSetSize(srcLen << 3, bufLen, buf);

    /* place data+ECC pixels */
    qrSequence(ver, bufLen, buf);

    /* XOR the matrix and update markup with a new XOR mask info */
    ver=qrXor(ver, buf);

    return (ver.valid = 1,ver);
}
