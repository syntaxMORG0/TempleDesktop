/*
 * TempleDesktop - minimal window system with a scrollable text pane
 * - Blue desktop background
 * - Dark status bar at bottom
 * - Scrollable text area with keyboard and mouse-wheel support
 * Controls: Up/Down/PageUp/PageDown/Home/End, mouse wheel, 'q' to quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

static void draw(Display *display, Window window, GC gc, XFontStruct *fontinfo,
                 char **lines, int num_lines, int scroll, int width, int height) {
    int status_height = 48;
    int status_y = height - status_height;
    const int pad_x = 16;
    const int pad_y = 16;

    /* blue background */
    XSetForeground(display, gc, 0x0000FF);
    XFillRectangle(display, window, gc, 0, 0, (unsigned int)width, (unsigned int)height);

    /* darker status bar */
    XSetForeground(display, gc, 0x000099);
    XFillRectangle(display, window, gc, 0, status_y, (unsigned int)width, (unsigned int)status_height);

    /* white status text */
    XSetForeground(display, gc, 0xFFFFFF);
    const char *line1 = "CPU00   Description   #_Task_   UnusedStk   UsedMem   AllocMem   Flags";
    const char *line2 = "Debug: TempleDesktop running";
    XDrawString(display, window, gc, pad_x, status_y + 18, line1, (int)strlen(line1));
    XDrawString(display, window, gc, pad_x, status_y + 36, line2, (int)strlen(line2));

    /* draw scrollable text pane above the status bar */
    int line_height = fontinfo->ascent + fontinfo->descent;
    int available_h = status_y - pad_y - pad_y;
    int lines_per_page = (line_height > 0) ? (available_h / line_height) : 0;
    if (lines_per_page <= 0) return;

    int i, idx;
    for (i = 0; i < lines_per_page; ++i) {
        idx = scroll + i;
        if (idx >= num_lines) break;
        int y = pad_y + (i * line_height) + fontinfo->ascent;
        XDrawString(display, window, gc, pad_x, y, lines[idx], (int)strlen(lines[idx]));
    }

    /* floating child window will be drawn by caller after this draw() call */
}

/* Find a top-level window by its WM_NAME (window name). Returns 0 if not found. */
Window find_window_by_name(Display *dpy, Window root, const char *name) {
    Window ret = 0;
    Window root_return, parent_return;
    Window *children;
    unsigned int nchildren;
    if (!XQueryTree(dpy, root, &root_return, &parent_return, &children, &nchildren)) return 0;
    for (unsigned int i = 0; i < nchildren; ++i) {
        char *wname = NULL;
        if (XFetchName(dpy, children[i], &wname) && wname) {
            if (strcmp(wname, name) == 0) {
                ret = children[i];
                XFree(wname);
                break;
            }
            XFree(wname);
        }
        /* search recursively */
        ret = find_window_by_name(dpy, children[i], name);
        if (ret) break;
    }
    if (children) XFree(children);
    return ret;
}

