CFLAGS=-std=c99 -O2 -Wall -pedantic -fno-diagnostics-color -fstack-protector-strong -fPIC -fPIE -pie -pipe

dupenv: dupenv.c
	$(CC) $(CFLAGS) dupenv.c -o dupenv

nodupenv: nodupenv.c redblack-bst.h redblack-bst.o
	$(CC) $(CFLAGS) redblack-bst.o nodupenv.c -o nodupenv

redblack-bst.o: redblack-bst.c redblack-bst.h
	$(CC) $(CFLAGS) -c redblack-bst.c -o redblack-bst.o 

clean:
	git clean --force -x

test: dupenv nodupenv
	@prove --nocolor

.PHONY: clean test
