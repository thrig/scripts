CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

16: 16.c
coinflip: coinflip.c
fair-from-unfair: fair-from-unfair.c

modulo-bias: modulo-bias.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo gsl` modulo-bias.c -o modulo-bias

oneof: oneof.c

n-of: n-of.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` n-of.c -o n-of

randline: randline.c

randnum: randnum.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs goptfoo` randnum.c -o randnum

randrand: randrand.c
	$(CC) $(CFLAGS) `pkg-config --cflags --libs gsl` randrand.c -o randrand

roll: roll.c

test: coinflip oneof n-of randline roll
	@prove --nocolor

clean:
	git clean --force -x

.PHONY: clean
