CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

epochal: epochal.c

clean:
	git clean --force -x

test: epochal
	@prove --nocolor

.PHONY: clean test
