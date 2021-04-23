// xembiggen - display text in biggest font possible (while assuming a
// fairly short maximum text length). for those awkward moments when
// someone asks you the coffee shop wifi password amid noise and
// mumbling over a mask. granted, this is a technological solution to
// what really is a social problem

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

Display *X_Dpy; // XOpenDisplay
int X_Snum;     // DefaultScreen
Window X_Win;   // main window

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
    // the font size... the font size cannot be larger than 999
    char bytes[3], fontname[] = "Utopia:rgba=vbgr:pixelsize=32\0\0";
    int count, width, height;
    size_t len;

#ifdef __OpenBSD__
    if (pledge("rpath stdio unix", NULL) == -1) err(1, "pledge failed");
#endif

    if (argc != 2) errx(1, "Usage: xembiggen text-label\n");
    len = strlen(argv[1]);

    x_init(argc, argv, "xembiggen", "Xmb", "Xembiggen", "Xembiggen", &width,
           &height);

    font = fontus_maximus(fontname, argv[1], len, width, height);

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
                              (const FcChar8 *) argv[1], len);
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
