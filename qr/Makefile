vpath=%.c src
vpath=%.h include
vpath=%.o obj

SRCS := $(wildcard src/qr*.c)
TEST_SRCS := $(wildcard t/test_*.c)
OBJS := $(addprefix obj/,$(notdir $(SRCS:.c=.o))) obj/QR.o

CFLAGS = -DUSE_ASSERTS -O0 -g -DDEBUG -ansi -Wall -pedantic -Iinclude
#CFLAGS = -DUSE_ASSERTS -Os -march=native -DDEBUG -ansi -Wall -pedantic -Iinclude

.PHONY: clean

clean:
	rm -rf UNICODE PBM .depend obj/* lib/* tests/* log/*

objs:	$(TEST_OBJS) $(OBJS)

tests: $(addprefix tests/,$(notdir $(subst test_,,$(TEST_SRCS:.c=))))

tests/qrGaloisField:		obj/qrFIFO.o obj/qrBlocks.o
tests/qrFIFO:		obj/qrFIFO.o
tests/qrIsQR:		obj/qrFIFO.o
tests/qrSpecial:		obj/qrBitMagic.o obj/qrFIFO.o
tests/qrXOR:		obj/qrBitMagic.o obj/qrFIFO.o
tests/qrB32:		obj/qrBitMagic.o obj/qrFIFO.o obj/qrBlocks.o obj/qrGaloisField.o obj/qrShuffle.o obj/qrEncode.o obj/qrMatrix.o
tests/qrSequence:		obj/qrMatrix.o obj/qrBitMagic.o obj/qrFIFO.o
tests/qrShuffle:		obj/qrBlocks.o
tests/qrBitMagic:		obj/qrBitMagic.o
tests/qrEncode1:		obj/qrBlocks.o obj/qrGaloisField.o obj/qrShuffle.o obj/qrMatrix.o obj/qrBitMagic.o
PBM:	$(OBJS)
	$(CC)	-o $@ $(CFLAGS) -std=c99 -DPBM UNICODE.c $(OBJS)
UNICODE:	$(OBJS)
ASCII:	$(OBJS)
	$(CC)	-o $@ $(CFLAGS) -std=c99 ASCII.c $(OBJS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%_t.o: t/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

tests/%: t/test_%.c
	$(CC) $(CFLAGS) -Isrc -o $@ $^
	$@