/* Spawn xterm with a WM_NAME and reparent its window into our parent at (x,y). */
int renderwindow_embed_xterm(Display *dpy, Window parent, const char *xterm_path, const char *winname, int x, int y, int w, int h) {
    pid_t pid = fork();
    if (pid == 0) {
        /* child */
        execlp(xterm_path, xterm_path, "-name", winname, (char*)NULL);
        _exit(127);
    } else if (pid < 0) {
        return -1;
    }

    /* parent: wait for the window to appear (timeout ~3s) */
    Window childwin = 0;
    int attempts = 30;
    for (int i = 0; i < attempts; ++i) {
        usleep(100 * 1000); /* 100ms */
        childwin = find_window_by_name(dpy, DefaultRootWindow(dpy), winname);
        if (childwin) break;
    }
    if (!childwin) return -2; /* not found */

    /* Reparent into our parent window at position x,y */
    XReparentWindow(dpy, childwin, parent, x, y);
    XMapWindow(dpy, childwin);
    XFlush(dpy);
    /* optionally wait a bit for mapping */
    return (int)childwin;
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
    Window root = RootWindow(display, screen);

    Window window = XCreateSimpleWindow(
        display,
        root,
        0, 0,
        (unsigned int)width,
        (unsigned int)height,
        0,
        BlackPixel(display, screen),
        0x0000FF);

    /* receive mouse buttons, motion and release for dragging and wheel */
    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask
                 | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XStoreName(display, window, "TempleDesktop");

    /* font and GC */
    XFontStruct *fontinfo = XLoadQueryFont(display, "fixed");
    if (!fontinfo) fontinfo = XLoadQueryFont(display, "6x13");
    if (!fontinfo) {
        fprintf(stderr, "Failed to load font\n");
        XCloseDisplay(display);
        return 1;
    }
    GC gc = XCreateGC(display, window, 0, NULL);
    XSetFont(display, gc, fontinfo->fid);

    Cursor cursor = XCreateFontCursor(display, XC_top_left_arrow);
    XDefineCursor(display, window, cursor);

    /* sample buffer: replaceable with real output stream */
    int num_lines = 400;
    char **lines = calloc((size_t)num_lines, sizeof(char*));
    if (!lines) { perror("calloc"); return 1; }
    for (int i = 0; i < num_lines; ++i) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Line %03d: This is a sample desktop terminal line to demonstrate scrolling.", i+1);
        lines[i] = strdup(buf);
    }
    int scroll = 0;

    /* floating window state */
    int fw_x = 80, fw_y = 80, fw_w = 480, fw_h = 240, fw_title = 24;
    bool fw_drag = false;
    int drag_off_x = 0, drag_off_y = 0;

    XMapWindow(display, window);
    draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);

    for (;;) {
        XEvent ev;
        XNextEvent(display, &ev);
        if (ev.type == Expose) {
            draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
            /* draw floating child window on top */
            XSetForeground(display, gc, 0xDDDDDD);
            XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
            /* title bar */
            XSetForeground(display, gc, 0x333399);
            XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_title);
            /* border */
            XSetForeground(display, gc, 0x000000);
            XDrawRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
            /* title text */
            XSetForeground(display, gc, 0xFFFFFF);
            XDrawString(display, window, gc, fw_x + 8, fw_y + (fw_title/2) + 4, "Floating", 8);
        } else if (ev.type == ConfigureNotify) {
            XConfigureEvent xce = ev.xconfigure;
            if ((int)xce.width != width || (int)xce.height != height) {
                width = xce.width;
                height = xce.height;
                draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
            }
        } else if (ev.type == KeyPress) {
            KeySym ks = XLookupKeysym(&ev.xkey, 0);
            int line_height = fontinfo->ascent + fontinfo->descent;
            int available_h = (height - 48) - 16 - 16;
            int lines_per_page = (line_height > 0) ? (available_h / line_height) : 1;
            if (ks == XK_Up) {
                if (scroll > 0) --scroll;
            } else if (ks == XK_Down) {
                if (scroll < num_lines - lines_per_page) ++scroll;
            } else if (ks == XK_Page_Up) {
                scroll -= lines_per_page; if (scroll < 0) scroll = 0;
            } else if (ks == XK_Page_Down) {
                scroll += lines_per_page; if (scroll > num_lines - lines_per_page) scroll = num_lines - lines_per_page;
            } else if (ks == XK_Home) {
                scroll = 0;
            } else if (ks == XK_End) {
                scroll = num_lines - lines_per_page; if (scroll < 0) scroll = 0;
            } else if (ks == XK_q || ks == XK_Q || ks == XK_Escape) {
                break;
            } else if (ks == XK_x || ks == XK_X) {
                /* demo: spawn and embed into the floating window */
                char namebuf[64];
                snprintf(namebuf, sizeof(namebuf), "TEMPLE_XTERM_%ld", (long)time(NULL));
                int res = renderwindow_embed_xterm(display, window, "xterm", namebuf, fw_x, fw_y + fw_title, fw_w, fw_h - fw_title);
                if (res < 0) {
                    fprintf(stderr, "embed failed: %d\n", res);
                }
            }
            draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
            /* redraw floating window after updating content */
            XSetForeground(display, gc, 0xDDDDDD);
            XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
            XSetForeground(display, gc, 0x333399);
            XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_title);
            XSetForeground(display, gc, 0x000000);
            XDrawRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
            XSetForeground(display, gc, 0xFFFFFF);
            XDrawString(display, window, gc, fw_x + 8, fw_y + (fw_title/2) + 4, "Floating", 8);
        } else if (ev.type == ButtonPress) {
            if (ev.xbutton.button == 4) { /* wheel up */
                if (scroll > 0) --scroll;
                draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
                /* redraw floating window */
                XSetForeground(display, gc, 0xDDDDDD);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0x333399);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_title);
                XSetForeground(display, gc, 0x000000);
                XDrawRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0xFFFFFF);
                XDrawString(display, window, gc, fw_x + 8, fw_y + (fw_title/2) + 4, "Floating", 8);
            } else if (ev.xbutton.button == 5) { /* wheel down */
                int line_height = fontinfo->ascent + fontinfo->descent;
                int available_h = (height - 48) - 16 - 16;
                int lines_per_page = (line_height > 0) ? (available_h / line_height) : 1;
                if (scroll < num_lines - lines_per_page) ++scroll;
                draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
                XSetForeground(display, gc, 0xDDDDDD);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0x333399);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_title);
                XSetForeground(display, gc, 0x000000);
                XDrawRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0xFFFFFF);
                XDrawString(display, window, gc, fw_x + 8, fw_y + (fw_title/2) + 4, "Floating", 8);
            }
            /* check for click inside floating title bar to start drag */
            if (ev.xbutton.button == Button1) {
                int mx = ev.xbutton.x;
                int my = ev.xbutton.y;
                if (mx >= fw_x && mx <= fw_x + fw_w && my >= fw_y && my <= fw_y + fw_title) {
                    fw_drag = true;
                    drag_off_x = mx - fw_x;
                    drag_off_y = my - fw_y;
                }
            }
        }
        else if (ev.type == ButtonRelease) {
            if (ev.xbutton.button == Button1 && fw_drag) fw_drag = false;
        }
        else if (ev.type == MotionNotify) {
            if (fw_drag) {
                int mx = ev.xmotion.x;
                int my = ev.xmotion.y;
                fw_x = mx - drag_off_x;
                fw_y = my - drag_off_y;
                if (fw_x < 0) fw_x = 0;
                if (fw_y < 0) fw_y = 0;
                if (fw_x + fw_w > width) fw_x = width - fw_w;
                if (fw_y + fw_h > height - 48) fw_y = height - 48 - fw_h;
                draw(display, window, gc, fontinfo, lines, num_lines, scroll, width, height);
                XSetForeground(display, gc, 0xDDDDDD);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0x333399);
                XFillRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_title);
                XSetForeground(display, gc, 0x000000);
                XDrawRectangle(display, window, gc, fw_x, fw_y, fw_w, fw_h);
                XSetForeground(display, gc, 0xFFFFFF);
                XDrawString(display, window, gc, fw_x + 8, fw_y + (fw_title/2) + 4, "Floating", 8);
            }
        }
    }

    for (int i = 0; i < num_lines; ++i) free(lines[i]);
    free(lines);
    XFreeCursor(display, cursor);
    XFreeGC(display, gc);
    XFreeFont(display, fontinfo);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
