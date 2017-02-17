#include "qrGaloisField.h"
#include "qrBlocks.h"
#include "qrFIFO.h"

#define	GF_BITS		8
#define	GF_SIZE		(1<<GF_BITS)
#define PRIMITIVE	0x11d

static void initGF(u_int8_t *alphaGF) {
    int a,b;

    for(b=0, a=1; b < GF_SIZE; b++) {
        alphaGF[b] = a;
        a <<= 1;
        a ^= (a & GF_SIZE ? PRIMITIVE : 0);
    }
}
static u_int8_t powerGF(u_int8_t *alphaGF, u_int8_t a) {
    /*
     * this been a table lookup but it was traded to save a few bytes
     * of a buffer (256 - function size); idk if it worth the trouble
     */
    int b;

    if (a == 0) {
        return a;
    }
    for (b = 0; b < GF_SIZE; b++) {
        if (alphaGF[b] == a) {
            break;
        }
    }
    return b;
}
static u_int8_t addGF(u_int8_t a, u_int8_t b) {
    return a ^ b;
}
static int mulGF(u_int8_t *alphaGF, u_int8_t a, u_int8_t b) {
    if (!(a&&b)) return 0; /* anything times zero (AF0) is zero */
    return alphaGF[(powerGF(alphaGF,a)+powerGF(alphaGF,b)) % (GF_SIZE-1)];
    /* yes, GF_SIZE-1 */
}
static void genGF(u_int8_t *alphaGF, u_int8_t power, u_int8_t *poly) {
    int i,a;

    for (a=0; a<power; a++) {
        poly[a] = alphaGF[0];
    }
    for (a=1; a<power; a++) { /* black magic here */
        for (i=a; i>0; i--) {
            poly[i] = addGF(poly[i-1], mulGF(alphaGF, poly[i], alphaGF[a]));
        }
        poly[0] = mulGF(alphaGF, poly[0], alphaGF[a]);
    }
}
static void qrReedSolomon(size_t power, size_t len, u_int8_t *src, u_int8_t *dst, u_int8_t *alphaGF) {
    /*
     * power:   ECC poly power; also the number of bytes to generate
     * len:     Number of input bytes in the buffer (src)
     * src:     Source data
     * dst:     Where to put the result
     * alphaGF: processing buffer, must hold GF_SIZE + 128 bytes
     *          (actually, 256 + 68 is technically enough by the QR spec)
     */
    unsigned    i,j;
    u_int8_t    s,*poly;

    poly = alphaGF + GF_SIZE;

    initGF(alphaGF);
    genGF(alphaGF, power, poly); /* init polynominal coefficents */

    for (i=0; i<power; i++) {
        dst[i]=0; /* zero out registers */
    }
    for (i=0; i<len; i++) {  /* stage A: passing message, making regs */
        s = addGF(src[i], dst[power-1]);
        for (j=power-1; j>0; j--) {
            dst[j]=addGF(dst[j-1],mulGF(alphaGF, s, poly[j]));
        }
        dst[0]=mulGF(alphaGF, s, poly[0]);
    }
    for (i=0; i<(power/2); i++) { /* stage B: (collapsed) reverse registers */
        s=dst[i];
        dst[i]=dst[power-i-1];
        dst[power-i-1]=s;
    }
}
int qrEncodeEcc(hint_t ver, size_t bufLen, u_int8_t *buf) {
    u_int8_t    *gfBuf, *dst;
    int         i,power,block; /* power is ECC size; block is a block size */

    gfBuf = bfrGetBfr(bufLen, buf);
    power = qrShortSize(ver,1)-qrShortSize(ver,0);

    dst = buf + qrTotalSize(ver,0);

    /* 'short' blocks */
    for (i=0,block=qrShortSize(ver,0); i<qrShortBlocks(ver); i++) {
        qrReedSolomon(power,block,buf,dst,gfBuf);
        buf += block;
        dst += power;
    }
    /* 'long' blocks */
    for (i=0,block=qrLongSize(ver,0); i<qrLongBlocks(ver); i++) {
        qrReedSolomon(power,block,buf,dst,gfBuf);
        buf += block;
        dst += power;
    }

    return qrTotalSize(ver,1);
}
