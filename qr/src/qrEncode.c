#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef USE_ASSERTS
#include <assert.h>
#endif
#ifdef DEBUG2
#include <stdio.h>
#include <wchar.h>
#endif

#include "qrEnum.h"
#include "qrBlocks.h"
#include "qrEncode.h"
#include "qrFIFO.h"

static int getQRindex(modeIndicator m, int i) {
    static const int8_t alnum[] = {
        36, -1, -1, -1, 37, 38, -1, -1, -1, -1, 39, 40, -1, 41, 42, 43,/*0x20*/
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 44, -1, -1, -1, -1, -1,/*0x30*/
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,/*0x40*/
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
    };
    /*
     * getQRindex: returns character code according to the mode selected
     * (i.e. returns 10 for 'A' in alphanumeric, 65 for 'A' in 8 bit
     * For the improper combination (ie 'A' in numeric) returns -1
     */
    switch (m) {
        case numericMI:
            return i - '0';
        case alphanumericMI:
            return alnum[i - 0x20];
        case eightBitMI:
            return i;
        default: /* make gcc happy */
            break;
    }
#ifdef USE_ASSERTS
    assert("Mode indicator is not supported (Kanji?)" == NULL);
#endif
    return -1; /* TODO Add Kanji support as ISO requires */
}
static int isQRmode(modeIndicator m, int i) {
    /*
     * returns true if 'i' belongs to the mode m ('0' *is* numericMI)
     * false if not
     */
    switch (m) {
        case numericMI:
            return isdigit(i);
        case alphanumericMI:
            return i >= 0x20 && i <= 0x5a && getQRindex(alphanumericMI,i) != -1;
        case eightBitMI:
            return 1;
        default: /* make gcc happy */
            break;
    }
#ifdef USE_ASSERTS
    assert("Mode indicator is not supported (Kanji?)" == NULL);
#endif
    return 0; /* TODO Add Kanji support as ISO requires */
}
static int isExclusive(modeIndicator m, int i) {
    /*
     * returns true if 'i' exclusively belongs to some class, for example:
     * '0' is numeric AND alphanumeric AND 8bit (so false)
     * 'A' is alphanumeric AND 8bit (so true for alphanumeric, false for 8bit)
     * 'a' is 8bit-only (so true for 8bit)
     */
    switch (m) {
        case alphanumericMI:
            return isQRmode(alphanumericMI,i) && !isQRmode(numericMI,i);
        case eightBitMI:
            return isQRmode(eightBitMI,i) && !isQRmode(alphanumericMI,i);
        case numericMI:
            return isQRmode(numericMI,i);
        default:
            break;
    }
#ifdef USE_ASSERTS
    assert("Mode indicator is not supported (Kanji?)" == NULL);
#endif
    return 0; /* TODO Add Kanji support as ISO requires */
}
static modeIndicator initialEnc(int grade, const char *url, size_t srcLen, u_int8_t *buf) {
    /*
     * Returns initial encoding as specified in Annex H; see Annex H
     * for the reference algorithm. This function does not change buffer
     * so it only needs url, srcLen & buf.
     * UPD: It's Annex J in 2006 spec; nice, isn't it?
     */
    static const int8_t cntr[][3] = {
        { 5, 5, 6}, /* 1.c; UPD: J.2 a(2); Kanji, currently unused */
        { 6, 7, 8}, /* 1.c; UPD: J.2 a(3) */
        { 4, 4, 5}, /* 1.d; UPD: J.2 a(4), Num -> 8bit */
        { 7, 8, 9}  /* 1.d; UPD: J.2 a(4), Num -> Alnum */
    };
    int i,total;

    total = (url == NULL ? 0 : strlen(url)) + srcLen;

    if (isExclusive(eightBitMI, src(0, url, srcLen, buf))) {
        return eightBitMI; /* buffer starts with 8bit; no much of a choice */
    }
    /* this implements J.2 a(3) (2006 spec) */
    if (isExclusive(alphanumericMI, src(0, url, srcLen, buf))) {
        for (i=1; (i<total)&&(i<cntr[1][grade]); i++) {
            if (isExclusive(eightBitMI, src(i, url, srcLen, buf))) {
                return eightBitMI;
            }
        }
        return alphanumericMI;
    }
    /* this implements J.2 a(4) (2006 spec). Note that src{0} IS numeric now */
    /* part 1: shall I start with 8bit? */
    for (i=1; (i<total)&&(i<cntr[2][grade]); i++) {
        if (isExclusive(eightBitMI, src(i, url, srcLen, buf))) {
            return eightBitMI;
        }
    }
    /* part 2: shall I start with alnum? */
    for (i=1; (i<total)&&(i<cntr[3][grade]); i++) {
        if (isExclusive(alphanumericMI, src(i, url, srcLen, buf))) {
            return alphanumericMI;
        }
    }
    return numericMI;
}
static modeIndicator annex(int grade, modeIndicator m, int index, const char *url, size_t srcLen, u_int8_t *buf) {
    /*
     * TODO Add more non-orthodox cases (not part of the original ISO
     * specification). Examples include every 'terminal' case as the EOL
     * can be of any encoding (or non-existent if the last bit happend to
     * be on the byte boundary) and encoder does not have to switch back,
     * so bit requirements can be significanly lowered.
     *
     * For example: 'aa0000'<EOL> can be encoded as:
     *  'aa'+<numericMI(4)>+<counter(10)>+'000'(10)+'0'(4) == 36bit
     *  while Byte encoding is 'aa0000'(8x6)               == 48bit
     */
    static const int8_t cntr[][3] = {
        { 9,12,13}, /* J.2 b(1); Kanji, currently unused */
        {11,15,16}, /* J.2 b(2) */
        { 6, 8, 9}, /* J.2 b(3) */
        { 6, 7, 8}, /* J.2 b(4) */
        {13,15,17}, /* J.2 c(3) */
#ifndef STDANNEX
        { 7, 8, 9}  /* NOT a part of the Annex; A->8 transition */
#endif
    };
    u_int8_t    c = 0; /* make gcc happy */
    int         i,e,total;

    total = (url == NULL ? 0 : strlen(url)) + srcLen;

    if (m == eightBitMI) { /* J.2 b */
        /*
         * 'e' tells if the whole string (not one char) is exclusive AlNum
         * This part is not really clear from the standard and I think it's
         * not as optimal as it should be (but it should work OK for DTV)
         */
        for (i=0,e=0; ((index+i)<total)&&(i<cntr[1][grade]); i++) {
            c = src(index+i, url, srcLen, buf);
            if (!isQRmode(alphanumericMI,c)) {
                break; /* Kanji or Byte; let's try other alternatives */
            }
            if (!e) {
                e = isExclusive(alphanumericMI,c);
            }
        }
        if (e && (i >= cntr[1][grade])) {
            return alphanumericMI;
        }
        for (i=0; (index+i)<total; i++) { /* count Numeric characters */
            c = src(index+i, url, srcLen, buf);
            if (!isQRmode(numericMI,c)) {
                break;
            }
        }
        if (isExclusive(eightBitMI, c) && (i >= cntr[2][grade])) {
            return numericMI;
        }
        if (isExclusive(alphanumericMI, c) && (i >= cntr[3][grade])) {
            return numericMI;
        }
    }
    if (m == alphanumericMI) { /* J.2 c */
        /*
         * this check for eightBitMI can be moved up but I keep it the way
         * it all described in the standard
         */
        if (isExclusive(eightBitMI, src(index, url, srcLen, buf))) {
            return eightBitMI;
        }
        for (i=0; ((index+i)<total)&&(i<cntr[4][grade]); i++) {
            c = src(index+i, url, srcLen, buf);
            if (!isQRmode(numericMI,c)) {
                break; /* Kanji or Byte; let's try other alternatives */
            }
        }
        /*
         * Even though ISO spec (J.2 c(3)) says:
         * "If sequence of at least [13,15,17] Numeric characters occurs
         * before more data from the exclusive subset of the Alphanumeric
         * character set, switch to Numeric mode" I believe that it is not
         * entirely correct. Follow-up data CAN be Byte type; look:
         *  4 bit   - switch to the Numeric mode
         * 10 bit   - Numeric counter for v1..10
         * 44 bit   - 13 bytes in Numeric (10 bit * 4 + 4 bit)
         *  4 bit   - switch back to Byte mode
         *  9 bit   - Byte counter for v1..10
         * Total: 71 bit
         *
         * 72 bit   - 13 bytes in Alphanumeric
         *
         */
        if (i >= cntr[4][grade]) {
            return numericMI;
        }
#ifndef STDANNEX
        /*
         * NOT a part of the Annex; switch from the Alphanumeric to the
         * Numeric if the tail of Alphanumeric is actually Numeric AND the
         * next data is Byte. This way I have to switch at the Byte boundary
         * so the MI+COUNTER will be sent anyway and can be left out from
         * the calculation. So it's 'easier' to switch from the Alphanumeric
         * to Numeric at the Byte boundary than without it
         */
        if (i >= cntr[5][grade] && isExclusive(eightBitMI,c)) {
            return numericMI;
        }
#endif
    }
    if (m == numericMI) { /* J.2 d */
        if (isExclusive(eightBitMI, src(index, url, srcLen, buf))) {
            return eightBitMI;
        }
        if (isExclusive(alphanumericMI, src(index, url, srcLen, buf))) {
            return alphanumericMI;
        }
    }
    return m; /* no change */
}
static int encN(hint_t ver, int srcIndex, size_t srcLen, const char *url, size_t bufLen, u_int8_t *buf) {
    int index,sum,temp,totalBits = 0;

    for (sum=0,index=0; index<srcLen; index++) {
        temp = getQRindex(numericMI, src(srcIndex+index,url,bufLen,buf));
#ifdef USE_ASSERTS
        assert(isQRmode(numericMI, src(srcIndex+index,url,bufLen,buf)));
#endif
        switch (index % 3) {
            case 0:
                sum = temp * 100;
                break;
            case 1:
                sum += temp * 10;
                break;
            case 2:
                totalBits += 10; /* 10 bits to store */
                sum += temp;
                if (!ver.dry) {
#ifdef DEBUG2
                    fwprintf(stderr,L"[10:%03d]",sum);
#endif
                    bfrStore(10, sum, bufLen, buf);
                }
        }
    }
    switch (index % 3) {
        case 1:
            totalBits += 4;
            if (!ver.dry) {
#ifdef DEBUG2
                fwprintf(stderr,L"[4:%d]",sum / 100);
#endif
                bfrStore(4, sum / 100, bufLen, buf);
            }
            break;
        case 2:
            totalBits += 7;
            if (!ver.dry) {
#ifdef DEBUG2
                fwprintf(stderr,L"[7:%02d]",sum / 10);
#endif
                bfrStore(7, sum / 10, bufLen, buf);
            }
    }
    return totalBits;
}
static int encA(hint_t ver, int srcIndex, size_t srcLen, const char *url, size_t bufLen, u_int8_t *buf) {
    int index,totalBits,sum,temp;

    for (totalBits=0,sum=0,index=0; index<srcLen; index++) {
#ifdef USE_ASSERTS
        assert(isQRmode(alphanumericMI, src(srcIndex+index,url,bufLen,buf)));
#endif
        temp = getQRindex(alphanumericMI,src(srcIndex+index,url,bufLen,buf));
        switch (index % 2) {
            case 0:
                sum = temp * 45;
                break;
            case 1:
                totalBits += 11;
                sum += temp;
                if (!ver.dry) {
#ifdef DEBUG2
                    fwprintf(stderr,L"[11:%d(%c%c)", sum,
                        src(srcIndex+index-1,url,bufLen,buf),
                        src(srcIndex+index,url,bufLen,buf));
#endif
                    bfrStore(11, sum, bufLen, buf);
                }
        }
    }
    if (index % 2) {
        totalBits += 6;
        if (!ver.dry) {
#ifdef DEBUG2
        fwprintf(stderr,L"[6:%d](%c)", sum / 45,
            src(srcIndex+index-1,url,bufLen,buf));
#endif
            bfrStore(6, sum / 45, bufLen, buf);
        }
    }
    return totalBits;
}
static int enc8(hint_t ver, int srcIndex, size_t srcLen, const char *url, size_t bufLen, u_int8_t *buf) {
    int index,totalBits,temp;

    for (totalBits=0,index=0; index<srcLen; index++) {
        temp = src(srcIndex+index,url,bufLen,buf);
        totalBits += 8;
        if (!ver.dry) {
#ifdef DEBUG2
            fwprintf(stderr,L"[8:%d]",temp);
#endif
            bfrStore(8, temp, bufLen, buf);
        }
    }
    return totalBits;
}
int qrEncode(hint_t ver, const char *url, size_t srcLen, size_t bufLen, u_int8_t *buf) {
    static const int8_t cntr[][9] = {
    /*       N  A     8           K  */
        {-1,10, 9,-1, 8,-1,-1,-1, 8},   /*   v1..9 */
        {-1,12,11,-1,16,-1,-1,-1,10},   /* v10..26 */
        {-1,14,13,-1,16,-1,-1,-1,12}    /* v27..40 */
    };
    modeIndicator   mV, mN; /* mode and counter: viejo, nuevo */
    int             grade,total,index,bytes,countBits,totalBits = 0;

    /*
     * calculate total size; refuse to encode an empty QR Code
     *
     * TODO see what happens; maybe it's possible to allow?
     * TODO verify very short data cases, like '12' or '123'
     */
    if ((total = (url == NULL ? 0 : strlen(url)) + srcLen) == 0) {
        return -1;
    }
    /*
     * 'grade' is an index to a different tables; 0 for 1..9, 1 for 10..26
     * 2 for 27..40. Don't change 'grade' without changing 'initialEnc'
     * and 'annex' too
     */
    grade = (ver.ver < 9) ? 0 : (ver.ver < 26 ? 1 : 2);

    /*
     * determine the initial encoding; the one that is used for the very
     * first byte. Most likely eightBitMI for 'http://...'
     */
    mV = initialEnc(grade, url, srcLen, buf);

    for (index=0; index<total; index+=bytes) {
        for (bytes=0; (index+bytes)<total; bytes++) {
            mN = annex(grade, mV, index+bytes, url, srcLen, buf);
            if (mN != mV) {
                break;
            }
        }
#ifdef USE_ASSERTS
        assert(bytes > 0);
#endif
        /* mode indicator is always 4 bit long */
        totalBits += 4 + cntr[grade][mV];
        if (!ver.dry) {
#ifdef DEBUG2
                fwprintf(stderr,L"<M:%c><%d for %d:%d>",
                    (mV == 1 ? 'N' : (mV == 2 ? 'A' : '8')),
                    cntr[grade][mV],grade,bytes);
#endif
            bfrStore(4, mV, bufLen, buf);
            bfrStore(cntr[grade][mV], bytes, bufLen, buf);
        }
        switch (mV) {
            case numericMI:
                countBits = encN(ver,index,bytes,url,srcLen,buf);
                break;
            case alphanumericMI:
                countBits = encA(ver,index,bytes,url,srcLen,buf);
                break;
            case eightBitMI:
                countBits = enc8(ver,index,bytes,url,srcLen,buf);
                break;
            default:
#ifdef USE_ASSERTS
                assert("Mode indicator is not supported (Kanji?)" == NULL);
#endif
                countBits = -1; /* TODO: add Kanji support */
                break;
        }
        if (countBits == -1) {
#ifdef USE_ASSERTS
            assert(countBits != -1);
#endif
            return countBits; /* Internal error occured; propagate */
        }
        totalBits += countBits;
        mV = mN;
    }
    if (((qrTotalSize(ver,0) << 3)-bfrGetSize(bufLen,buf)) > 3) {
        /* add endOfMessageMI if I have space */
        totalBits += 4;
        if (!ver.dry) {
#ifdef DEBUG2
            fwprintf(stderr,L"<EOM>");
#endif
            bfrStore(4, endOfMessageMI, bufLen, buf);
        }
    }
    if ((countBits = 8 - (bfrGetSize(bufLen,buf) & 7)) != 8) {
        /* pad to the next whole byte */
        totalBits += countBits;
        if (!ver.dry) {
#ifdef DEBUG2
            fwprintf(stderr,L"<PAD:%d>", countBits);
#endif
            bfrStore(countBits, 0, bufLen, buf); /* add countBits zeros */
        }
    }
#ifdef USE_ASSERTS
    assert((bfrGetSize(bufLen,buf) == -1)||((bfrGetSize(bufLen,buf) & 7) == 0));
    assert(ver.dry || (bfrGetSize(bufLen,buf) == totalBits));
#endif
    return (ver.dry ? totalBits : bfrGetSize(bufLen,buf)) >> 3;
}
