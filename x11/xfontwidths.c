// xfontwidths - show the width of a given input phrase for all X11
// fonts. the BADFONT value indicates that the font failed to load (or
// that a semipredicate problem happened). note that fonts may have a
// width of 0

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define BADFONT -1

static void emit_font_width(Display *x_display, char *fontname, char *phrase,
                            size_t len);

int main(int argc, char *argv[]) {
    Display *x_display;
    char **fontnames;
    int maxnames, total;
    size_t len;

    if (argc != 2) {
        fputs("Usage: xfontwidths text-phrase\n", stderr);
        exit(EX_USAGE);
    }

    if (!(x_display = XOpenDisplay(NULL))) errx(1, "XOpenDisplay failed");

    // there can be lots of these
    maxnames = 4096;
    while (1) {
        if (!(fontnames = XListFonts(x_display, "*", maxnames, &total)))
            errx(1, "no fonts found??");
        if (total == maxnames) {
            // though probably not this many lots
            if (maxnames > INT_MAX / 2)
                errc(1, EOVERFLOW, "overflow on font names list??");
            maxnames *= 2;
            XFreeFontNames(fontnames);
            continue;
        }
        break;
    }
    fprintf(stderr, "total-fonts %d\n", total);

    // emit as much output as possible should this program or X crash --
    // there may be bad fonts (less so on OpenBSD, more so on Mac OS X)
    setvbuf(stdout, (char *) NULL, _IONBF, (size_t) 0);

    len = strlen(argv[1]);
    for (int i = 0; i < total; i++)
        emit_font_width(x_display, fontnames[i], argv[1], len);

    XFreeFontNames(fontnames);
    XCloseDisplay(x_display);
    exit(EXIT_SUCCESS);
}

inline static void emit_font_width(Display *x_display, char *fontname,
                                   char *phrase, size_t len) {
    XCharStruct xcs;
    XFontStruct *font;
    int dir, asc, dsc;

    int width = BADFONT;

    if ((font = XLoadQueryFont(x_display, fontname))) {
        XTextExtents(font, phrase, len, &dir, &asc, &dsc, &xcs);

        // same number as provided by XTextWidth(3), though I haven't
        // checked whether XTextWidth uses XTextExtents(3) itself, or
        // the less efficient XQueryTextExtents
        width = xcs.width;

        XFreeFont(x_display, font);
    }
    printf("%d %s\n", width, fontname);
}
