#include <stdio.h>
#include <string.h>

#include <assert.h>
#ifndef DEBUG
#define DEBUG
#endif
#ifndef DEBUG_DETECT
#define DEBUG_DETECT
#endif

#include "qrEnum.h"
#include "QR.c"

#include "qrEncode.c"
#include "qrFIFO.c"

/*
 * x x x x
 * ! ! ! +- use Ver
 * ! ! +--- strict Ver
 * ! +----- use ECC
 * +------- strict ECC
 */
struct  {
    int         ver,ecc,num,vEcc,vVer,vValid;
} ecc[] = { /*{{{*/
    { 0x400, eccL, 9, eccH, 0, 1 },
    { 0x400, eccH, 10, -1, 0, 0 },
    { 0x400, eccL, 13, eccQ, 0, 1 },
    { 0x400, eccQ, 14, -1, 0, 0 },
    { 0x400, eccL, 16, eccM, 0, 1 },
    { 0x400, eccM, 17, -1, 0, 0 },
    { 0x400, eccL, 19, eccL, 0, 1 },
    { 0x400, eccL, 20, -1, 0, 0 },

    { 0x404, eccL, 46, eccH, 0, 1 },
    { 0x404, eccH, 47, -1, 0, 0 },
    { 0x404, eccL, 62, eccQ, 0, 1 },
    { 0x404, eccQ, 63, -1, 0, 0 },
    { 0x404, eccL, 86, eccM, 0, 1 },
    { 0x404, eccM, 87, -1, 0, 0 },
    { 0x404, eccL, 108, eccL, 0, 1 },
    { 0x404, eccL, 109, -1, 0, 0 },

    { -1, -1, -1, -1, -1 }
}; /*}}}*/
struct  {
    int         ver,ecc,num,vEcc,vVer,vValid;
} ver[] = { /*{{{*/
    { 0x000, eccL, 23, eccM, 1, 1 },
    { 0x102, eccL, 23, eccH, 2, 1 },
    { 0xD02, eccL, 23, eccL, 2, 1 },
    { 0xC00, eccH, 23, eccH, 2, 1 },
    { 0x100, eccL, 8, eccH, 0, 1 },
    { 0x100, eccL, 9, eccH, 0, 1 },
    { 0xf00, eccH, 10, -1, 0, 0 },
    { 0x100, eccL, 10, eccQ, 1, 1 },
    { 0x100, eccL, 13, eccQ, 0, 1 },
    { 0x100, eccL, 14, eccM, 0, 1 },
    { 0x100, eccL, 16, eccM, 0, 1 },
    { 0x100, eccL, 17, eccL, 0, 1 },
    { 0x100, eccL, 19, eccL, 0, 1 },
/*    { 0x101, eccL, 23, eccQ, 1, 1 }, */
    { -1, -1, -1, -1, -1 }
}; /*}}}*/

int main(int argc, char *argv[]) {
    int         i,j;
    hint_t      hint;

    printf("Testing detectEcc ");
    for (i=0; ecc[i].ver != -1; i++) {
        *((int *)&hint) = 0;

        hint.ecc = ecc[i].ecc;
        hint.ver = (j = ecc[i].ver) & 0xff;
        hint.useVer = (j >> 8) & 1;
        hint.strictVer = (j >> 9) & 1;
        hint.useEcc = (j >> 10) & 1;
        hint.strictEcc = (j >> 11) & 1;

        /*
        fprintf(stderr, "\t>ver: %d ecc: %d %d\n",
            hint.ver, hint.ecc, hint.useEcc);
        */
        hint = detectEcc(hint, ecc[i].num);
        assert(hint.valid == ecc[i].vValid);
        if (hint.valid) {
            assert(hint.ecc == ecc[i].vEcc);
        }
        printf(".");
    }
    printf(" done\n");

    printf("Testing detectVer ");
    for (i=0; ver[i].ver != -1; i++) {
        *((int *)&hint) = 0;

        hint.ecc = ver[i].ecc;
        hint.ver = (j = ver[i].ver) & 0xff;
        hint.useVer = (j >> 8) & 1;
        hint.strictVer = (j >> 9) & 1;
        hint.useEcc = (j >> 10) & 1;
        hint.strictEcc = (j >> 11) & 1;

        fprintf(stderr, "\tbytes: %d ver: %d ecc: %d v:%d sv:%d e:%d se:%d\n",
            ver[i].num, hint.ver, hint.ecc,
                hint.useVer, hint.strictVer,
                hint.useEcc, hint.strictEcc);

        hint = detectVer(hint, NULL, ver[i].num, 0, NULL);
        fprintf(stderr, "\tver: %d ecc: %d valid: %d\n",
            hint.ver, hint.ecc, hint.valid);
        assert(hint.valid == ver[i].vValid);
        if (hint.valid) {
            assert(hint.ecc == ver[i].vEcc);
        }
        printf(".");
    }
    printf(" done\n");

    return 0;
}
