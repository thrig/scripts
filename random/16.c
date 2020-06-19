/* 16 -- 16 bits of entropy */

#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

char output[20];

int main(void)
{
    int fd, ret, i, j;
    uint16_t number;

#ifdef __OpenBSD__
    if (pledge("rpath stdio", NULL) == -1)
        err(1, "pledge failed");
#endif

    if ((fd = open("/dev/urandom", O_RDONLY)) == -1)
        err(1, "open failed");

    ret = read(fd, &number, sizeof(uint16_t));
    if (ret != sizeof(uint16_t)) {
        if (ret == 0) {
            err(1, "eof on read");
        } else if (ret < 0) {
            err(1, "read failed");
        } else {
            errx(1, "unexpected read got %d want %lu", ret, sizeof(uint16_t));
        }
    }

    for (i = 0, j = 0; i < 16; i++, j++) {
        output[j] = (number >> i) & 1 ? '1' : '0';
        if (i % 4 == 3)
            output[++j] = ' ';
    }
    output[19] = '\n';

    ret = write(STDOUT_FILENO, output, 20);
    if (ret != 20) {
        if (ret < 0) {
            err(1, "write failed");
        } else {
            errx(1, "unexpected write: %d expect 20", ret);
        }
    }

    exit(EXIT_SUCCESS);
}
