#ifndef __QRMISC_H__
#define __QRMISC_H__
#include <sys/types.h>
#include <string.h>
#include "qrEnum.h"

void    bfrSetSize(int len, u_int8_t *buf);
int     bfrGetSize(u_int8_t *buf);
int16_t loadByte(int bufLen, u_int8_t *buf);

#ifdef DEBUG
u_int8_t    *getBuffer(void);
#endif
#endif
