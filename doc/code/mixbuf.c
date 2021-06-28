#include <stdio.h>
#include <unistd.h>
int main(void) {
    puts("duck");
    puts("duck");
    write(1, "goose\n", 6);
    return 0;
}
