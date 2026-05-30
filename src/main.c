#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <stdlib.h>

int RenderBackground(Display *display, Window window) {
    int color = 0x0000FF;
    XSetWindowBackground(display, window, color);
    XClearWindow(display, window);
    return 0; // Return 0 on success
}

int RenderCursor(Display *display, Window window) {
    Cursor cursor = XcursorLibraryLoadCursor(display, "left_ptr");
    if (cursor == None) {
        fprintf(stderr, "Failed to load cursor\n");
        return 1; // Return non-zero on failure
    }
    XDefineCursor(display, window, cursor);
    return 0; // Return 0 on success
}

int main() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 800, 600, 1,
                                        BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, window);
    XFlush(display);

    RenderBackground(display, window);
    RenderCursor(display, window);
    XCloseDisplay(display);
    return 0;
}