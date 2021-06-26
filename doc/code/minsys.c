#include <stdlib.h>
#include <string.h>
int main(int argc, char *argv[]) {
    char *ap;
    argv++;
    argc -= 2;
    ap = *argv;
    while (argc--) {
        ap  = strchr(ap, '\0');
        *ap = ' ';
    }
    system(*argv);
}
