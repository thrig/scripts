CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

# probably not portable but works(TM) for me on OpenBS 6.8 and Mac OS X
# 10.11 with MacPorts
CFLAGS+=`pkg-config --cflags --libs x11`

xtcidu: xtcidu.c

clean:
	git clean --force -x

test: xtcidu
	@prove --nocolor

.PHONY: clean test
