#include <sys/types.h>
#include "qrEnum.h"
#include "qrBlocks.h"
/*
 * qrShuffle shuffles the data to make it as described in Section 6.6
 * of the ISO 18004/2005. It does not use any buffers and the transformation
 * is performed 'in-place'. However, space is traded for the CPU.
 */

typedef struct {
    unsigned    row:8;
    unsigned    col:8;
    unsigned    fwd:1;  /* forward (1) or reverse (0) index */
    unsigned    ecc:1;  /* use ECC (1) or data (0) layout   */
} idx_t;

idx_t i2rc(hint_t ver, idx_t arg, int index) {
    int     block, shortLen, shortCnt, longLen, longCnt;
    idx_t   fwd = arg, rev = arg; /* carry fwd & ecc flags over */
    if (arg.ecc) {
        shortLen = longLen = qrShortSize(ver,1)-qrShortSize(ver,0);
        shortCnt = qrTotalBlocks(ver);
        longCnt = 0;
    } else {
        shortLen = qrShortSize(ver,0);
        shortCnt = qrShortBlocks(ver);
        longLen = qrLongSize(ver,0);
        longCnt = qrLongBlocks(ver);
    }

    /* move left to right, next row upon the end */
    if (index < (block = shortCnt * shortLen)) {
        fwd.row = index / shortLen;
        fwd.col = index % shortLen;
    } else {
        fwd.row = shortCnt + ((index - block) / longLen);
        fwd.col = (index - block) % longLen;
    }

    /* move top to bottom, next column upon the end */
    if (index < (block = shortLen * (shortCnt + longCnt))) {
        rev.col = index / (shortCnt + longCnt);
        rev.row = index % (shortCnt + longCnt);
    } else {
        rev.col = shortLen + (index - block) / longCnt;
        rev.row = shortCnt + (index - block) % longCnt;
    }

    return arg.fwd ? fwd : rev;
}
int rc2i(hint_t ver, idx_t arg) {
    int     shortLen, shortCnt, longLen, longCnt;
    int     fwd = 0, rev = 0;
    if (arg.ecc) {
        shortLen = longLen = qrShortSize(ver,1)-qrShortSize(ver,0);
        shortCnt = qrTotalBlocks(ver);
        longCnt = 0;
    } else {
        shortLen = qrShortSize(ver,0);
        shortCnt = qrShortBlocks(ver);
        longLen = qrLongSize(ver,0);
        longCnt = qrLongBlocks(ver);
    }

    if (arg.row < shortCnt) {
        fwd = shortLen * arg.row + arg.col;
    } else {
        fwd = shortLen * shortCnt + longLen * (arg.row - shortCnt) + arg.col;
    }
    if (arg.col < shortLen) {
        rev = (shortCnt + longCnt) * arg.col + arg.row;
    } else {
        rev = (shortCnt + longCnt) * shortLen +
            (arg.col - shortLen) * longCnt + (arg.row - shortCnt);
    }

    return arg.fwd ? fwd : rev;
}
static int idx(hint_t ver, idx_t arg, int index) {
    arg = i2rc(ver, (arg.fwd ^= 1, arg), index);

    return rc2i(ver, (arg.fwd ^= 1, arg));
}
static void qrShuffleStep(hint_t ver, idx_t arg, u_int8_t *buf) {
    int         len, cnt, ff;
    u_int8_t    temp;

    len = arg.ecc ? qrTotalSize(ver,1)-qrTotalSize(ver,0) : qrTotalSize(ver,0);

    for (cnt = 0; cnt < len; cnt++) {
        ff = cnt;
        while((ff = idx(ver, (arg.fwd = 1,arg), ff)) < cnt);

        temp = buf[cnt];
        buf[cnt] = buf[ff];
        buf[ff] = temp;
    }
}
int qrShuffle(hint_t ver, u_int8_t *buf) {
    idx_t   arg;

    qrShuffleStep(ver, (arg.ecc = 0, arg), buf); /* data */
    qrShuffleStep(ver, (arg.ecc = 1, arg), buf + qrTotalSize(ver,0)); /* ecc */

    return qrTotalSize(ver,1);
}
