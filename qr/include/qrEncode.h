#ifndef __QRENCODE_H__
#define __QRENCODE_H__
#include "qrEnum.h"

/*
 *  qrEncode takes the following:
 *  ver:    QR code version minus one; use 0x40 to dry-run
 *  url:    URL, encode it as text
 *  srcLen: how many bytes to encode? (reversed order)
 *  bufLen: storage buffer length
 *  buf:    storage buffer
 *
 *  Output:
 *  Number of BITS encoded or -1 if it doesn't fit.
 *  If no dry-run is requested - buffer is filled with the data.
 *
 */
int qrEncode(hint_t hint, const char *url, size_t srcLen, size_t bufLen, u_int8_t *buf);
#endif
