CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

bt: bt.c
charinfo: charinfo.c
copycat: copycat.c
justblocks: justblocks.c
muss-with-environ: muss-with-environ.c

pcre-precompile: pcre-precompile.c Makefile
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libpcre` pcre-precompile.c -o pcre-precompile

procrust: procrust.c Makefile
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` procrust.c -o procrust

setsid: setsid.c
sgrax: sgrax.c
solitary: solitary.c

threading: threading.c
	$(CC) $(CFLAGS) -lpthread threading.c -o threading

waitornot: waitornot.c

clean:
	git clean --force -x

test: bt copycat justblocks procrust sgrax waitornot unwrap
	make -C t
	@prove --nocolor

.PHONY: clean test
