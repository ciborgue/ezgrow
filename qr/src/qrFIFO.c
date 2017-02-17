#include <string.h>
#include "qrEnum.h"
#ifdef DEBUG
#include <stdio.h>
#include <assert.h>
#endif
/*
 * How many bytes are reserved at the very end of the processingBuffer
 * to hold the number of bits in the buffer? Normally this should be 2.
 * WARNING!!! Used as a documentation tool and not as tunable!!!
 */
#define COUNTER_LENGTH  2

/*
 *  ######   #     #  #######  #######  #######  ######
 *  #     #  #     #  #        #        #        #     #
 *  #     #  #     #  #        #        #        #     #
 *  ######   #     #  #####    #####    #####    ######
 *  #     #  #     #  #        #        #        #   #
 *  #     #  #     #  #        #        #        #    #
 *  ######    #####   #        #        #######  #     #
 *
 * The following implements a bitstring with the following operations:
 * bfrSave - saves bits to the buffer
 * bfrLoad - retrieves 1 bit from the buffer or -1. Shrinks the buffer by 1
 */
#ifndef _REENTRANT
/*
 * Static buffer (don't confuse 'static' keyword here; that's for visibility)
 * This breaks thread safety, so it's only allocated if no thread safety
 * requested
 */
static u_int8_t processingBuffer[QRBUFFER];
#endif
u_int8_t src(size_t index, const char *url, size_t srcLen, u_int8_t *buf) {
    /*
     * Returns the source data byte (after Base32+reverse is done).
     * Either pointer might be NULL (but no range check is performed).
     */
    int urlLen;

    if (url != NULL) {
        urlLen = strlen(url);
        if (index < urlLen) {
            return (u_int8_t)url[index];
        }
        index -= urlLen;
    }
    if (buf != NULL) {
#ifdef USE_ASSERTS
        assert(srcLen >= index+1);
#endif
        return buf[srcLen-index-1];
    }
#ifdef USE_ASSERTS
    assert("Both URL and buf can't be NULL" == NULL);
#endif
    return 0;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
u_int8_t *bfrGetBfr(size_t bufLen, u_int8_t *buf) {
#ifndef _REENTRANT
    return processingBuffer;
#else
    return buf-QRBUFFER;
#endif
}
#pragma GCC diagnostic pop
/*
 * bfrGetOctet & bfrSetOctet give the transparent interface to the
 * main buffer + processing buffer combination and are used to mask
 * the hole in the buffer.
 * No range check should be performed as I'm not creating poor man's Java
 * all the checks MUST be complete before the operation is started at
 * the higher level
 */
u_int8_t bfrGetOctet(int index, int bufLen, u_int8_t *buf) {
#ifdef _REENTRANT
    u_int8_t    *processingBuffer; /* must be stack var for thread safety */
    bufLen -= QRBUFFER;
    processingBuffer = buf + bufLen;
#endif
    if (index < (QRBUFFER-COUNTER_LENGTH)) {
        return processingBuffer[QRBUFFER-COUNTER_LENGTH-1-index];
    }
    return buf[bufLen-index-COUNTER_LENGTH+QRBUFFER-1];
}
void bfrSetOctet(int index, unsigned value, int bufLen, u_int8_t *buf) {
#ifdef _REENTRANT
    u_int8_t    *processingBuffer; /* must be stack var for thread safety */
    bufLen -= QRBUFFER;
    processingBuffer = buf + bufLen;
#endif
    if (index < QRBUFFER-COUNTER_LENGTH) {
        processingBuffer[QRBUFFER-COUNTER_LENGTH-1-index] = value;
        return;
    }
    buf[bufLen-index-COUNTER_LENGTH+QRBUFFER-1] = value;
}
void bfrSetSize(int len, int bufLen, u_int8_t *buf) {
    /*
     * Set bit counter for the buffer; it's always on the fixed position
     * at the very end of the processingBuffer.
     * 16 bit is enough as v41 (177 x 177) is 31329 bits
     */
    bfrSetOctet(-2, len, bufLen, buf);
    bfrSetOctet(-1, len >> 8, bufLen, buf);
}
int bfrGetSize(int bufLen, u_int8_t *buf) {
    return bfrGetOctet(-2, bufLen, buf)|(bfrGetOctet(-1, bufLen, buf) << 8);
}
void bfrStore(int bits, unsigned value, int bufLen, u_int8_t *buf) {
    int index, bytes, carry, buffr;

    for ( ; bits > 0; bits--) {
        bfrSetSize(bytes = (bfrGetSize(bufLen, buf)+1), bufLen, buf);

        carry = value & (1 << (bits - 1)) ? 1 : 0;
        for (index = 0; index < (bytes>>3)+1; index++) {
            buffr = bfrGetOctet(index, bufLen, buf);
            buffr = (buffr << 1) | carry;
            carry = buffr & 0x100 ? 1 : 0;
            bfrSetOctet(index, buffr, bufLen, buf);
        }
    }

}
int bfrLoad(int bits, int bufLen, u_int8_t *buf) {
    int    index, shift, value;

    if (bits > bfrGetSize(bufLen, buf)) {
        return -1;
    }
    for (value = 0; bits > 0; bits--) {
        index = bfrGetSize(bufLen, buf);
        bfrSetSize(index-1, bufLen, buf);
        shift = (index-1) & 7;
        index = (index >> 3)+(index & 7 ? 1 : 0)-1;
        value = (value << 1)|((bfrGetOctet(index, bufLen, buf) >> shift) & 1);
    }

    return value;
}
