#include <stdlib.h>

#include "qrEnum.h"
#include "qrBitMagic.h"
#include "qrBlocks.h"
#include "qrMatrix.h"
#include "qrFIFO.h"

typedef struct {
    unsigned data : 1;
    unsigned soft : 1;
    unsigned hard : 1;
    unsigned valid: 1;
} pixel_t;

#define	brHard  4   /* hard border: finder, separator, version */
#define	brSoft  2   /* soft border: timing, position */

#ifdef DEBUG
#include <wchar.h>
#include <stdio.h>

int         qrCnt = 0;
u_int16_t   qrLog[QRSIZE(40) * QRSIZE(40)];

u_int16_t   *getLog() { return qrLog; }
#endif
static pixel_t qrIsVersionInfo(hint_t ver, int x, int y) {
    /*
     * qrIsVersionInfo returns the following:
     * pixel value bitwise OR with the flags if (x,y) is on the info
     * area or -1 if it is not
     *
     * version: values + ECC; I don't want to make them so precanned
     */
    static const int versionPattern[] = {
#if QRMAX > 5
                 0x07c94, 0x085bc, 0x09a99, 0x0a4d3, /* v7,8,9,10 */
#endif
#if QRMAX > 10
        0x0bbf6, 0x0c762, 0x0d847, 0x0e60d, 0x0f928,
#endif
#if QRMAX > 15
        0x10b78, 0x1145d, 0x12a17, 0x13532, 0x149a6,
#endif
#if QRMAX > 20
        0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1,
#endif
#if QRMAX > 25
        0x1afab, 0x1b08e, 0x1cc1a, 0x1d33f, 0x1ed75,
#endif
#if QRMAX > 30
        0x1f250, 0x209d5, 0x216f0, 0x228ba, 0x2379f,
#endif
#if QRMAX > 35
        0x24b0b, 0x2542e, 0x26a64, 0x27541, 0x28c69
#endif
    };
    pixel_t     value;
    int	        bit,data,size;

    value.valid = value.hard = value.soft = value.data = 0;
    if (ver.ver < 6) {
        return value; /* no version info prior to the v7 */
    }
    size=VERSIZE(ver)-11;
    /* performance hack */
    if (((x>=size)&&(x<(size+3))&&(y<6))||((y>=size)&&(y<(size+3))&&(x<6))) {
        data=versionPattern[ver.ver-6];
        for (bit=0; bit<18; bit++,data >>= 1) {
            if (((x == (size+(bit%3)))&&(y == (bit/3))) /* top-right */
                || ((x == (bit/3))&&(y == (size+(bit%3))))) { /* bottom-left */
                value.hard = value.valid = 1, value.soft = 0,
                    value.data = data & 1;
                break;
            }
        }
    }
    return value; /* no; (x,y) is not on the info area */
}
static pixel_t qrIsFormatInfo(hint_t ver, int x, int y) {
    static const int16_t formatPattern[][8] = {
        /* everything is smaller than 0x7FFF so signed can be used here */
/* m */	{0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x45f9, 0x40ce, 0x4f97, 0x4aa0},
/* l */	{0x77c4, 0x72f3, 0x7daa, 0x789d, 0x662f, 0x6318, 0x6c41, 0x6976},
/* h */	{0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b},
/* q */	{0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed}
    };
    static const int8_t formatPosition[][4] = { /* positions+values */
        {-1, 8, 8, 0}, /* bit  0; x1,y1+x2,y2 */
        {-2, 8, 8, 1}, /* bit  1 */
        {-3, 8, 8, 2}, /* bit  2 */
        {-4, 8, 8, 3}, /* bit  3 */
        {-5, 8, 8, 4}, /* bit  4 */
        {-6, 8, 8, 5}, /* bit  5 */
        {-7, 8, 8, 7}, /* bit  6 */
        {-8, 8, 8, 8}, /* bit  7 */
        { 8,-7, 7, 8}, /* bit  8 */
        { 8,-6, 5, 8}, /* bit  9 */
        { 8,-5, 4, 8}, /* bit 10 */
        { 8,-4, 3, 8}, /* bit 11 */
        { 8,-3, 2, 8}, /* bit 12 */
        { 8,-2, 1, 8}, /* bit 13 */
        { 8,-1, 0, 8}, /* bit 14 */
        { 0, 0, 0, 0}  /* STOP   */
    };
    pixel_t     value;
    int         x0,y0,x1,y1,bit,data;

    value.valid = value.hard = value.soft = value.data = 0;
    if ((x == 8) && (y == (VERSIZE(ver)-8))) {
        /* this is a 'special' dark module that is technically not a part
         * of a format info but documented in the same chapter of the
         * standard; see '8.9 Format information' on page 53:
         * "The module in position (4V+9,8) where V is the version number,
         * shall always be dark and does not form part of the Format
         * Information"
         */
        value.hard = value.valid = value.data = 1, value.soft = 0;
    } else {
        data=formatPattern[ver.ecc][ver.xor];
        for (bit=0; formatPosition[bit][0]; bit++,data >>= 1) {
            x0 = formatPosition[bit][0]
                + (formatPosition[bit][0] < 0 ? VERSIZE(ver) : 0);
            y0 = formatPosition[bit][1]
                + (formatPosition[bit][1] < 0 ? VERSIZE(ver) : 0);

            x1 = formatPosition[bit][2]
                + (formatPosition[bit][2] < 0 ? VERSIZE(ver) : 0);
            y1 = formatPosition[bit][3]
                + (formatPosition[bit][3] < 0 ? VERSIZE(ver) : 0);

            if(((x == x0) && (y == y0)) || ((x == x1) && (y == y1))) {
                value.hard = value.valid = 1, value.soft = 0,
                    value.data = data & 1;
                break;
            }
        }
    }
    return value;
}
static pixel_t qrIsFinder(hint_t ver, int x, int y) {
    /*
     * qrIsFinder returns the following:
     * pixel value bitwise OR with the flags if (x,y) is on the finder
     * (large BW squares in the 3 corners of the image)
     * area or -1 if it is not
     */
    static const u_int8_t finderPattern[]
        = {0x7f,0x41,0x5d,0x5d,0x5d,0x41,0x7f,0};
    int         xx, yy, x0, y0;
    pixel_t     value;

    value.valid = value.hard = value.soft = value.data = 0;
    if ((((x < 8)&&((y < 8)||(y >= (VERSIZE(ver)-8)))))
        ||((y < 8) && (x >= (VERSIZE(ver)-8)))) {
        for (yy=0; yy<8; yy++) {
            for (xx=0; xx<8; xx++) {
                y0 = VERSIZE(ver)-yy-1; /* lower-left  */
                x0 = VERSIZE(ver)-xx-1; /* upper-right */
                if (((xx == x) && ((yy == y) || (y == y0)))
                    || ((x == x0) && (y == yy))) {
                    value.valid = value.hard = 1, value.soft = 0,
                        value.data = (finderPattern[yy] >> xx) & 1;
                    goto bye;
                }
            }
        }
    }
    bye: return value;
}
static pixel_t qrIsAlignment(hint_t ver, int x, int y) {
    /*
     * qrIsAlignment returns the following:
     * pixel value bitwise OR with the flags if (x,y) is on the alignment
     * (small squares with the black dot inside)
     * area or -1 if it is not
     */
#if (QRMAX > 0) && (QRMAX < 7)
#define CAPACITY    3
#elif (QRMAX > 6) && (QRMAX < 14)
#define CAPACITY    4
#elif (QRMAX > 13) && (QRMAX < 21)
#define CAPACITY    5
#elif (QRMAX > 20) && (QRMAX < 28)
#define CAPACITY    6
#elif (QRMAX > 27) && (QRMAX < 35)
#define CAPACITY    7
#elif (QRMAX > 35)
#define CAPACITY    8
#endif
    static const u_int8_t patternPosition[][CAPACITY] = {
	/* v1 */ {0},
	/*  2 */ {6, 18, 0},
	/*  3 */ {6, 22, 0},
	/*  4 */ {6, 26, 0},
	/*  5 */ {6, 30, 0},
#if QRMAX > 5
	/*  6 */ {6, 34, 0},
	/*  7 */ {6, 22, 38, 0},
	/*  8 */ {6, 24, 42, 0},
	/*  9 */ {6, 26, 46, 0},
	/* 10 */ {6, 28, 50, 0},
#endif
#if QRMAX > 10
	/* 11 */ {6, 30, 54, 0},		
	/* 12 */ {6, 32, 58, 0},
	/* 13 */ {6, 34, 62, 0},
	/* 14 */ {6, 26, 46, 66,  0},
	/* 15 */ {6, 26, 48, 70,  0},
#endif
#if QRMAX > 15
	/* 16 */ {6, 26, 50, 74,  0},
	/* 17 */ {6, 30, 54, 78,  0},
	/* 18 */ {6, 30, 56, 82,  0},
	/* 19 */ {6, 30, 58, 86,  0},
	/* 20 */ {6, 34, 62, 90,  0},
#endif
#if QRMAX > 20
	/* 21 */ {6, 28, 50, 72,  94, 0},
	/* 22 */ {6, 26, 50, 74,  98, 0},
	/* 23 */ {6, 30, 54, 78, 102, 0},
	/* 24 */ {6, 28, 54, 80, 106, 0},
	/* 25 */ {6, 32, 58, 84, 110, 0},
#endif
#if QRMAX > 25
	/* 26 */ {6, 30, 58, 86, 114, 0},
	/* 27 */ {6, 34, 62, 90, 118, 0},
	/* 28 */ {6, 26, 50, 74,  98, 122, 0},
	/* 29 */ {6, 30, 54, 78, 102, 126, 0},
	/* 30 */ {6, 26, 52, 78, 104, 130, 0},
#endif
#if QRMAX > 30
	/* 31 */ {6, 30, 56, 82, 108, 134, 0},
	/* 32 */ {6, 34, 60, 86, 112, 138, 0},
	/* 33 */ {6, 30, 58, 86, 114, 142, 0},
	/* 34 */ {6, 34, 62, 90, 118, 146, 0},
	/* 35 */ {6, 30, 54, 78, 102, 126, 150, 0},
#endif
#if QRMAX > 35
	/* 36 */ {6, 24, 50, 76, 102, 128, 154, 0},
	/* 37 */ {6, 28, 54, 80, 106, 132, 158, 0},
	/* 38 */ {6, 32, 58, 84, 110, 136, 162, 0},
	/* 39 */ {6, 26, 54, 82, 110, 138, 166, 0},
	/* 40 */ {6, 30, 58, 86, 114, 142, 170, 0}
#endif
    };
    static const int8_t timingPattern[]
        = {0x1f,0x11,0x15,0x11,0x1f};
    /*
     * x0,y0 is not a coordinate per se but the position inside the
     * square matrix of the alignment pattern positions (see the table above)
     * For example, take v7. Table has three values: 6,22,38
     * So the alignment patterns should be on:
     *  6,6     22,6    38,6
     *  6,22    22,22   38,22
     *  6,38    22,38   38,38
     *  Positions 6,6 38,6 and 6,38 must be skipped as they are occupied
     *  by the finder patterns
     */
    int         x0,y0,x1,y1,x2,y2;
    pixel_t     value;

    value.valid = value.hard = value.soft = value.data = 0;
    for (y0=0; patternPosition[ver.ver][y0]; y0++) {
        if (y < (patternPosition[ver.ver][y0]-2)
            || y > (patternPosition[ver.ver][y0]+2)) {
            continue; /* performance hack */
        }
        for (x0=0; patternPosition[ver.ver][x0]; x0++) {
            if (x < (patternPosition[ver.ver][x0]-2)
                || x > (patternPosition[ver.ver][x0]+2)) {
                continue; /* performance hack */
            }
            if (qrIsFinder(ver, patternPosition[ver.ver][x0],
                    patternPosition[ver.ver][y0]).valid) {
                continue; /* already occupied by finder pattern; skip */
            }
            /*
             * (x0,y0) is the pattern base to check the (x,y) pixel against.
             * (x1,y1) are the deltas from the pattern base to the given pixel
             */
            for (y1=0; y1<5; y1++) {
                for (x1=0; x1<5; x1++) {
                    x2 = x1+patternPosition[ver.ver][x0]-2;
                    y2 = y1+patternPosition[ver.ver][y0]-2;
                    if ((x == x2)&&(y == y2)) {
                        value.valid = value.soft = 1,
                            value.data = (timingPattern[y1] >> x1) & 1;
                        goto bye;
                    }
                }
            }
        }
	}
    bye: return value; /* no; (x,y) is not on the info area */
}
static pixel_t qrIsTiming(hint_t ver, int x, int y) {
    /*
     * IMPORTANT: Timing has to be checked AFTER the Finder and Alignment!
     * This is a minor performance optimization. Without that you have to
     * check if timing overlaps the Finder or Alignment
     * qrIsTiming returns the following:
     * pixel value bitwise OR with the flags if (x,y) is on the timing
     * area or -1 if it is not
     */
    pixel_t value;
    int     idx;

    value.valid = value.hard = value.soft = value.data = 0;
    for (idx=8; ((x == 6)||(y == 6)) && idx<VERSIZE(ver)-8; idx++) {
        if (((x == 6) && (y == idx)) || ((x == idx) && (y == 6))) {
            value.valid = value.soft = 1, value.data = ((idx & 1) ^ 1);
            break;
        }
    }
    return value;
}
static pixel_t qrGetSpecial(hint_t ver, int x, int y) {
    static pixel_t (* const special[])(hint_t, int, int) = {
        &qrIsVersionInfo, &qrIsFormatInfo, &qrIsFinder,
        &qrIsAlignment, &qrIsTiming, NULL
    };
    /*
     * tells if the pixel is 'special', ie Finder, Timing or so. Returns
     * pixel value plus flags if so or -1 if pixel isn't special
     * Note: qrIs.. is not user-accessible (there's no point)
     */
    int     idx;
    pixel_t value;

    for (idx = 0; special[idx] != NULL; idx++) {
        if ((value = (*special[idx])(ver,x,y)).valid) {
            break;
        }
    }
    return value;
}
static pixel_t qrOutOfBorder(hint_t ver, int x, int y) {
    pixel_t     pixel;

    return pixel.valid = ((x < 0)||(y < 0)
        ||(x >= VERSIZE(ver))||(y >= VERSIZE(ver))), pixel;
}
static int qrScanV(hint_t ver, int x, int y, int dy) {
    while (!qrOutOfBorder(ver,x,y).valid) {
        if (!qrGetSpecial(ver,x,y).valid) {
            return y;
        }
        y+=dy;
    }
    return -1;
}
void qrSequence(hint_t ver, int bfrLen, u_int8_t *mtrx) {
    /*
     * transfer the buffer data to the matrix (yes, they overlap); note that
     * once you decide upon the version ECC & XOR only change the values of
     * the markup but not the positions. So I use '1' for both as a placeholder
     *
     * Don't forget to 'burn' the ACTUAL markup when you done (that sets up
     * the version info and ECC/XOR info)
     */
    pixel_t     pix,tmp;
    int         x=VERSIZE(ver)-1, y=VERSIZE(ver)-1;
    int         bit,dy=-1; /* start at LRC going up */

#ifdef DEBUG
    qrCnt = 0;
#endif
    for (;;) {
        if ((bit=bfrLoad(1,bfrLen,mtrx)) == -1) {
            /*
             * The buffer is empty but matrix has some pixels left as
             * Remainder Bits; the spec says that the Remaindr Bits MUST be zero
             * even though real world decoders ignore 'em. This might be
             * exploited somehow with some code versions.
             */
            bit = 0;
        }
        qrSetMod(ver,x,y,bit,mtrx);
#ifdef DEBUG
        qrLog[qrCnt++] = (x << 8) | y;
#endif
        if (x == 0 && y == (VERSIZE(ver)-(ver.ver < 6 ? 9 : 12))) {
            /*
             * This is the end of the matrix; prior to the v7 there's no
             * Version Info block; after there's 3 px height Version Info
             */
            break;
        }
        if (((x % 2)==0) ^ (x < 6)) { /* move left */
            x--;
#ifdef DEBUG
            if (qrOutOfBorder(ver,x,y).valid||qrGetSpecial(ver,x,y).valid) {
                assert("We shouldn't ever be here. Really. I mean it." == NULL);
            }
#endif
            continue;
        }
        x++;
        y+=dy; /* move right + up/down */
        pix = qrGetSpecial(ver,x,y); /* cache the current pixel flags */
        if (tmp = qrOutOfBorder(ver,x,y),tmp.valid|(pix.valid&pix.hard)) {
            if (!(tmp.valid||qrGetSpecial(ver,x-1,y).valid)) {
                /* ugly special; glide across LUC version */
                x-=1;
                continue;
            }
            x -= (x==8 ? 3 : 2); /* upper left corner exception */
            y += (dy = -dy); /* undo the vertical step and reverse direction */
            y=qrScanV(ver,x,y,dy);
            continue;
        }
        if (pix.valid && pix.soft) {
            if ((tmp = qrGetSpecial(ver,x,y-dy),tmp.valid && tmp.soft)
                ||(!(tmp = qrGetSpecial(ver,x-1,y),tmp.valid && tmp.soft))) {
                x-=1;
            }
            if ((y=qrScanV(ver,x,y,dy))==-1) { /* RUC exception */
                y=0;
                x-=3;
                dy=-dy;
            }
            continue;
        }
    }
}
/* XOR table */
static int xor0(int i, int j) {
    return ((i+j)%2) ? 0 : 1;
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static int xor1(int i, int j) {
    return (j%2) ? 0 : 1;
}
static int xor2(int i, int j) {
    return (i%3) ? 0 : 1;
}
#pragma GCC diagnostic pop
static int xor3(int i, int j) {
    return ((i+j)%3) ? 0 : 1;
}
static int xor4(int i, int j) {
    return ((j/2+i/3)%2) ? 0 : 1;
}
static int xor5(int i, int j) {
    return ((i*j)%2+(i*j)%3) ? 0 : 1;
}
static int xor6(int i, int j) {
    return (((i*j)%2+(i*j)%3)%2) ? 0 : 1;
}
static int xor7(int i, int j) {
    return (((i*j)%3+(i+j)%2)%2) ? 0 : 1;
}
static int (* const xor[])(int, int) = {
    &xor0, &xor1, &xor2, &xor3, &xor4, &xor5, &xor6, &xor7
};

static const int8_t evalScores[] = {3,3,40,10} ; /* see page 52 */
static int qrTest0(hint_t ver, int x, int y, int v, u_int8_t *mtrx) {
    int	i, pixel = qrGetMod(ver,x,y,mtrx);

    for (i=0; !qrOutOfBorder(ver, v ? x : x+i, v ? y+i : y).valid &&
	    pixel==qrGetMod(ver, v?x:x+i, v?y+i:y, mtrx); i++);
    return i>5 ? evalScores[0]+i-5 : 0;
}
static int qrTest1(hint_t ver, int x, int y, u_int8_t *mtrx) {
    int dx, dy, dxLimit = VERSIZE(ver)-x, m = 1, n = 1;
    int pixel = qrGetMod(ver,x,y,mtrx);

    for (dy=0; dy<(VERSIZE(ver)-y); dy++) {
        for (dx=0; dx<dxLimit; dx++) {
            if (pixel!=qrGetMod(ver,x+dx,y+dy,mtrx)) {
                break;
            }
        }
        if (dx < dxLimit) {
            dxLimit = dx; /* shrink horizontally */
        }
        if (dxLimit <= 1) {
            break; /* single column or no pixels left */
        }
        /* update rectangle only if not the first row */
        if (dy && (m*n < dx*(dy+1))) {
            m = dx;
            n = dy+1;
        }
    }
    return evalScores[1]*(m-1)*(n-1);
}
static int qrTest2(hint_t ver, int x, int y, int v, u_int8_t *mtrx) {
    int i;
    for (i=0; i<7; i++) {
        if (qrOutOfBorder(ver, v?x:x+i, v?y+i:y).valid) {
            return 0;
        }
        if (((0x5d >> i) & 1)!=qrGetMod(ver,(v?x:x+i),(v?y+i:y),mtrx)) {
            return 0;
        }
    }
    return evalScores[2];
}
static int qrEvaluate(hint_t ver, u_int8_t *mtrx) {
    int	x,y,score = 0,black = 0;

    for (x=0; x<VERSIZE(ver); x++) {
        for (y=0; y<VERSIZE(ver); y++) {
            black += qrGetMod(ver,x,y,mtrx) & 1; /* for #4 */

            score += qrTest0(ver,x,y,0,mtrx); /* horizontal */
            score += qrTest0(ver,x,y,1,mtrx); /* vertical   */

            score += qrTest1(ver,x,y,mtrx);

            score += qrTest2(ver,x,y,0,mtrx); /* horizontal */
            score += qrTest2(ver,x,y,1,mtrx); /* vertical   */
        }
    }
    black = (VERSIZE(ver)*VERSIZE(ver)-black) / 100;
    score += (black < 50 ? 50 - black : black - 50) * evalScores[3] / 5;

    return score;
}
static void qrApplyXOR(hint_t ver, u_int8_t *mtrx) {
    int	    i,j;
    pixel_t pixel;

    for (j=0; j<VERSIZE(ver); j++) {
        for (i=0; i<VERSIZE(ver); i++) {
            if ((pixel = qrGetSpecial(ver, i, j)).valid) {
                /* special pixel; set the value */
                qrSetMod(ver,i,j,pixel.data,mtrx);
            } else {
                /* regular pixel; XOR according to the matrix */
                qrXorMod(ver,i,j,(*xor[ver.xor])(i,j),mtrx);
            }
        }
    }
}
hint_t qrXor(hint_t ver, u_int8_t *mtrx) {
    /* if hint2xor > 0 then this IS the best mask (plus one) */
    int     mask,bestMask = -1;
    int	    value,bestValue = 0;

    for (mask=0; mask<8; mask++) {
        qrApplyXOR((ver.xor = mask, ver), mtrx);
        value =  qrEvaluate(ver, mtrx);
        qrApplyXOR(ver, mtrx); /* undo the mask */
        if (bestMask == -1 || value < bestValue) {
            bestMask = mask;
            bestValue = value;
        }
    }
    qrApplyXOR((ver.xor = bestMask,ver), mtrx);
    return ver;
}
