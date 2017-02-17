#ifndef __QRSHUFFLE_H__
#define __QRSHUFFLE_H__
#include "qrEnum.h"

/*
 *  qrShuffle shuffles (or transposes) the data in the buffer 'buf'
 *      'ver' dictates the amount of data and amount of ECC
 *
 *  Buffer is assumed correct; no checks for NULL or range checks ever attempted
 *
 *  Note: ver.ver and ver.ecc is examined; 'ver.wet' is ignored
 */
int qrShuffle(hint_t ver, u_int8_t *buf);
#endif
