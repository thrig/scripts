%option 8bit nodefault noinput nounput noyywrap

%%

\x07    { printf("\\a"); }
\x08    { printf("\\b"); }
\x09    { printf("\\t"); }
\x0a    { putchar('\n'); }
\x0d    { printf("\\r"); }
\x1b    { printf("\\e"); }
[[:print:]]+ { fputs(yytext, stdout); }
.       { printf("\\x%02X", yytext[0]); }

%%

#include <err.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    FILE *fh    = NULL;
    char *fname = "stdin";
    struct timespec zzz = { .tv_sec = 0, .tv_nsec = 25000000 };

#ifdef __OpenBSD__
    if (pledge("rpath stdio", NULL) == -1) err(1, "pledge failed");
#endif

    if (argc > 2) {
        fputs("Usage: termsplain [filename|-]\n", stderr);
        exit(EX_USAGE);
    } else if (argc == 2 && !(strncmp(argv[1], "-", (size_t) 2) == 0)) {
        if ((fh = fopen(argv[1], "r")) == NULL)
            err(EX_IOERR, "could not open '%s'", argv[1]);
        fname = argv[1];
        yyrestart(fh);
    }
    if (fh == NULL) fh = stdin;

    setvbuf(stdout, (char *) NULL, _IONBF, (size_t) 0);

    while (1) {
        yylex();
        if (ferror(fh)) err(EX_IOERR, "ferror on %s", fname);
        nanosleep(&zzz, NULL);
        fseek(fh, 0, SEEK_CUR);
    }

    exit(1);
}
