CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

.SUFFIXES: .m4 .c
.m4.c:
	m4 ${.IMPSRC} > ${.TARGET}

corrupt: corrupt.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` corrupt.c -o corrupt

fbd: fbd.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` fbd.c -o fbd

fdsplit: fdsplit.c
fillerup: fillerup.c
findin: findin.c
findup: findup.c
getpof: getpof.c
is-mounted: is-mounted.c
llcount: llcount.c
log-update: log-update.c

nomfd: nomfd.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` nomfd.c -o nomfd

pathof: pathof.c
permcheck: permcheck.c

seek: seek.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` seek.c -o seek

stripeofeol: stripeofeol.c

thisdir: thisdir.c
thisdir.c: thisdir.m4

twiddle: twiddle.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` twiddle.c -o twiddle

unatomappend: unatomappend.c

uvi: uvi.c

clean:
	git clean --force -x

test: fbd fdsplit findin findup getpof is-mounted llcount pamei seek thisdir twiddle uvi
	@prove --nocolor

.PHONY: clean test
