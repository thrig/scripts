/* xtcidu - reads a short ASCII label using an X window and emits said
 * label to stdout. this has probably already been written by someone
 * else, but it is good xlib practice */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <err.h>
#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

enum { HKEY_DONE, HKEY_MORE };

#ifndef OUTPUT_BUFFER_LEN
#define OUTPUT_BUFFER_LEN 64
#endif

#define TEXT_X 5
#define TEXT_Y 15

#define DINGBELL XBell(X_Disp, 50)

void emit_help(void);
int handle_key(XEvent *);
void x_destroy(void);
void x_init(int, char **, char *, char *, char *, char *);

Display *X_Disp; // XOpenDisplay
int X_Snum;      // DefaultScreen
Window X_Win;    // main window
GC X_Gc;

char Output[OUTPUT_BUFFER_LEN];

char *Flag_Badchars; // -B
int Flag_Nonewline;  // -n

int main(int argc, char *argv[]) {
    XEvent xev;
    int ch, status = EXIT_SUCCESS;

#ifdef __OpenBSD__
    if (pledge("rpath stdio unix", NULL) == -1) err(1, "pledge failed");
#endif

    x_init(argc, argv, "xtcidu", "Xt", "Xtcidu", "Xtcidu");

    while ((ch = getopt(argc, argv, "h?B:n")) != -1) {
        switch (ch) {
        case 'B': Flag_Badchars = optarg; break;
        case 'n': Flag_Nonewline = 1; break;
        case 'h':
        case '?':
        default:
            emit_help();
            /* NOTREACHED */
        }
    }
    argc -= optind;
    argv += optind;

    while (1) {
        XNextEvent(X_Disp, &xev);
        switch (xev.type) {
        case Expose:
            if (xev.xexpose.count != 0) break;
            if (Output[0] == '\0')
                XDrawString(X_Disp, X_Win, X_Gc, TEXT_X, TEXT_Y, "label:", 6);
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
        fputs(Output, stdout);
        if (!Flag_Nonewline) putchar('\n');
    } else {
        // did not get a string?? report this to caller
        status = 1;
    }
    exit(status);
}

void emit_help(void) {
    fputs("Usage: xtcidu [-B badchars] [-n]\n", stderr);
    exit(EX_USAGE);
}

inline int handle_key(XEvent *xevp) {
    char bytes[3], *bcp;
    int count, isbad;

    static unsigned long current = 0;
    unsigned long oldcur         = current;

    count = XLookupString(&xevp->xkey, bytes, sizeof(bytes), NULL, NULL);
    switch (count) {
    case 0: // control character
        break;
    case 1: // printable character -- ascii(7)
        switch (bytes[0]) {
        case 8: // backspace
            if (current == 0) {
                DINGBELL;
                break;
            }
            Output[--current] = '\0';
            break;
        case 13: // enter -- accepts the label, if there is one
            if (current == 0) break;
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
            if (current >= OUTPUT_BUFFER_LEN) {
                DINGBELL;
                break;
            }
            Output[current++] = bytes[0];
        }
        break;
    }

    if (current != oldcur) {
        XClearWindow(X_Disp, X_Win);
        XDrawString(X_Disp, X_Win, X_Gc, TEXT_X, TEXT_Y, Output, current);
    }

    return HKEY_MORE;
}

inline void x_destroy(void) {
    XUnmapWindow(X_Disp, X_Win);
    XDestroyWindow(X_Disp, X_Win);
    XCloseDisplay(X_Disp);
}

inline void x_init(int argc, char *argv[], char *window_name, char *icon_name,
                   char *res_name, char *res_class) {
    XClassHint *class;
    XGCValues gcvals;
    XSetWindowAttributes attr;
    XSizeHints wmsize;
    XTextProperty windowName, iconName;
    XWMHints wmhints;
    unsigned long mask;

    if (!(X_Disp = XOpenDisplay(NULL))) errx(EX_OSERR, "XOpenDisplay failed");

    X_Snum = DefaultScreen(X_Disp);

    attr.background_pixel = WhitePixel(X_Disp, X_Snum);
    attr.border_pixel     = BlackPixel(X_Disp, X_Snum);
    attr.event_mask       = ExposureMask | KeyPressMask;
    mask                  = CWBackPixel | CWBorderPixel | CWEventMask;

    // TODO at least support -geometry and see if can set minimum width
    // based off of max size of output buffer/font size
    X_Win = XCreateWindow(X_Disp, RootWindow(X_Disp, X_Snum), 300, 300, 256, 64,
                          2, DefaultDepth(X_Disp, X_Snum), InputOutput,
                          DefaultVisual(X_Disp, X_Snum), mask, &attr);

    wmsize.flags          = USPosition | USSize;
    wmhints.initial_state = NormalState;
    wmhints.flags         = StateHint;
    XStringListToTextProperty(&window_name, 1, &windowName);
    XStringListToTextProperty(&icon_name, 1, &iconName);
    if (!(class = XAllocClassHint())) err(1, "XAllocClassHint failed");
    class->res_name  = res_name;
    class->res_class = res_class;

    XSetWMProperties(X_Disp, X_Win, &windowName, &iconName, argv, argc, &wmsize,
                     &wmhints, class);
    // XFree(class);

    gcvals.background = WhitePixel(X_Disp, X_Snum);
    gcvals.foreground = BlackPixel(X_Disp, X_Snum);
    mask              = GCForeground | GCBackground;

    X_Gc = XCreateGC(X_Disp, X_Win, mask, &gcvals);

    XMapWindow(X_Disp, X_Win);
}
