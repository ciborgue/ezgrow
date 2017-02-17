#include "qrEnum.h"
#include "qrBlocks.h"

static unsigned char blockTable[][4][4] = {
    /*
     * { -- one element for each version
     *  { data for M-level },
     *  { data for L-level },
     *  { data for H-level },
     *  { data for Q-level }
     * }
     *
     * Each data is 4 integers (actually, int8s) that are:
     *  - number of 'short' blocks
     *  - total number of bytes in the 'short' block (data+ECC)
     *  - number of data bytes in the 'short' block
     *  - number of 'long' blocks
     *  As 'long' is always one data byte longer than the 'short' there's
     *  no point to keep its characteristics.
     *  Note: field order is historically weird
     *  Note: MLHQ-order follows the ID numerical representaion values for
     *      the bitstream; ie. M is 0, L is 1 and so on.
     */
    { /*  v1 */
        {1,  26,  16, 0}, /* M */
        {1,  26,  19, 0}, /* L */
        {1,  26,   9, 0}, /* H */
        {1,  26,  13, 0}  /* Q */
    },
    { /*  v2 */
        {1,  44,  28, 0}, /* M */
        {1,  44,  34, 0}, /* L */
        {1,  44,  16, 0}, /* H */
        {1,  44,  22, 0}  /* Q */
    },
    { /*  v3 */
        {1,  70,  44, 0}, /* M */
        {1,  70,  55, 0}, /* L */
        {2,  35,  13, 0}, /* H */
        {2,  35,  17, 0}  /* Q */
    },
    { /*  v4 */
        {2,  50,  32, 0}, /* M */
        {1, 100,  80, 0}, /* L */
        {4,  25,   9, 0}, /* H */
        {2,  50,  24, 0}  /* Q */
    },
    { /*  v5 */
        {2,  67,  43, 0}, /* M */
        {1, 134, 108, 0}, /* L */
        {2,  33,  11, 2}, /* H */
        {2,  33,  15, 2}  /* Q */
    },
#if QRMAX > 5
    { /*  v6 */
        {4,  43,  27, 0}, /* M */
        {2,  86,  68, 0}, /* L */
        {4,  43,  15, 0}, /* H */
        {4,  43,  19, 0}  /* Q */
    },
    { /*  v7 */
        {4,  49,  31, 0}, /* M */
        {2,  98,  78, 0}, /* L */
        {4,  39,  13, 1}, /* H */
        {2,  32,  14, 4}  /* Q */
    },
    { /*  v8 */
        {2,  60,  38, 2}, /* M */
        {2, 121,  97, 0}, /* L */
        {4,  40,  14, 2}, /* H */
        {4,  40,  18, 2}  /* Q */
    },
    { /*  v9 */
        {3,  58,  36, 2}, /* M */
        {2, 146, 116, 0}, /* L */
        {4,  36,  12, 4}, /* H */
        {4,  36,  16, 4}  /* Q */
    },
    { /* v10 */
        {4,  69,  43, 1}, /* M */
        {2,  86,  68, 2}, /* L */
        {6,  43,  15, 2}, /* H */
        {6,  43,  19, 2}  /* Q */
    },
#endif
#if QRMAX > 10
    { /* v11 */
        {1,  80,  50, 4}, /* M */
        {4, 101,  81, 0}, /* L */
        {3,  36,  12, 8}, /* H */
        {4,  50,  22, 4}  /* Q */
    },
    { /* v12 */
        {6,  58,  36, 2}, /* M */
        {2, 116,  92, 2}, /* L */
        {7,  42,  14, 4}, /* H */
        {4,  46,  20, 6}  /* Q */
    },
    { /* v13 */
        { 8,  59,  37, 1}, /* M */
        { 4, 133, 107, 0}, /* L */
        {12,  33,  11, 4}, /* H */
        { 8,  44,  20, 4}  /* Q */
    },
    { /* v14 */
        { 4,  64,  40, 5}, /* M */
        { 3, 145, 115, 1}, /* L */
        {11,  36,  12, 5}, /* H */
        {11,  36,  16, 5}  /* Q */
    },
    { /* v15 */
        { 5,  65,  41, 5}, /* M */
        { 5, 109,  87, 1}, /* L */
        {11,  36,  12, 7}, /* H */
        { 5,  54,  24, 7}  /* Q */
    },
#endif
#if QRMAX > 15
    { /* v16 */
        { 7,  73,  45, 3}, /* M */
        { 5, 122,  98, 1}, /* L */
        { 3,  45,  15,13}, /* H */
        {15,  43,  19, 2}  /* Q */
    },
    { /* v17 */
        {10,  74,  46, 1}, /* M */
        { 1, 135, 107, 5}, /* L */
        { 2,  42,  14,17}, /* H */
        { 1,  50,  22,15}  /* Q */
    },
    { /* v18 */
        { 9,  69,  43, 4}, /* M */
        { 5, 150, 120, 1}, /* L */
        { 2,  42,  14,19}, /* H */
        {17,  50,  22, 1}  /* Q */
    },
    { /* v19 */
        { 3,  70,  44,11}, /* M */
        { 3, 141, 113, 4}, /* L */
        { 9,  39,  13,16}, /* H */
        {17,  47,  21, 4}  /* Q */
    },
    { /* v20 */
        { 3,  67,  41,13}, /* M */
        { 3, 135, 107, 5}, /* L */
        {15,  43,  15,10}, /* H */
        {15,  54,  24, 5}  /* Q */
    },
#endif
#if QRMAX > 20
    { /* v21 */
        {17,  68,  42, 0}, /* M */
        { 4, 144, 116, 4}, /* L */
        {19,  46,  16, 6}, /* H */
        {17,  50,  22, 6}  /* Q */
    },
    { /* v22 */
        {17,  74,  46, 0}, /* M */
        { 2, 139, 111, 7}, /* L */
        {34,  37,  13, 0}, /* H */
        { 7,  54,  24,16}  /* Q */
    },
    { /* v23 */
        { 4,  75,  47,14}, /* M */
        { 4, 151, 121, 5}, /* L */
        {16,  45,  15,14}, /* H */
        {11,  54,  24,14}  /* Q */
    },
    { /* v24 */
        { 6,  73,  45,14}, /* M */
        { 6, 147, 117, 4}, /* L */
        {30,  46,  16, 2}, /* H */
        {11,  54,  24,16}  /* Q */
    },
    { /* v25 */
        { 8,  75,  47,13}, /* M */
        { 8, 132, 106, 4}, /* L */
        {22,  45,  15,13}, /* H */
        { 7,  54,  24,22}  /* Q */
    },
#endif
#if QRMAX > 25
    { /* v26 */
        {19,  74,  46, 4}, /* M */
        {10, 142, 114, 2}, /* L */
        {33,  46,  16, 4}, /* H */
        {28,  50,  22, 6}  /* Q */
    },
    { /* v27 */
        {22,  73,  45, 3}, /* M */
        { 8, 152, 122, 4}, /* L */
        {12,  45,  15,28}, /* H */
        { 8,  53,  23,26}  /* Q */
    },
    { /* v28 */
        { 3,  73,  45,23}, /* M */
        { 3, 147, 117,10}, /* L */
        {11,  45,  15,31}, /* H */
        { 4,  54,  24,31}  /* Q */
    },
    { /* v29 */
        {21,  73,  45, 7}, /* M */
        { 7, 146, 116, 7}, /* L */
        {19,  45,  15,26}, /* H */
        { 1,  53,  23,37}  /* Q */
    },
    { /* v30 */
        {19,  75,  47,10}, /* M */
        { 5, 145, 115,10}, /* L */
        {23,  45,  15,25}, /* H */
        {15,  54,  24,25}  /* Q */
    },
#endif
#if QRMAX > 30
    { /* v31 */
        { 2,  74,  46,29}, /* M */
        {13, 145, 115, 3}, /* L */
        {23,  45,  15,28}, /* H */
        {42,  54,  24, 1}  /* Q */
    },
    { /* v32 */
        {10,  74,  46,23}, /* M */
        {17, 145, 115, 0}, /* L */
        {19,  45,  15,35}, /* H */
        {10,  54,  24,35}  /* Q */
    },
    { /* v33 */
        {14,  74,  46,21}, /* M */
        {17, 145, 115, 1}, /* L */
        {11,  45,  15,46}, /* H */
        {29,  54,  24,19}  /* Q */
    },
    { /* v34 */
        {14,  74,  46,23}, /* M */
        {13, 145, 115, 6}, /* L */
        {59,  46,  16, 1}, /* H */
        {44,  54,  24, 7}  /* Q */
    },
    { /* v35 */
        {12,  75,  47,26}, /* M */
        {12, 151, 121, 7}, /* L */
        {22,  45,  15,41}, /* H */
        {39,  54,  24,14}  /* Q */
    },
#endif
#if QRMAX > 35
    { /* v36 */
        { 6,  75,  47,34}, /* M */
        { 6, 151, 121,14}, /* L */
        { 2,  45,  15,64}, /* H */
        {46,  54,  24,10}  /* Q */
    },
    { /* v37 */
        {29,  74,  46,14}, /* M */
        {17, 152, 122, 4}, /* L */
        {24,  45,  15,46}, /* H */
        {49,  54,  24,10}  /* Q */
    },
    { /* v38 */
        {13,  74,  46,32}, /* M */
        { 4, 152, 122,18}, /* L */
        {42,  45,  15,32}, /* H */
        {48,  54,  24,14}  /* Q */
    },
    { /* v39 */
        {40,  75,  47, 7}, /* M */
        {20, 147, 117, 4}, /* L */
        {10,  45,  15,67}, /* H */
        {43,  54,  24,22}  /* Q */
    },
    { /* v40 */
        {18,  75,  47,31}, /* M */
        {19, 148, 118, 6}, /* L */
        {20,  45,  15,61}, /* H */
        {34,  54,  24,34}  /* Q */
    }
#endif
};
int qrShortBlocks(hint_t hint) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return blockTable[hint.ver][hint.ecc][0];
}
int qrShortSize(hint_t hint, int full) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return blockTable[hint.ver][hint.ecc][full ? 1 : 2];
}
int qrLongBlocks(hint_t hint) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return blockTable[hint.ver][hint.ecc][3];
}
int qrLongSize(hint_t hint, int full) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    /* return 'short' block if there's no 'long' blocks */
    return qrShortSize(hint,full)+(blockTable[hint.ver][hint.ecc][3] ? 1 : 0);
}
int qrTotalBlocks(hint_t hint) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return qrShortBlocks(hint)+qrLongBlocks(hint);
}
int qrTotalSize(hint_t hint, int full) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return qrShortBlocks(hint)*qrShortSize(hint,full)+
        qrLongBlocks(hint)*qrLongSize(hint,full);
}
int qrBlockLength(hint_t hint, int block) {
#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    return (block<qrShortBlocks(hint)
        ? qrShortSize(hint,0) : qrLongSize(hint,0));
}
int qrBlockOffset(hint_t hint, int block) {
    int offset;

#ifdef USE_ASSERTS
    assert((hint.ver<QRMAX)||(hint.ecc<eccH));
#endif
    /* 'short' blocks */
    offset = qrShortSize(hint,0) *
        (block<qrShortBlocks(hint) ? block : qrShortBlocks(hint));

    /* 'long' blocks */
    offset += qrLongSize(hint,0) *
        (block<qrShortBlocks(hint) ? 0 : block-qrShortBlocks(hint));

    return offset;
}
