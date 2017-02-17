#ifndef __QRBLOCKS_H__
#define __QRBLOCKS_H__

/* 'full'=1 means to give size with ECC, =0 without */
int qrShortBlocks(hint_t hint);
int qrShortSize(hint_t hint, int full);
int qrLongBlocks(hint_t hint);
int qrLongSize(hint_t hint, int full);

int qrTotalBlocks(hint_t hint);
int qrTotalSize(hint_t hint, int full);

int qrBlockLength(hint_t hint, int block);
int qrBlockOffset(hint_t hint, int block);
#endif
