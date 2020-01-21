CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

# https://github.com/thrig/goptfoo

# for OpenBSD ports;
TCL=tcl85
# Mac OS X with MacPorts will instead need
#   make dnstw TCL=tcl
# or to create a link pkg-config can find
#   cd .../lib/pkgconfig
#   ln -s /opt/local/lib/pkgconfig/tcl.pc tcl86.pc 
#
# OpenBSD 6.2 also needs -pthread added to the $(CC) lines

dnstw: dnstw.c dnstw.h ipparse.o
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo $(TCL)` dnstw.c ipparse.o -o dnstw

ipparse.o: dnstw.h ipparse.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags --libs $(TCL)` ipparse.c -o ipparse.o

clean:
	git clean --force -x

# the Perl deps are for `make test`; dnstw itself should only need TCL
depend:
	pkg-config --exists goptfoo $(TCL)
	@expect -c "package require Tcl 8.5"

test: dnstw
	@prove --nocolor

.PHONY: clean depend test
