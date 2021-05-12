CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

bat: bat.c

depend:
	doas pkg_add pngcrush imlib2 xdotool

clean:
	git clean --force -x

.PHONY: clean depend
