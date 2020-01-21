CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

minmda: minmda.c minmda.h files.o Makefile
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libsodium` files.o minmda.c -o minmda

minmda-corrupts: minmda.c minmda.h files.o Makefile
	$(CC) $(CFLAGS) -DMINMDA_ALWAYS_CORRUPTS=1 `pkg-config --cflags --libs libsodium` files.o minmda.c -o minmda-corrupts

files.o: files.c
	$(CC) $(CFLAGS) -c files.c -o files.o

clean:
	git clean --force -x

test: minmda minmda-corrupts
	@prove --nocolor
	@-rm minmda-corrupts 2>/dev/null

.PHONY: clean test
