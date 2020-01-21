CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

chii: chii.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs gsl goptfoo` chii.c -o chii

gaussiate: gaussiate.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs gsl goptfoo` gaussiate.c -o gaussiate

clean:
	git clean --force -x

.PHONY: clean
