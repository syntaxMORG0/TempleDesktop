#include <stdio.h>
#include <X11/Xlib.h>

int main() {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }
    XCloseDisplay(display);
    return 0;
}