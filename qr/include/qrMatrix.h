#ifndef __QRMATRIX_H__
#define __QRMATRIX_H__
#include "qrEnum.h"

void qrSequence(hint_t ver, int bfrLen, u_int8_t *mtrx);
void qrBurnMarkup(hint_t ver, u_int8_t *mtrx);
hint_t qrXor(hint_t ver, u_int8_t *mtrx);
#endif
