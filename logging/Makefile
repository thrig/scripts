CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

# maybe `pkg-config ... libevent`_core on Centos7 ?
# for sd_notify() add `-DSYSTEMD -lsystemd` and ensure OS devel foo
buggyservd: buggyservd.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libevent` -o buggyservd buggyservd.c

cts: cts.c
logsoc: logsoc.c

clean:
	git clean --force -x

.PHONY: clean
