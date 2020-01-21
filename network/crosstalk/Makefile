CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

crosstalkc: crosstalkc.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libevent` -lm -lpthread -o crosstalkc crosstalkc.c

# for sd_notify() on Linux add `-DSYSTEMD -lsystemd` (plus various other
# random requirements, because Linux)
crosstalkd: crosstalkd.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs libevent` -o crosstalkd crosstalkd.c

clean:
	git clean --force -x

.PHONY: clean
