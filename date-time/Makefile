CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

# for OpenBSD ports; other OS differ or will need suitable symlinks
TCL=tcl85

c-delta-time: c-delta-time.c
date2epoch: date2epoch.c
myat: myat.c
mycal: mycal.c
now: now.c

stardate: stardate.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs $(TCL)` stardate.c -o stardate

timeout: timeout.c

clean:
	git clean --force -x

test: date2epoch now stardate timeout
	@prove --nocolor

.PHONY: clean test
