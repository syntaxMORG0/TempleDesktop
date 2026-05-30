#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

static void DrawScene(Display *display, Window window, GC gc, int width, int height) {
    int status_height = 48;
    int status_y = height - status_height;

    XSetForeground(display, gc, 0x0000FF);
    XFillRectangle(display, window, gc, 0, 0, (unsigned int)width, (unsigned int)height);

    XSetForeground(display, gc, 0x000088);
    XFillRectangle(display, window, gc, 0, status_y, (unsigned int)width, (unsigned int)status_height);

    XSetForeground(display, gc, 0xFFFFFF);
    XDrawString(display, window, gc, 16, status_y + 18, "CPU00   Description   #_Task_   UnusedStk   UsedMem   AllocMem   Flags", 74);
    XDrawString(display, window, gc, 16, status_y + 34, "Debug: TempleDesktop running", 29);
}

int main(void) {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);
    GC gc = XCreateGC(display, RootWindow(display, screen), 0, NULL);

    Window window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        0, 0,
        (unsigned int)width,
        (unsigned int)height,
        0,
        BlackPixel(display, screen),
        0x0000FF);

    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XStoreName(display, window, "TempleDesktop");
    XDefineCursor(display, window, XCreateFontCursor(display, XC_top_left_arrow));
    XMapWindow(display, window);
    DrawScene(display, window, gc, width, height);

    for (;;) {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == Expose) {
            DrawScene(display, window, gc, width, height);
            continue;
        }
        if (event.type == KeyPress || event.type == DestroyNotify) {
            break;
        }
    }

    XFreeGC(display, gc);
    XCloseDisplay(display);
    return 0;
}