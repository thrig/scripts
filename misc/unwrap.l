/* unwrap - unfold hard-wrapped text lines */

%option 8bit nodefault noinput nounput noyywrap
%x BODY
%{

#include <ctype.h>
#include <err.h>
#include <sysexits.h>
#include <unistd.h>

int Flag_Header; // -H

char fill = '\n';
int indent, nl = 0;

#define NEW_PP                                                                 \
    {                                                                          \
        putchar('\n');                                                         \
        fill = '\n';                                                           \
        nl   = 1;                                                              \
    }

%}
%%

[^\n]+ {
    for (indent = yyleng - 1; indent > 0; indent--)
        if (!isspace(yytext[indent])) break;
    if (indent > 0) {
        yytext[indent + 1] = '\0';
        fputs(yytext, stdout);
    }
    if (!Flag_Header) {
        fill = ' ';
        BEGIN BODY;
    }
}
\n\n   { putchar('\n'); nl = 1; BEGIN BODY; }
\n     { putchar('\n'); nl = 1; }

<BODY>[^\n]+ {
    char *start;
    if (indent) putchar(fill);
    start = yytext;
    /* consume leading whitespace on continuation lines */
    if (!nl) {
        while (*start != '\0') {
            if (isblank(*start)) {
                start++;
                continue;
            }
            break;
        }
    }
    if (*start != '\0') {
        for (indent = yyleng - 1; indent > 0; indent--)
            if (!isspace(yytext[indent])) break;
        if (indent > 0) {
            yytext[indent + 1] = '\0';
            fputs(start, stdout);
        }
        fill = ' ';
        nl   = 0;
    } else NEW_PP;
}
<BODY>\n\n+ NEW_PP;
<BODY>\n

<<EOF>> { if (!nl) putchar('\n'); yyterminate(); }

%%

int main(int argc, char *argv[]) {
    FILE *fh;
    int ch;

#ifdef __OpenBSD__
    if (pledge("rpath stdio", NULL) == -1) err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?H")) != -1) {
        switch (ch) {
        case 'H': Flag_Header = 1; break;
        case 'h':
        case '?':
        default:
            fputs("Usage: unwrap [-H] [file .. | -]\n", stderr);
            exit(EX_USAGE);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0 || strncmp(*argv, "-", (size_t) 2) == 0) {
        yylex();
    } else {
        while (*argv) {
            if ((fh = fopen(*argv, "r")) == NULL)
                err(EX_IOERR, "could not open '%s'", *argv);
            yyrestart(fh);
            yylex();
            fclose(fh);
            argv++;
        }
    }

    exit(EXIT_SUCCESS);
}
