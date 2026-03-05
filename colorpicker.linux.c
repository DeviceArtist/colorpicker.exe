#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xrandr.h>
#include <X11/extensions/Xfixes.h>
#include <X11/Xatom.h>

// Global variables
Display *g_display = NULL;
Window g_root_window;
int g_screen_num;
XImage *g_screen_image = NULL;
unsigned int g_screen_width = 0, g_screen_height = 0;
double g_scale = 1.0;  // Zoom scale factor, default 1.0 (original size)
int g_offset_x = 0, g_offset_y = 0;  // Screenshot offset for center-based zoom
XPoint g_last_mouse;  // Last recorded mouse position
Window g_window;  // Main application window

/**
 * Get current mouse position in screen coordinates
 * @return XPoint structure containing x and y coordinates
 */
XPoint get_mouse_position() {
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    
    XQueryPointer(g_display, g_root_window, &root, &child,
                  &root_x, &root_y, &win_x, &win_y, &mask);
    
    XPoint p = {root_x, root_y};
    return p;
}

/**
 * Get pixel color at specific screen coordinates
 * @param x Horizontal screen coordinate
 * @param y Vertical screen coordinate
 * @return Unsigned long representing pixel color value
 */
unsigned long get_pixel_color(int x, int y) {
    if (x < 0 || x >= g_screen_width || y < 0 || y >= g_screen_height)
        return 0;
    
    return XGetPixel(g_screen_image, x, y);
}

/**
 * Copy text string to system clipboard
 * @param text Null-terminated string to copy
 */
void copy_to_clipboard(const char *text) {
    int text_len = strlen(text);
    
    // Get clipboard selection atom
    Atom clipboard = XInternAtom(g_display, "CLIPBOARD", False);
    
    // Request clipboard ownership
    XSetSelectionOwner(g_display, clipboard, g_window, CurrentTime);
    if (XGetSelectionOwner(g_display, clipboard) != g_window) {
        fprintf(stderr, "Failed to acquire clipboard ownership\n");
        return;
    }
    
    // Set clipboard content
    XChangeProperty(g_display, g_window, clipboard, 
                    XA_STRING, 8, PropModeReplace,
                    (unsigned char *)text, text_len);
    
    // Notify clipboard manager of change
    XEvent event;
    memset(&event, 0, sizeof(event));
    event.xclient.type = ClientMessage;
    event.xclient.window = g_window;
    event.xclient.message_type = XInternAtom(g_display, "WM_SAVE_YOURSELF", False);
    event.xclient.format = 32;
    
    XSendEvent(g_display, DefaultRootWindow(g_display), False, 
               NoEventMask, &event);
    XFlush(g_display);
}

/**
 * Create full-screen screenshot of primary display
 */
void create_screenshot() {
    if (g_screen_image) {
        XDestroyImage(g_screen_image);
    }
    
    g_screen_image = XGetImage(g_display, g_root_window, 
                               0, 0, g_screen_width, g_screen_height,
                               AllPlanes, ZPixmap);
}

/**
 * Handle zoom operations while maintaining mouse position as center point
 * @param factor Zoom multiplier (1.2 for 20% zoom in, 1/1.2 for zoom out)
 */
