CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

blocksig: blocksig.c

jitterbits: jitterbits.c

obdurate: obdurate.c

sigint-on: sigint-on.c

slay-self: slay-self.c

test: blocksig
	@prove --nocolor

clean:
	git clean --force -x

.PHONY: clean test
