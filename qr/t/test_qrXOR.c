/* $Id: test_qrXOR.c 657 2013-04-13 03:42:40Z  $ */
#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <locale.h>
#include <sys/types.h>
#include <assert.h>

#include "qrEnum.h"
#include "qrMatrix.h"
#include "qrInterface.h"

#include "qrMatrix.c"

struct { /* {{{ */
    int         ver;
    int16_t     x,y,h,t,v;
} data[] = {
    { 0, 0, 0, 0, 0, 19},
    { 0, 0, 0, 1, 0, 19},

    { 0, 1, 0, 0, 0, 18},
    { 0, 0, 1, 1, 0, 18},

    { 0, 1, 1, 0, 0, 18},
    { 0, 2, 2, 1, 0, 17},

    { 0,17,17, 0, 1, 27},
    { 0, 0, 0, 0, 1,540},

    { 0, 0, 0, 0, 2,  0},
    { 0,14, 4, 0, 2, 40},
    { 0,14, 4, 1, 2,  0},

    {-1, 0, 0, 0, 0,  0}
}; /* }}} */

int main(int argc, char *argv[]) {
    int         i;
    hint_t       ver;
    u_int8_t	buffer[65536];

    for (i=0; i<65536; i++) {
        buffer[i] = 0;
    }
    printf("Testing eval ");
    for (i=0; data[i].ver != -1; i++) {
        ver.ver = data[i].ver;
        qrSetMod(ver, 10, 10, 1, buffer);

        qrSetMod(ver, 14,  4, 1, buffer);
        qrSetMod(ver, 16,  4, 1, buffer);
        qrSetMod(ver, 17,  4, 1, buffer);
        qrSetMod(ver, 18,  4, 1, buffer);
        qrSetMod(ver, 20,  4, 1, buffer);
        /* fprintf(stderr, "%d %d %d\n", data[i].x,data[i].y,data[i].h); */
        if (data[i].t == 0) {
            assert(data[i].v
                == qrTest0(ver,data[i].x,data[i].y,data[i].h,buffer));
        } else if (data[i].t == 1) {
            assert(data[i].v
                == qrTest1(ver,data[i].x,data[i].y,buffer));
        } else if (data[i].t == 2) {
            assert(data[i].v
                == qrTest2(ver,data[i].x,data[i].y,data[i].h,buffer));
        }
        printf(".");
    }
    printf(" done\n");

    return 0;
}
