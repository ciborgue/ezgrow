/* $Id: UNICODE.c 671 2013-04-27 17:29:37Z  $ */
#define UNICODE
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

#include "qrBitMagic.h"
#include "QR.h"

#define BUFLEN  4096

char	buffer[BUFLEN];

void usage(void) { /* {{{ */
    wprintf(L"%s%s", "usage: qrUNICODE -x xor -v ver -e ecc ",
        "-t text\n\twhere ecc is 0..3\n");
    exit(0);
} /* }}} */
int parseToPckt(char *data, char *pckt) { /* {{{ */
    /**
     * Decodes the supplied binary data from the ASCII encoding (in data)
     * to the true binary data (to pckt). Returns pckt length (in bytes)
     */
    int	i,v;

    for (i=0; data[i]!=0; i++) {
        v = 0;
        if ((data[i]>0x2f) && (data[i]<0x3a)) v = data[i] ^ 0x30; /*0-9*/
        if ((data[i]>0x40) && (data[i]<0x47)) v = (data[i] ^ 0x40) + 9; /*A-F*/
        if ((data[i]>0x60) && (data[i]<0x67)) v = (data[i] ^ 0x60) + 9; /*a-f*/
        if (i % 2) {
            pckt[i/2] |= v & 15;
        } else {
            pckt[i/2] = v << 4;
        }
    }
    return i/2;
} /* }}} */
int main(int argc, char *argv[]) {
    const int cp[] = { 0x20, 0x2580, 0x2584, 0x2588 };
    int	    x,y,ch;
    int	    ver = -1, ecc = -1;
    hint_t  hint,v;
    char    *text = NULL;

    int	    pcktLen = 0;
    char    pckt[4096];

    setlocale(LC_ALL,"");

    hint.useVer = hint.useEcc = 0;

    while ((ch = getopt(argc, argv, "p:v:e:t:")) != -1) {
        switch (ch) {
            case 'v':
                sscanf(optarg, "%d", &ver);
                hint.useVer = 1;
                if (ver <= 0) {
                    hint.ver = -ver;
                    hint.strictVer = 1;
                } else {
                    hint.ver = ver;
                    hint.strictVer = 0;
                }
                break;
            case 'e':
                sscanf(optarg, "%d", &ecc);
                hint.useEcc = 1;
                if (ecc <= 0) {
                    hint.ecc = -ecc;
                    hint.strictEcc = 1;
                } else {
                    hint.ecc = ecc;
                    hint.strictEcc = 0;
                }
                break;
            case 'p':
                pcktLen += parseToPckt(optarg,pckt+pcktLen);
                break;
            case 't':
                text = optarg;
                break;
            default:
            usage();
        }
    }

    if (text == NULL) usage();

    v=QR(hint, (const char *)text, 0, BUFLEN, (u_int8_t *)buffer);

    fwprintf(stderr, L"[%s] ", (v.valid ? "VALID" : "INVALID"));
    fwprintf(stderr, L"VER: %d ECC: %d XOR: %d\n", v.ver, v.ecc, v.xor);
    if (!v.valid) {
        return 1;
    }

#ifdef PBM
    wprintf(L"P1\n%d %d\n", VERSIZE(v), VERSIZE(v));
    for (y=0; y<VERSIZE(v); y++) {
        for (x=0; x<VERSIZE(v); x++) {
            ch = qrGetMod(v, x, y, (u_int8_t *)buffer) ? 1 : 0;
            wprintf(L"%d ", ch);
        }
        wprintf(L"\n");
    }
#else
    for (y=0; y<VERSIZE(v); y+=2) {
        for (x=0; x<VERSIZE(v); x++) {
            ch = qrGetMod(v, x, y, (u_int8_t *)buffer) ? 1 : 0;
            if ((y+1)<VERSIZE(v)) {
                ch |= (qrGetMod(v, x, y+1, (u_int8_t *)buffer) ? 2 : 0);
            }
            wprintf(L"%lc", cp[ch]);
        }
        wprintf(L"\n");
    }
#endif

    return 0;
}
