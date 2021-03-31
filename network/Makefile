CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

endian: endian.c
getsockname: getsockname.c
ip6tomac: ip6tomac.c
ipint: ipint.c
ipvalid: ipvalid.c
localaddr: localaddr.c

macnorm: macnorm.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` macnorm.c -o macnorm

# -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DSYSDIG
portpester: portpester.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` portpester.c -o portpester

macutil.o: macutil.c macutil.h
	$(CC) -c $(CFLAGS) macutil.c -o macutil.o

ow: ow.c
	`perl -MConfig -e 'print $$Config{cc}'` -O2 -Wall -fno-diagnostics-color -o ow ow.c `perl -MExtUtils::Embed -e ccopts -e ldopts`

randmac: randmac.c macutil.o macutil.h
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` macutil.o randmac.c -o randmac

v4addr: v4addr.c

v4in6addr: v4in6addr.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` v4in6addr.c -o v4in6addr

v6addr: v6addr.c

validate-rsnapshot: validate-rsnapshot.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libpcre` validate-rsnapshot.c -o validate-rsnapshot

clean:
	git clean --force -x

test: ipint localaddr macnorm ow v4addr v6addr validate-rsnapshot
	@prove --nocolor

.PHONY: clean test
