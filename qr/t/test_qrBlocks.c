#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "qrBlocks.c"

#define	VER	5
void test_qrTotalSize(void) { /* {{{ */
    int     i;
    hint_t  hint;
    struct  {
        int     ver,ecc,all,value;
    } data[] = {
        {0,eccL,0,19},
        {0,eccM,0,16},
        {0,eccQ,0,13},
        {0,eccH,0, 9},

        {1,eccL,0,34},
        {1,eccM,0,28},
        {1,eccQ,0,22},
        {1,eccH,0,16},

        {1,eccL,1,44},
        {1,eccM,1,44},
        {1,eccQ,1,44},
        {1,eccH,1,44},

        {6,eccL,0,156},
        {6,eccM,0,124},
        {6,eccQ,0,88},
        {6,eccH,0,66},

        {-1,-1,-1,-1}
    };
    printf("Testing qrTotalSize ");
    for (i=0; data[i].ver != -1; i++) {
        hint.ver = data[i].ver;
        hint.ecc = data[i].ecc;
        assert(qrTotalSize(hint,data[i].all) == data[i].value);
        printf(".");
    }
    printf(" ok\n");
} /* }}} */
void test_tableConsistency(void) { /* {{{ */
    static int ecc[] = { /* possible e lengths */
        7, 10, 13, 15, 16, 17, 18, 20, 22, 24, 26, 28,
        30, 32, 34, 36, 40, 42, 44, 46, 48, 50, 52, 54,
        56, 58, 60, 62, 64, 66, 68, -1
    };
    hint_t  eL, eM, eQ, eH;
    hint_t  el, em, eq, eh;
    int     i,j,e;

    printf("Testing table consistency ");
    for (i=0; i<QRMAX; i++) {
        eL.ver = eM.ver = eQ.ver = eH.ver = i;
        el.ecc = eL.ecc = eccL;
        em.ecc = eM.ecc = eccM;
        eq.ecc = eQ.ecc = eccQ;
        eh.ecc = eH.ecc = eccH;

        /* make sure there's less data than data+e, C.O. */
        assert(qrTotalSize(eL,0)<qrTotalSize(eL,1));
        assert(qrTotalSize(eM,0)<qrTotalSize(eM,1));
        assert(qrTotalSize(eH,0)<qrTotalSize(eH,1));
        assert(qrTotalSize(eQ,0)<qrTotalSize(eQ,1));

        /* higher e has less data then the lower, C.O. */
        assert(qrTotalSize(eL,0)>qrTotalSize(eM,0));
        assert(qrTotalSize(eM,0)>qrTotalSize(eQ,0));
        assert(qrTotalSize(eQ,0)>qrTotalSize(eH,0));

        /* higher version has more full capacity then the lower */
        if (i > 0) {
            el.ver = em.ver = eq.ver = eh.ver = i-1;
            assert(qrTotalSize(em,1)<qrTotalSize(eM,1));
            assert(qrTotalSize(el,1)<qrTotalSize(eL,1));
            assert(qrTotalSize(eh,1)<qrTotalSize(eH,1));
            assert(qrTotalSize(eq,1)<qrTotalSize(eQ,1));
        }

        /* all e levels should have the same FULL capacity */
        assert(qrTotalSize(eL,1)==qrTotalSize(eM,1));
        assert(qrTotalSize(eL,1)==qrTotalSize(eQ,1));
        assert(qrTotalSize(eL,1)==qrTotalSize(eH,1));

        for (e=0; e<4; e++) { /* nasty hack; eM=0 eL=1 etc. */
            for (j=0; ecc[j]!=-1; j++) {
                eL.ecc = e;
                if (qrShortSize(eL,1)-qrShortSize(eL,0)==ecc[j])
                    break;
            }
            assert(ecc[j]!=-1);
        }
        printf(".");
    }
    printf(" ok\n");
} /* }}} */
int main(void) { /* {{{ */
    test_qrTotalSize();
    test_tableConsistency();
    return 0;
} /* }}} */
