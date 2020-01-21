CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

.SUFFIXES: .m4 .c
.m4.c:
	m4 ${.IMPSRC} > ${.TARGET}

todo: todo.c
todo.c: todo.m4

# cpanm assumes App::cpanminus is installed (and possibly local::lib too)
# only OpenBSD m4(1) has really been tested (portability is poor on m4)
depend:
	pkg-config --exists goptfoo
	pkg-config --exists gsl
	pkg-config --exists jkiss
	pkg-config --exists libsodium
	pkg-config --exists libpcre
	echo 'm4_m4exit(0)' | m4 -P
	perl -e 'use 5.24.0'
	cpanm --installdeps .
	expect -c 'package require Tcl 8.5; package require fileutil 1.13.0'

test: todo
	@prove --nocolor

clean:
	git clean --force -x

.PHONY: clean depend test
