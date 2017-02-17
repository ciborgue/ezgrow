#include "qrBitMagic.h"

static int qrIndex(hint_t hint, int x, int y) {
#ifdef USE_ASSERTS
    assert((x >= 0) && (y >= 0) && (x < VERSIZE(hint)) && (y < VERSIZE(hint)));
#endif
    return (VERSIZE(hint)-x-1) * ((VERSIZE(hint) >> 3)
        + (VERSIZE(hint) & 7 ? 1 : 0)) + (y >> 3);
}
void qrXorMod(hint_t hint, int x, int y, int data, u_int8_t *mtrx) {
    int index = qrIndex(hint,x,y = VERSIZE(hint) - y - 1);

    mtrx[index] ^= ((data & 1) << (y & 7));
}
void qrSetMod(hint_t hint, int x, int y, int data, u_int8_t *mtrx) {
    int index = qrIndex(hint,x,y = VERSIZE(hint) - y - 1);

    mtrx[index] = (mtrx[index] & (~(1 << (y & 7))))
        | ((data & 1) << (y & 7));
}
int qrGetMod(hint_t hint, int x, int y, u_int8_t *mtrx) {
    int index = qrIndex(hint,x,y = VERSIZE(hint) - y - 1);

    return (mtrx[index] & (1 << (y & 7))) != 0;
}
