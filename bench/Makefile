CFLAGS=-std=c99 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

lockalot: lockalot.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` lockalot.c -o lockalot

malloc: malloc.c
	$(CC) $(CFLAGS) -g `pkg-config --cflags --libs goptfoo` -lpthread malloc.c -o malloc

repcharcount: repcharcount.c
	$(CC) -O2 $(CFLAGS) repcharcount.c -o repcharcount

usemem: usemem.c
	$(CC) $(CFLAGS) -g `pkg-config --cflags --libs goptfoo jkiss` -lpthread  usemem.c -o usemem

clean:
	git clean --force -x

test: repcharcount
	@prove --nocolor

.PHONY: clean test
