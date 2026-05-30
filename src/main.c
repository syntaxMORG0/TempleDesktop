#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

int main(void) {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        10, 10,
        800, 600,
        0,
        BlackPixel(display, screen),
        0x0000FF);

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XStoreName(display, window, "TempleDesktop");
    XDefineCursor(display, window, XCreateFontCursor(display, XC_left_ptr));
    XMapWindow(display, window);

    for (;;) {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == KeyPress || event.type == DestroyNotify) {
            break;
        }
    }

    XCloseDisplay(display);
    return 0;
}