void handle_zoom(double factor) {
    // Get current mouse position
    XPoint mouse = get_mouse_position();
    
    // Calculate mouse position on original screen
    double mouse_x = (mouse.x - g_offset_x) / g_scale;
    double mouse_y = (mouse.y - g_offset_y) / g_scale;
    
    // Adjust zoom scale with limits
    g_scale *= factor;
    if (g_scale > 4.0) g_scale = 4.0;  // Maximum 4x zoom
    if (g_scale < 0.25) g_scale = 0.25;  // Minimum 0.25x zoom
    
    // Calculate new offset to keep mouse position centered
    g_offset_x = mouse.x - (int)(mouse_x * g_scale);
    g_offset_y = mouse.y - (int)(mouse_y * g_scale);
    
    // Ensure screenshot remains fully visible within window bounds
    int draw_width = (int)(g_screen_width * g_scale);
    int draw_height = (int)(g_screen_height * g_scale);
    
    g_offset_x = (g_offset_x > 0) ? 0 : g_offset_x;
    g_offset_y = (g_offset_y > 0) ? 0 : g_offset_y;
    
    if (draw_width < g_screen_width) {
        g_offset_x = 0;
    } else {
        int max_offset = draw_width - g_screen_width;
        g_offset_x = (g_offset_x < -max_offset) ? -max_offset : g_offset_x;
    }
    
    if (draw_height < g_screen_height) {
        g_offset_y = 0;
    } else {
        int max_offset = draw_height - g_screen_height;
        g_offset_y = (g_offset_y < -max_offset) ? -max_offset : g_offset_y;
    }
}

/**
 * Redraw application window content
 */
void redraw_window() {
    XClearWindow(g_display, g_window);
    
    // Create graphics context for drawing
    GC gc = XCreateGC(g_display, g_window, 0, NULL);
    
    // Calculate scaled drawing dimensions and position
    int draw_width = (int)(g_screen_width * g_scale);
    int draw_height = (int)(g_screen_height * g_scale);
    
    // Draw scaled screenshot
    if (g_screen_image) {
        // Disable graphics exposures for better performance
        XSetGraphicsExposures(g_display, gc, False);
        XPutImage(g_display, g_window, gc, g_screen_image,
                  0, 0, g_offset_x, g_offset_y,
                  draw_width, draw_height);
    }
    
    // Display current zoom level
    char scale_text[32];
    snprintf(scale_text, sizeof(scale_text), "Zoom: %.1f%%", g_scale * 100);
    
    XSetForeground(g_display, gc, WhitePixel(g_display, g_screen_num));
    XDrawString(g_display, g_window, gc, 10, 20, scale_text, strlen(scale_text));
    
    // Display operation hints
    char hint_text[128];
    snprintf(hint_text, sizeof(hint_text), 
             "Left Click: Copy HEX | Enter: Copy RGB | +/-: Zoom | 0: Reset");
    
    XDrawString(g_display, g_window, gc, 10, 40, hint_text, strlen(hint_text));
    
    XFreeGC(g_display, gc);
}

