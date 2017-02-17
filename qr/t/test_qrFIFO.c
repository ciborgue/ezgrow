#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "qrEnum.h"
#include "qrFIFO.h"

#define BSIZE   (QRBUFFER+32)
u_int8_t    buffer[BSIZE];

char    *buf = "aboof";

void dumpPb(u_int8_t *pb) { /*{{{*/
    int i;

    for (i = 0; i < QRBUFFER; i++) {
        printf("%02x ", pb[i]);
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    printf("---\n");
    for (i = 0; i < QRBUFFER; i++) {
        printf("%02x ", buffer[i]);
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    printf("\n");
} /*}}}*/
int main(int argc, char *argv[]) {
    int         i,index,srcLen = strlen(buf);
    u_int8_t    *pb = bfrGetBfr(BSIZE,buffer);

    printf("Testing bfrSetSize/bfrGetSize ");
    for (i = 0; i < 65536; i++) {
        bfrSetSize(i, QRBUFFER, buffer);
        assert(bfrGetSize(QRBUFFER, buffer) == i);
        assert(pb[QRBUFFER-2] == (i >> 8));
        assert(pb[QRBUFFER-1] == (i & 0xff));
        if ((i % 512) == 0) {
            printf(".");
        }
    }
    bfrSetSize(0, QRBUFFER, buffer);
    printf(" done\n");

    printf("Testing bfrSetOctet/bfrGetOctet ");
    for (i = 0; i < BSIZE; i++) {
        bfrSetOctet(i, i % 256, QRBUFFER, buffer);
        assert(bfrGetOctet(i, QRBUFFER, buffer) == (i % 256));
        if ((i % 16) == 0) {
            printf(".");
        }
    }
    for (i = 0; i < QRBUFFER; i++) {
        bfrSetOctet(i, 0, QRBUFFER, buffer);
    }
    printf(" done\n");

    bfrStore(1, 1, QRBUFFER, buffer);
    bfrStore(2, 1, QRBUFFER, buffer);
    bfrStore(3, 2, QRBUFFER, buffer);
    bfrStore(2, 2, QRBUFFER, buffer);

    assert(pb[QRBUFFER-1] == 8);
    assert(pb[QRBUFFER-2] == 0);
    assert(pb[QRBUFFER-3] == 0xaa);

    /*
    if (0) {
        bfrSetOctet(0, 0xaa, QRBUFFER, buffer);
        bfrSetOctet(1, 0x55, QRBUFFER, buffer);
        bfrSetOctet(2, 0xaa, QRBUFFER, buffer);
        bfrSetSize(24, QRBUFFER, buffer);
        bfrStore(1, 1, QRBUFFER, buffer);
        assert(memcmp("\x01\x54\xab\x55\x00\x19",
            pb+QRBUFFER-6, 6) == 0);
        bfrStore(3, 7, QRBUFFER, buffer);
        assert(memcmp("\x0a\xa5\x5a\xaf\x00\x1c",
            pb+QRBUFFER-6, 6) == 0);
    }
    if (1) {
        memcpy(pb+QRBUFFER-5, "\xf1\x34\x5b\x00\x18", 5);
        assert(memcmp("\xf1\x34\x5b\x00\x18",
            pb+QRBUFFER-5, 5) == 0);
        dumpPb(pb);
        printf("loading bits: %x\n", bfrLoad(9, QRBUFFER, buffer));
        printf("loading bits: %x\n", bfrLoad(15, QRBUFFER, buffer));
        dumpPb(pb);
    }
    */
    for (index = 0; index < srcLen; index++) {
        bfrSetOctet(index, buf[index], QRBUFFER, buffer);
    }
    bfrSetSize(srcLen << 3, QRBUFFER, buffer); /* set size in bits, not bytes */
    dumpPb(pb);

    return 0;
}
