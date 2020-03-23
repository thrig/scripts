/* macnorm - normalizes MAC addresses */

#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

// https://github.com/thrig/goptfoo
#include <goptfoo.h>

#define NOTHING 0

enum { A_NUMBER, A_SEPARATOR };

int Flag_Octets = 6;            /* -O */
char *Flag_Prefix;              /* -p */
int Flag_Quiet;                 /* -q */
int Flag_Sep = ':';             /* -s */
int Flag_TrailingGarbage;       /* -T */
char *Flag_OctetFormat = "%02x";        /* -X */

void emit_error(const char *input, int offset, char *msg);
void emit_help(void);
void normalize_mac(const char *input, int octets, char *out, int ofsep);

int main(int argc, char *argv[])
{
    char *buf;
    int ch;

#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1)
        err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?O:p:qs:TxX")) != -1) {
        switch (ch) {
        case 'O':
            Flag_Octets = (int) flagtoul(ch, optarg, 1UL, 100UL);
            break;
        case 'p':
            Flag_Prefix = optarg;
            break;
        case 'q':
            Flag_Quiet = 1;
            break;
        case 's':
            Flag_Sep = *optarg;
            break;
        case 'T':
            Flag_TrailingGarbage = 1;
            break;
        case 'x':
            Flag_OctetFormat = "%02x";
            break;
        case 'X':
            Flag_OctetFormat = "%02X";
            break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    if (argc < 1)
        emit_help();
    argv += optind;

    if ((buf = malloc(sizeof(char) * Flag_Octets * 3 + 1)) == NULL)
        err(EX_OSERR, "malloc failed");

    while (*argv) {
        normalize_mac(*argv, Flag_Octets, buf, Flag_Sep);
        if (Flag_Prefix)
            printf("%s", Flag_Prefix);
        printf("%s", buf);
        argv++;
    }

    exit(EXIT_SUCCESS);
}

void emit_error(const char *input, int offset, char *msg)
{
    if (!Flag_Quiet) {
        if (*input != '\0')
            fprintf(stderr, "  %s\n  %*c\n", input, offset + 1, '^');
        errx(EX_DATAERR, "%s", msg);
    } else {
        exit(EX_DATAERR);
    }
}

void emit_help(void)
{
    fputs("Usage: macnorm [-qTxX] [-p prefix] [-s sep] macaddr [..]\n", stderr);
    exit(EX_USAGE);
}

void normalize_mac(const char *input, int octets, char *out, int ofsep)
{
    char *ip;
    int expect = A_NUMBER;
    int offset;
    int onum = 1;
    int sep = -1;
    unsigned int value;

    setlocale(LC_ALL, "C");

    if (*input == '\0')
        emit_error(input, 0, "cannot parse empty string");
    /* disallow leading garbage. also prevents negative hex numbers from
     * being supplied to sscanf */
    if (!isxdigit(*input))
        emit_error(input, 0, "string must begin with xdigit");

    ip = (char *) input;

    while (1) {
        if (expect == A_NUMBER) {
            if (sscanf(ip, "%2X%n", &value, &offset) != 1)
                emit_error(input, ip - input, "did not match an xdigit");
            ip += offset;
            snprintf(out, 3, Flag_OctetFormat, value);
            out += 2;
            expect = A_SEPARATOR;
        } else {
            if (onum >= octets)
                break;
            if (sep > 0) {
                if (*ip == sep)
                    ip++;
                else
                    emit_error(input, ip - input,
                               "expected separator not found");
            } else if (sep == NOTHING) {
                ;
            } else {
                /* allow for most any separator (including nothing)
                 * provided that it is used consistently */
                if (*ip == '\0') {
                    emit_error(input, ip - input,
                               "NUL may not be used as separator");
                } else if (isxdigit(*ip)) {
                    sep = NOTHING;
                } else {
                    sep = *ip;
                    ip++;
                }
            }
            if (ofsep > 0) {
                *out = ofsep;
                out++;
            }
            expect = A_NUMBER;
            onum++;
        }
    }
    if (!Flag_TrailingGarbage && *ip != '\0')
        emit_error(input, ip - input, "trailing garbage");
    *out++ = '\n';
    *out = '\0';
}
