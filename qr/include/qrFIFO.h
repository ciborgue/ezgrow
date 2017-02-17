#ifndef __QRFIFO_H__
#define __QRFIFO_H__
#include "qrEnum.h"
/*
 * Uniformed access to the source data
 */
u_int8_t src(size_t index, const char *url, size_t srcLen, u_int8_t *buf);
/*
 * bfrGetBfr returns the internal processing buffer (that is either a
 * separate buffer or the *extra* buffer provided by the user for thread
 * safety). Used for Galois field operations and debugging.
 */
u_int8_t *bfrGetBfr(size_t bufLen, u_int8_t *buf);

u_int8_t bfrGetOctet(int index, int bufLen, u_int8_t *buf);
void bfrSetOctet(int index, unsigned value, int bufLen, u_int8_t *buf);

void bfrSetSize(int len, int bufLen, u_int8_t *buf);
int bfrGetSize(int bufLen, u_int8_t *buf);

void bfrStore(int bits, unsigned value, int bufLen, u_int8_t *buf);
int bfrLoad(int bits, int bufLen, u_int8_t *buf);
#endif
