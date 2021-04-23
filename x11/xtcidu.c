/* xtcidu - reads a short ASCII label using an X window and emits said
 * label to stdout. this has probably already been written by someone
 * else, but it is good xlib practice */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <ctype.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

enum { HKEY_DONE, HKEY_MORE };

#ifndef OUTPUT_BUFFER_LEN
#define OUTPUT_BUFFER_LEN 63
#endif

#define DINGBELL XBell(X_Disp, 50)

void emit_help(void);
int handle_key(XEvent *);
unsigned long x_color_pixel(char *);
unsigned long x_bg(void);
unsigned long x_fg(void);
void x_destroy(void);
void x_font(XFontStruct **, int *, int *);
void x_init(char *, char *, char *, char *);

Display *X_Disp; // XOpenDisplay
int X_Snum;      // DefaultScreen
Window X_Win;    // main window
GC X_Gc;         // "    "      graphics context

unsigned long Current;
char Output[OUTPUT_BUFFER_LEN + 1];

int Text_X = 5;
int Text_Y = 5;

char *Flag_Badchars; // -B
char *Flag_Bg;       // -b
char *Flag_Fg;       // -f
char *Flag_Font;     // -F
int Flag_Nonewline;  // -n

int main(int argc, char *argv[]) {
    XEvent xev;
    int ch, status = EXIT_SUCCESS;

#ifdef __OpenBSD__
    if (pledge("rpath stdio unix", NULL) == -1) err(1, "pledge failed");
#endif

    while ((ch = getopt(argc, argv, "h?B:F:b:f:n")) != -1) {
        switch (ch) {
        case 'B': Flag_Badchars = optarg; break;
        case 'F': Flag_Font = optarg; break;
        case 'b': Flag_Bg = optarg; break;
        case 'f': Flag_Fg = optarg; break;
        case 'n': Flag_Nonewline = 1; break;
        case 'h':
        case '?':
        default: emit_help();
        }
    }
    argc -= optind;
    argv += optind;

    // both MacPorts and OpenBSD have this font:
    //   xlsfonts | sort > a
    //   ...
    //   comm -12 a b
    if (!Flag_Font) Flag_Font = "lucidasanstypewriter-bold-24";

    x_init("xtcidu", "Xt", "Xtcidu", "Xtcidu");

    while (1) {
        XNextEvent(X_Disp, &xev);
        switch (xev.type) {
        case Expose:
            if (xev.xexpose.count != 0) break;
            if (Output[0] == '\0')
                XDrawString(X_Disp, X_Win, X_Gc, Text_X, Text_Y, "label:", 6);
            break;
        case KeyPress:
            if (handle_key(&xev) == HKEY_DONE) goto DONE;
            break;
        }
    }

DONE:
    x_destroy();

#ifdef __OpenBSD__
    if (pledge("stdio", NULL) == -1) err(1, "pledge failed");
#endif

    if (Output[0] != '\0') {
        if (!Flag_Nonewline) Output[Current++] = '\n';
        write(STDOUT_FILENO, Output, Current);
    } else {
        // did not get a string?? report this to caller
        status = 1;
    }

    exit(status);
}

void emit_help(void) {
    fputs("Usage: xtcidu [-B badchars] [-F font] [-b bg] [-f fg] [-n]\n",
          stderr);
    exit(EX_USAGE);
}

inline int handle_key(XEvent *xevp) {
    char bytes[3], *bcp;
    int count, isbad;
    unsigned long oldcur = Current;

    count = XLookupString(&xevp->xkey, bytes, sizeof(bytes), NULL, NULL);
    if (count == 1) { // printable character -- ascii(7)
        switch (bytes[0]) {
        case 8: // backspace
            if (Current == 0) {
                DINGBELL;
                break;
            }
            Output[--Current] = '\0';
            break;
        case 13: // enter -- accepts the label, if there is one
            if (Current == 0) {
                DINGBELL;
                break;
            }
            return HKEY_DONE;
        default:
            if (!isprint(bytes[0])) break;
            // so `-B /` can prevent '/' from appearing in a label that
            // is then used as a filename (the precise need this script
            // is aimed at)
            if (Flag_Badchars) {
                bcp   = Flag_Badchars;
                isbad = 0;
                while (*bcp != '\0') {
                    if (bytes[0] == *bcp) {
                        DINGBELL;
                        isbad = 1;
                        break;
                    }
                    bcp++;
                }
                if (isbad) break;
            }
            if (Current >= OUTPUT_BUFFER_LEN) {
                DINGBELL;
                break;
            }
            Output[Current++] = bytes[0];
        }
    }

    if (Current != oldcur) {
        XClearWindow(X_Disp, X_Win);
        XDrawString(X_Disp, X_Win, X_Gc, Text_X, Text_Y, Output, Current);
    }

    return HKEY_MORE;
}

