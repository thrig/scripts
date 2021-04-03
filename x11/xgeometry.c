// xgeometry -- just the window geometry. xwininfo(1) gunks that up with
// formatting, and while xdotool(1) can emit results suitable for a
// shell eval this either requires a shell and hoping that the eval does
// nothing naughty or otherwise it's the same problem as xwininfo(1)
// only a different parse

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

void emit_help(void);
Window parseid(char *, Display *, int);

int main(int argc, char *argv[]) {
    Display *disp;
    Window target;
    XWindowAttributes attr;
    int snum;

#ifdef __OpenBSD__
    if (pledge("rpath stdio unix", NULL) == -1) err(1, "pledge failed");
#endif

    if ((disp = XOpenDisplay(NULL)) == NULL)
        errx(EX_OSERR, "XOpenDisplay failed");
    snum = DefaultScreen(disp);

    target = RootWindow(disp, snum);
    if (argc == 2)
        target = parseid(*(argv + 1), disp, snum);
    else if (argc > 2)
        emit_help();

    XGetWindowAttributes(disp, target, &attr);
    printf("%dx%d%+d%+d\n", attr.width, attr.height, attr.x, attr.y);

    exit(EXIT_SUCCESS);
}

void emit_help(void) {
    fputs("Usage: xgeometry [window-id]\n", stderr);
    exit(EX_USAGE);
}

inline Window parseid(char *arg, Display *disp, int snum) {
    char *ep;
    unsigned long val;
    if (!arg || *arg == '\0') return RootWindow(disp, snum);
    while (isspace(*arg))
        arg++;
    if (*arg != '+' && !isdigit(*arg))
        errx(EX_DATAERR, "windowid must be positive");
    errno = 0;
    val   = strtoul(arg, &ep, 0);
    if (arg[0] == '\0' || *ep != '\0')
        errx(EX_DATAERR, "strtoul failed on '%s'", arg);
    if (errno == ERANGE && val == ULONG_MAX)
        errx(EX_DATAERR, "windowid is not an unsigned long");
    return val;
}
