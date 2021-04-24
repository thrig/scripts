//    xembiggen - display text using the biggest font size possible
// (assuming a fairly short maximum text length). for those awkward
// moments when someone asks you the coffee shop wifi password amid
// noise and mumbling over a mask. granted, this is a technological
// solution to what really is a social problem

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

char *Flag_Font; // -F

Display *X_Dpy; // XOpenDisplay
int X_Snum;     // DefaultScreen
Window X_Win;   // main window

void emit_help(void);
static char *fixup_fontname(const char *fontname);
static XftFont *fontus_maximus(char *fontname, char *text, size_t len,
                               int wwidth, int wheight);
static XftFont *getafont(const char *fontname);
static void x_destroy(void);
static void x_init(int argc, char *argv[], char *window_name, char *icon_name,
                   char *res_name, char *res_class, int *width, int *height);

int main(int argc, char *argv[]) {
    XEvent xev;
    XftColor fgcolor;
    XftDraw *draw;
    XftFont *font;
    // run fc-list(1) to see what fonts are available
    //
    // the \0\0 are to pad the string for the snprintf that fiddle with
    // the font size
    char bytes[3], *fontname = strdup("Utopia:pixelsize=32\0\0");
    int ch, count, width, height;
    size_t len;

#ifdef __OpenBSD__
    // wpath with no allowed writes is to help protect the process
    // from abortion should fontconfig desire to update the caches.
    // run fc-cache(1) now and then especially after updates to the
    // font files
    if (pledge("rpath stdio unix unveil wpath", NULL) == -1)
        err(1, "pledge failed");
    if (unveil("/", "r") == -1) err(1, "unveil failed");
    if (unveil(NULL, NULL) == -1) err(1, "unveil failed");
#endif

    while ((ch = getopt(argc, argv, "h?F:")) != -1) {
        switch (ch) {
        case 'F': Flag_Font = optarg; break;
        default: emit_help();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) emit_help();

    if (Flag_Font) fontname = fixup_fontname(Flag_Font);

    x_init(argc, argv, "xembiggen", "Xmb", "Xembiggen", "Xembiggen", &width,
           &height);

    len  = strlen(argv[0]);
    font = fontus_maximus(fontname, argv[0], len, width, height);

    XMapWindow(X_Dpy, X_Win);
    draw = XftDrawCreate(X_Dpy, X_Win, DefaultVisual(X_Dpy, X_Snum),
                         DefaultColormap(X_Dpy, X_Snum));
    if (!XftColorAllocName(X_Dpy, DefaultVisual(X_Dpy, X_Snum),
                           DefaultColormap(X_Dpy, X_Snum), "black", &fgcolor))
        errx(1, "XftColorAllocName failed");

    while (1) {
        XNextEvent(X_Dpy, &xev);
        switch (xev.type) {
        case Expose:
            if (xev.xexpose.count != 0) break;
            // TODO ideally minimize changes instead of entire string redraw
            XftDrawStringUtf8(draw, &fgcolor, font, 0, font->ascent,
                              (const FcChar8 *) argv[0], len);
            break;
        case KeyPress:
            count = XLookupString(&xev.xkey, bytes, sizeof(bytes), NULL, NULL);
            if (count == 1 && bytes[0] == '\033') goto CLEANUP;
        }
    }

CLEANUP:
    XftColorFree(X_Dpy, DefaultVisual(X_Dpy, X_Snum),
                 DefaultColormap(X_Dpy, X_Snum), &fgcolor);
    XftDrawDestroy(draw);
    x_destroy();
    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: xembiggen [-F font] text-label\n", stderr);
    exit(EX_USAGE);
}

static char *fixup_fontname(const char *fontname) {
    char *ep, *fp = NULL, *input, *output, *np;
    input = strdup(fontname);
    size_t len;

    while (fp == NULL) {
        fp = strstr(input, ":pixelsize=");
        if (fp != NULL) {
            ep = fp + 11;
            // any following :... attributes?
            np = strchr(ep, ':');
            if (np != NULL) {
                // copy remainder over the pixelsize entry
                len = strlen(np);
                memmove(fp, np, len);
                *(fp + len) = '\0';
                // maybe more pixelsize remain?
                fp = NULL;
            } else {
                // strip tail position pixelsize
                *fp = '\0';
            }
        } else {
            break;
        }
    }

    len = strlen(input);
    if (len > SIZE_MAX - 15) errc(1, EOVERFLOW, "overflow");
    if ((output = calloc(1, len + 15)) == NULL) err(1, "calloc failed");
    strcat(output, input);
    strcat(output + len, ":pixelsize=32");
    free(input);
    return output;
}

// measure twice, cut once - find the maximum font size (within limits)
// that can be used to display the given single line of text within the
// given window dimensions (with various assumptions that may not hold)
//
// NOTE this (or a bounding rectangle) may need some padding should
// super- or sub-scripts wander outside the usual bounding boxes and if
// that is a problem. some fonts also run pretty close to the edges
//
// NOTE very long lines of text are not wrapped. they should be. but
// that would complicate matters
inline static XftFont *fontus_maximus(char *fontname, char *text, size_t len,
                                      int wwidth, int wheight) {
    XGlyphInfo extents;
    XftFont *font;
    char *fp;
    int fwidthA, fheightA, fwidthB, fheightB, fontsize;
    double slope, intercept;

    fp = strstr(fontname, ":pixelsize=");
    if (fp == NULL) errx(1, "no pixelsize= in font name??");
    fp += 11;

    font = getafont(fontname);
    XftTextExtentsUtf8(X_Dpy, font, (const FcChar8 *) text, len, &extents);
    fwidthA  = extents.xOff;
    fheightA = font->height + 1;

    snprintf(fp, 4, "%d", 64);
    font = getafont(fontname);
    XftTextExtentsUtf8(X_Dpy, font, (const FcChar8 *) text, len, &extents);
    fwidthB  = extents.xOff;
    fheightB = font->height + 1;

    // now some slope equation and ratio work
    slope     = ((double) fheightB - fheightA) / (fwidthB - fwidthA);
    intercept = fheightB - slope * fwidthB;

    double x, y;
    x = (wheight - intercept) / slope;
    y = slope * wwidth + intercept;

    // assume that a/b = c/d proportion holds for fontsize/dimension
    // ratio; what dimension to use will vary though most likely the
    // text will be wider than it is taller
    if (x > wwidth)
        fontsize = 64 * y / fheightB;
    else
        fontsize = 64 * x / fwidthB;

    // TODO maybe want a warning when clipping happens?
    if (fontsize > 999)
        fontsize = 999;
    else if (fontsize < 12)
        fontsize = 12;

    snprintf(fp, 4, "%d", fontsize);
    font = getafont(fontname);
    return font;
}

inline static XftFont *getafont(const char *fontname) {
    XftFont *font;
    font = XftFontOpenXlfd(X_Dpy, X_Snum, fontname);
    if (font == NULL) {
        font = XftFontOpenName(X_Dpy, X_Snum, fontname);
        if (font == NULL) errx(1, "XftFontOpenName failed '%s'", fontname);
    }
    return font;
}

inline static void x_destroy(void) {
    XUnmapWindow(X_Dpy, X_Win);
    XDestroyWindow(X_Dpy, X_Win);
    XCloseDisplay(X_Dpy);
}

inline static void x_init(int argc, char *argv[], char *window_name,
                          char *icon_name, char *res_name, char *res_class,
                          int *width, int *height) {
    XClassHint *class;
    XGCValues gcvals;
    XSetWindowAttributes attr;
    XSizeHints wmsize;
    XTextProperty windowName, iconName;
    XWMHints wmhints;
    unsigned long mask;

    if (!(X_Dpy = XOpenDisplay(NULL))) errx(EX_OSERR, "XOpenDisplay failed");

    X_Snum = DefaultScreen(X_Dpy);

    attr.background_pixel = WhitePixel(X_Dpy, X_Snum);
    attr.border_pixel     = BlackPixel(X_Dpy, X_Snum);
    attr.event_mask       = ExposureMask | KeyPressMask;
    mask                  = CWBackPixel | CWBorderPixel | CWEventMask;

    XWindowAttributes rootattr;
    XGetWindowAttributes(X_Dpy, RootWindow(X_Dpy, X_Snum), &rootattr);
    *width  = rootattr.width;
    *height = rootattr.height;

    X_Win = XCreateWindow(X_Dpy, RootWindow(X_Dpy, X_Snum), rootattr.x,
                          rootattr.y, rootattr.width, rootattr.height, 0,
                          DefaultDepth(X_Dpy, X_Snum), InputOutput,
                          DefaultVisual(X_Dpy, X_Snum), mask, &attr);

    wmsize.flags          = USPosition | USSize;
    wmhints.initial_state = NormalState;
    wmhints.flags         = StateHint;
    XStringListToTextProperty(&window_name, 1, &windowName);
    XStringListToTextProperty(&icon_name, 1, &iconName);
    if (!(class = XAllocClassHint())) err(1, "XAllocClassHint failed");
    class->res_name  = res_name;
    class->res_class = res_class;

    XSetWMProperties(X_Dpy, X_Win, &windowName, &iconName, argv, argc, &wmsize,
                     &wmhints, class);
    XFree(class);

    gcvals.background = WhitePixel(X_Dpy, X_Snum);
    gcvals.foreground = BlackPixel(X_Dpy, X_Snum);
    mask              = GCForeground | GCBackground;
}
