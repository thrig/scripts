// pollute - create all possible single byte filenames, presumably for
// testing; '.', '/', and '\0' are problematic for various reasons

#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    int ch, exit_status, fd;
    char name[2];
    struct timeval when[2];

#ifdef __OpenBSD__
    if (pledge("cpath fattr rpath stdio wpath unveil", NULL) == -1)
        err(1, "pledge failed");
    if (unveil(".", "crw") == -1) err(1, "unveil failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    exit_status = EXIT_SUCCESS;
    name[1]     = '\0';
    if (gettimeofday(when, NULL) == -1) err(1, "gettimeofday failed");
    when[1] = when[0];

    for (ch = 1; ch < 256; ch++) {
        if (ch == '/' || ch == '.') continue;
        name[0] = ch;
        fd      = open(name, O_CREAT | O_NOFOLLOW | O_WRONLY, 0666);
        if (fd < 0) {
            warn("open failed '%c'", ch);
            exit_status = 1;
        } else {
            utimes(name, when);
            close(fd);
        }
    }

    exit(exit_status);
}
