#include <stdio.h>
#include <sys/types.h>

#include <assert.h>

#include "qrShuffle.c"

struct {
    int         ver,ecc;
    u_int8_t    dst[1024];
} data[] = {
    {2, eccQ, { /* 3Q */
        /* data, 34 bytes */
         0,17, 1,18, 2,19, 3,20, 4,21, 5,22, 6,23, 7,24, 8,
        25, 9,26,10,27,11,28,12,29,13,30,14,31,15,32,16,33,
        /* ecc, 36 bytes */
         0,18, 1,19, 2,20, 3,21, 4,22, 5,23, 6,24, 7,25, 8,26,
         9,27,10,28,11,29,12,30,13,31,14,32,15,33,16,34,17,35
    }},
    {4, eccQ, { /* 5Q */
        /* data, 62 bytes */
         0,15,30,46, 1,16,31,47, 2,17,32,48, 3,18,33,49, 4,19,34,50,
         5,20,35,51, 6,21,36,52, 7,22,37,53, 8,23,38,54, 9,24,39,55,
        10,25,40,56,11,26,41,57,12,27,42,58,13,28,43,59,14,29,44,60,
        45,61,
        /* ecc, 72 bytes */
         0,18,36,54, 1,19,37,55, 2,20,38,56, 3,21,39,57, 4,22,40,58,
         5,23,41,59, 6,24,42,60, 7,25,43,61, 8,26,44,62, 9,27,45,63,
        10,28,46,64,11,29,47,65,12,30,48,66,13,31,49,67,14,32,50,68,
        15,33,51,69,16,34,52,70,17,35,53,71
    }},
    {0, eccH, { /* 1H */
        /* data, 9 bytes */
         0, 1, 2, 3, 4, 5, 6, 7, 8,
        /* ecc, 17 bytes */
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16
    }},
    {-1, -1, {
        0
    }}
};

int main(void) {
    u_int8_t    buffer[1024];
    int         test,maxIndex,index,power;
    hint_t      hint;

    idx_t   dx;
    hint_t  ver;

    if (1) { /*{{{ rc2i */
        printf("Testing selected samples for the rc2i ");
        hint.ver = 0, hint.ecc = eccH;

        /* v5 */
        hint.ver = 4, hint.ecc = eccQ;
        /* fwd */
        index = rc2i(hint,(dx.row=0,dx.col=0,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 0); printf(".");

        index = rc2i(hint,(dx.row=0,dx.col=14,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 14); printf(".");

        index = rc2i(hint,(dx.row=1,dx.col=0,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 15); printf(".");

        index = rc2i(hint,(dx.row=1,dx.col=14,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 29); printf(".");

        index = rc2i(hint,(dx.row=2,dx.col=0,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 30); printf(".");

        index = rc2i(hint,(dx.row=2,dx.col=15,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 45); printf(".");

        index = rc2i(hint,(dx.row=3,dx.col=0,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 46); printf(".");

        index = rc2i(hint,(dx.row=3,dx.col=15,dx.fwd=1,dx.ecc=0,dx));
        assert(index == 61); printf(".");

        /* rev */
        index = rc2i(hint,(dx.row=0,dx.col=0,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 0); printf(".");

        index = rc2i(hint,(dx.row=0,dx.col=14,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 56); printf(".");

        index = rc2i(hint,(dx.row=1,dx.col=0,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 1); printf(".");

        index = rc2i(hint,(dx.row=1,dx.col=14,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 57); printf(".");

        index = rc2i(hint,(dx.row=2,dx.col=0,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 2); printf(".");

        index = rc2i(hint,(dx.row=2,dx.col=15,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 60); printf(".");

        index = rc2i(hint,(dx.row=3,dx.col=0,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 3); printf(".");

        index = rc2i(hint,(dx.row=3,dx.col=15,dx.fwd=0,dx.ecc=0,dx));
        assert(index == 61); printf(".");

        printf(" ok\n");
    } /*}}}*/
    if (1) { /*{{{ i2rc */
        printf("Testing selected samples for the i2rc ");

        /* v5 */
        hint.ver = 4, hint.ecc = eccQ;
        /* fwd */
        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),0);
        assert(dx.row == 0 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),14);
        assert(dx.row == 0 && dx.col == 14); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),15);
        assert(dx.row == 1 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),29);
        assert(dx.row == 1 && dx.col == 14); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),30);
        assert(dx.row == 2 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),45);
        assert(dx.row == 2 && dx.col == 15); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),46);
        assert(dx.row == 3 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=1,dx.ecc=0,dx),61);
        assert(dx.row == 3 && dx.col == 15); printf(".");

        /* rev */
        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),0);
        assert(dx.row == 0 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),56);
        assert(dx.row == 0 && dx.col == 14); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),1);
        assert(dx.row == 1 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),57);
        assert(dx.row == 1 && dx.col == 14); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),2);
        assert(dx.row == 2 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),60);
        assert(dx.row == 2 && dx.col == 15); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),3);
        assert(dx.row == 3 && dx.col == 0); printf(".");

        dx = i2rc(hint,(dx.fwd=0,dx.ecc=0,dx),61);
        assert(dx.row == 3 && dx.col == 15); printf(".");

        printf(" ok\n");
    } /*}}}*/

    printf("Testing shuffle index translator ");
    for (test = 0; data[test].ver != -1; test++) {
        ver.ver = data[test].ver;
        ver.ecc = data[test].ecc;

        maxIndex = qrTotalSize(ver,0);

        for (index = 0; index < maxIndex; index++) {
            /* ff */
            dx.fwd = 1; dx.ecc = 0;
            /*
            fprintf(stderr, "ff: %d -> %d, got: %d\n",
                index, data[test].dst[index], idx(ver, dx, index));
            */
            assert(idx(ver, dx, index) == data[test].dst[index]);
            printf(".");

            /* bb */
            dx.fwd = 0; dx.ecc = 0;
            /*
            fprintf(stderr, "bb: %d -> %d, got: %d\n",
                index, data[test].dst[index], idx(ver, dx, index));
            */
            assert(idx(ver, dx, data[test].dst[index]) == index);
            printf(".");
        }
    }
    printf(" done\n");

    printf("Testing shuffle result ");
    for (test = 0; data[test].ver != -1; test++) {
        hint.ver = data[test].ver;
        hint.ecc = data[test].ecc;

        power = qrTotalSize(hint,1)-qrTotalSize(hint,0);
        for (index = 0; index < qrTotalSize(hint,0); index++) {
            buffer[index] = index;
        }
        for (index = 0; index < power; index++) {
            buffer[index+qrTotalSize(hint,0)] = index;
        }
        qrShuffle(hint,buffer);

        for (index = 0; index < qrTotalSize(hint,1); index++) {
            assert(buffer[index] == data[test].dst[index]);
        }
        printf(".");
    }
    printf(" done\n");

    return 0;
}