inline unsigned long x_color_pixel(char *name) {
    XColor exact, closest;
    XAllocNamedColor(X_Disp, XDefaultColormap(X_Disp, X_Snum), name, &exact,
                     &closest);
    return closest.pixel;
}

inline unsigned long x_bg(void) {
    return Flag_Bg ? x_color_pixel(Flag_Bg) : WhitePixel(X_Disp, X_Snum);
}

inline unsigned long x_fg(void) {
    return Flag_Fg ? x_color_pixel(Flag_Fg) : BlackPixel(X_Disp, X_Snum);
}

inline void x_destroy(void) {
    XUnmapWindow(X_Disp, X_Win);
    XDestroyWindow(X_Disp, X_Win);
    XCloseDisplay(X_Disp);
}

inline void x_font(XFontStruct **font, int *width, int *height) {
    XCharStruct xcs;
    int dir, asc, dsc;

    if (!(*font = XLoadQueryFont(X_Disp, Flag_Font)))
        errx(1, "XLoadQueryFont failed for '%s'", Flag_Font);

    // "Q" as it goes up pretty high and has at least something of a
    // dangly bit below it and is a single character for the width
    // multiplication which is not perfect but hopefully is good enough
    XTextExtents(*font, "Q", 1, &dir, &asc, &dsc, &xcs);

    if (xcs.width < 1) errx(1, "invalid no-width font '%s'", Flag_Font);

    // TODO may need to limit the width (or the buffer, or more likely
    // use a smaller font size) if this turns out to be too wide. or
    // support -geometry and leave that and the font up to the caller...
    *width  = xcs.width * OUTPUT_BUFFER_LEN;
    *height = xcs.ascent + xcs.descent;
}

inline void x_init(char *window_name, char *icon_name, char *res_name,
                   char *res_class) {
    XClassHint *class;
    XFontStruct *font;
    XGCValues gcvals;
    XSetWindowAttributes attr;
    XSizeHints wmsize;
    XTextProperty windowName, iconName;
    XWMHints wmhints;
    int est_width, est_height;
    unsigned long fg, bg, mask;

    if (!(X_Disp = XOpenDisplay(NULL))) errx(EX_OSERR, "XOpenDisplay failed");
    X_Snum = DefaultScreen(X_Disp);

    bg                    = x_bg();
    fg                    = x_fg();
    attr.background_pixel = bg;
    attr.border_pixel     = BlackPixel(X_Disp, X_Snum);
    attr.event_mask       = ExposureMask | KeyPressMask;
    mask                  = CWBackPixel | CWBorderPixel | CWEventMask;

    x_font(&font, &est_width, &est_height);
    Text_Y += est_height;

    X_Win =
        XCreateWindow(X_Disp, RootWindow(X_Disp, X_Snum), 0, 0, est_width,
                      est_height * 2.6, 1, DefaultDepth(X_Disp, X_Snum),
                      InputOutput, DefaultVisual(X_Disp, X_Snum), mask, &attr);

    wmsize.flags          = USPosition | USSize;
    wmhints.initial_state = NormalState;
    wmhints.flags         = StateHint;
    XStringListToTextProperty(&window_name, 1, &windowName);
    XStringListToTextProperty(&icon_name, 1, &iconName);
    if (!(class = XAllocClassHint())) err(1, "XAllocClassHint failed");
    class->res_name  = res_name;
    class->res_class = res_class;

    XSetWMProperties(X_Disp, X_Win, &windowName, &iconName, NULL, 0, &wmsize,
                     &wmhints, class);
    // XFree(class);

    gcvals.background = WhitePixel(X_Disp, X_Snum);
    gcvals.foreground = BlackPixel(X_Disp, X_Snum);
    mask              = GCForeground | GCBackground;

    X_Gc = XCreateGC(X_Disp, X_Win, mask, &gcvals);
    XSetForeground(X_Disp, X_Gc, fg);
    XSetBackground(X_Disp, X_Gc, bg);

    XSetFont(X_Disp, X_Gc, font->fid);

    XMapWindow(X_Disp, X_Win);
}
