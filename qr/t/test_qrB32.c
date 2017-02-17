/* $Id: test_qrB32.c 637 2013-04-01 05:24:53Z  $ */
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

#include "QR.c"

struct { /* {{{ */
    u_int8_t    src[128], dst[128];
} data[] = {
    { "f", "WWWWWWOC" },
    { "fo", "WWWWGNPC" },
    { "foo", "WWWUMNPC" },
    { "foob", "WGOUMNPC" },
    { "fooba", "1JOUMNPC" },
    { "foobar", "WWWWWW8E1JOUMNPC" },
    { "", "" }
}; /* }}} */
int main(int argc, char *argv[]) {
    u_int8_t    buf[1024];
    int         i,j,k;

    printf("Testing encodings ");
    for (i=0; (j=strlen((char *)data[i].src)) != 0; i++) {
        printf(".");
        memcpy(buf,data[i].src,j);
        k=qrEncodeB32(j,1024,buf);
        assert(k == strlen((char *)data[i].dst));
        for (j=0; j<k; j++) {
            assert(buf[j] == data[i].dst[j]);
        }
    }
    printf(" done\n");

    return 0;
}
