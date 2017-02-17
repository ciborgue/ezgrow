#include <stdio.h>
#include <strings.h>
#include <assert.h>

#ifndef DEBUG
#define DEBUG
#endif
#ifndef USE_ASSERTS
#define USE_ASSERTS
#endif

#include "qrEnum.h"
#include "qrEncode.c"

void test_isQR(void) { /* {{{ */
    int i;
    printf("Testing isQR* functions ... ");
    for (i=0; i<256; i++) {
	/* isQRdigit */
	int t = isQRmode(numericMI, i);
	if (i >= '0' && i <= '9') {
	    if (t == 0) {
		printf("isQRdigit(%d) failed f-\n", i); abort(); }
	} else {
	    if (t != 0) {
		printf("isQRdigit(%d) failed f+\n", i); abort(); }
	}

	/* isQRalnum */
	t = isQRmode(alphanumericMI, i);
	if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z') ||
		i == ' ' || i == '$' || i == '%' || i == '*' ||
		i == '+' || i == '-' || i == '.' || i == '/' ||
		i == ':') {
	    if (t == 0) {
		printf("isQRalnum(%c) failed f- [%d]\n", i, t); abort(); }
	} else {
	    if (t != 0) {
		printf("isQRalnum(%c) failed f+\n", i); abort(); }
	}
    }
    printf("ok\n");
} /* }}} */
int main(void) { /* {{{ */
    test_isQR();

    return 0;
} /* }}} */
