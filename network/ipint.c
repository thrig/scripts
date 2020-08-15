/* ipint - convert IPv4 address to 32-bit number, or the other way */

#include <arpa/inet.h>
#include <sys/socket.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

// so that 0... numbers are not taken as octal nor hex by strtoul.
// others may desire such shennanigans
#define BASE10 10

char buf[INET_ADDRSTRLEN];

void emit_usage(void);

int main(int argc, char *argv[]) {
    char *ep;
    int ch;
    struct in_addr addr;
    unsigned long val;

#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?")) != -1) {
        switch (ch) {
        case 'h':
        case '?':
        default:
            emit_usage();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1 || **argv == '\0') emit_usage();

    // assume number, convert presentation
    if (strchr(*argv, '.') == NULL) {
        // do not allow strtoul to parse negative numbers as those get huge
        while (isspace(**argv))
            (*argv)++;
        if (**argv != '+' && !isdigit(**argv))
            errx(EX_DATAERR, "need a positive integer");

        errno = 0;

        val = strtoul(*argv, &ep, BASE10);
        if (**argv == '\0' || *ep != '\0') errx(EX_DATAERR, "strtoul failed");
        if (val > UINT32_MAX) errx(EX_DATAERR, "need 32-bit value");

        addr.s_addr = htonl(val);
        if (inet_ntop(AF_INET, &addr, (char *) &buf, sizeof(buf))) {
            puts(buf);
        } else {
            err(1, "inet_ntop failed");
        }
    } else {
        // assume IPv4 presentation, convert to number
        int ret = inet_pton(AF_INET, *argv, &addr);
        if (ret == 1) {
            printf("%u\n", ntohl(addr.s_addr));
        } else if (ret == 0) {
            errx(EX_DATAERR, "could not parse IPv4 address '%s'", *argv);
        } else {
            err(EX_DATAERR, "could not parse IPv4 address '%s'", *argv);
        }
    }
    exit(EXIT_SUCCESS);
}

void emit_usage(void) {
    fputs("Usage: ipint positive-integer-or-ipv4-address\n", stderr);
    exit(EX_USAGE);
}
