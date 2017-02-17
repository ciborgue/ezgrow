#ifndef __QRBITMAGIC_H__
#define __QRBITMAGIC_H__
#include "qrEnum.h"

/*
 * Pixel operation; weird coordinate translation is implemented according
 * to the way matrix is filled with data: mtrx[0] represents the LAST pixel
 * (that is version-dependent, ie (20,20) for ver.1
 * This makes possible to use some portions of the matrix buffer while
 * filling the same matrix bottom-to-top, right-to-left.
 */
void qrSetMod(hint_t ver, int x, int y, int data, u_int8_t *mtrx);
void qrXorMod(hint_t ver, int x, int y, int data, u_int8_t *mtrx);
int qrGetMod(hint_t ver, int x, int y, u_int8_t *mtrx);
#endif
