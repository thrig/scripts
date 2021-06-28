#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
    int ch, prev = getchar();
    unsigned long count = 1;
    while ((ch = getchar()) != EOF) {
        if (ch != prev) {
            printf("%lu %c\n", count, prev);
            count = 0;
            prev  = ch;
        }
        count++;
    }
    printf("%lu %c\n", count, prev);
}