int main() {
    // Connect to X server
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        fprintf(stderr, "Failed to open X display\n");
        return 1;
    }
    
    g_screen_num = DefaultScreen(g_display);
    g_root_window = DefaultRootWindow(g_display);
    
    // Get screen resolution using XRandR for multi-monitor support
    XRRScreenResources *res = XRRGetScreenResourcesCurrent(g_display, g_root_window);
    if (res) {
        XRRCrtcInfo *crtc = XRRGetCrtcInfo(g_display, res, res->crtcs[0]);
        if (crtc) {
            g_screen_width = crtc->width;
            g_screen_height = crtc->height;
            XRRFreeCrtcInfo(crtc);
        }
        XRRFreeScreenResources(res);
    }
    
    // Fallback to default screen dimensions if XRandR fails
    if (g_screen_width == 0 || g_screen_height == 0) {
        g_screen_width = DisplayWidth(g_display, g_screen_num);
        g_screen_height = DisplayHeight(g_display, g_screen_num);
    }
    
    // Create main application window
    XSetWindowAttributes attr;
    attr.override_redirect = True;  // Bypass window manager for fullscreen
    attr.background_pixel = BlackPixel(g_display, g_screen_num);
    attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask;
    
    g_window = XCreateWindow(g_display, g_root_window,
                             0, 0, g_screen_width, g_screen_height,
                             0, DefaultDepth(g_display, g_screen_num),
                             InputOutput, DefaultVisual(g_display, g_screen_num),
                             CWOverrideRedirect | CWBackPixel | CWEventMask, &attr);
    
    // Set window title
    XStoreName(g_display, g_window, "Color Picker");
    
    // Display window
    XMapRaised(g_display, g_window);
    XFlush(g_display);
    
    // Create initial full-screen screenshot
    create_screenshot();
    
    // Set crosshair cursor
    Cursor cursor = XCreateFontCursor(g_display, XC_crosshair);
    XDefineCursor(g_display, g_window, cursor);
    
    // Event loop
    XEvent event;
    int running = 1;
    
    while (running) {
        XNextEvent(g_display, &event);
        
        switch (event.type) {
            case Expose:
                redraw_window();
                break;
                
            case MotionNotify:
                g_last_mouse.x = event.xmotion.x_root;
                g_last_mouse.y = event.xmotion.y_root;
                break;
                
            case KeyPress: {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                
                switch (key) {
                    case XK_plus:
                        handle_zoom(1.2);  // Zoom in by 20%
                        redraw_window();
                        break;
                        
                    case XK_minus:
                        handle_zoom(1/1.2);  // Zoom out by 20%
                        redraw_window();
                        break;
                        
                    case XK_0:
                        g_scale = 1.0;
                        g_offset_x = 0;
                        g_offset_y = 0;
                        redraw_window();
                        break;
                        
                    case XK_Return: {
                        // Convert window coordinates to original screen coordinates
                        int x = (g_last_mouse.x - g_offset_x) / g_scale;
                        int y = (g_last_mouse.y - g_offset_y) / g_scale;
                        
                        // Ensure coordinates stay within screen bounds
                        x = (x < 0) ? 0 : (x >= g_screen_width) ? g_screen_width - 1 : x;
                        y = (y < 0) ? 0 : (y >= g_screen_height) ? g_screen_height - 1 : y;
                        
                        // Get pixel color and convert to RGB values
                        unsigned long pixel = get_pixel_color(x, y);
                        XColor color;
                        XQueryColor(g_display, DefaultColormap(g_display, g_screen_num), &color);
                        color.pixel = pixel;
                        XQueryColor(g_display, DefaultColormap(g_display, g_screen_num), &color);
                        
                        // Format RGB string
                        char rgb_str[32];
                        snprintf(rgb_str, sizeof(rgb_str), "RGB(%d,%d,%d)",
                                 color.red >> 8, color.green >> 8, color.blue >> 8);
                        
                        // Copy to clipboard and exit
                        copy_to_clipboard(rgb_str);
                        running = 0;
                        break;
                    }
                }
                break;
            }
            
            case ButtonPress: {
                if (event.xbutton.button == Button1) {  // Left mouse button click
                    // Convert window coordinates to original screen coordinates
                    int x = (event.xbutton.x_root - g_offset_x) / g_scale;
                    int y = (event.xbutton.y_root - g_offset_y) / g_scale;
                    
                    // Ensure coordinates stay within screen bounds
                    x = (x < 0) ? 0 : (x >= g_screen_width) ? g_screen_width - 1 : x;
                    y = (y < 0) ? 0 : (y >= g_screen_height) ? g_screen_height - 1 : y;
                    
                    // Get pixel color and convert to RGB values
                    unsigned long pixel = get_pixel_color(x, y);
                    XColor color;
                    XQueryColor(g_display, DefaultColormap(g_display, g_screen_num), &color);
                    color.pixel = pixel;
                    XQueryColor(g_display, DefaultColormap(g_display, g_screen_num), &color);
                    
                    // Format HEX string
                    char hex_str[16];
                    snprintf(hex_str, sizeof(hex_str), "#%02X%02X%02X",
                             color.red >> 8, color.green >> 8, color.blue >> 8);
                    
                    // Copy to clipboard and exit
                    copy_to_clipboard(hex_str);
                    running = 0;
                }
                break;
            }
        }
    }
    
    // Clean up resources
    if (g_screen_image) {
        XDestroyImage(g_screen_image);
    }
    
    XFreeCursor(g_display, cursor);
    XDestroyWindow(g_display, g_window);
    XCloseDisplay(g_display);
    
    return 0;
}