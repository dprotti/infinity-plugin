#ifndef __INFINITY_UI__
#define __INFINITY_UI__

#include <glib.h>

struct DisplayCallbacks {
    void (*notify_resize)(gint32 width, gint32 height);
    void (*notify_close)(void);
    void (*notify_visibility)(gboolean is_visible);
};

// These would resolve either to the Gtk or else the Qt implementation - chosen at compile time.

gboolean ui_init(gint32 width, gint32 height, const DisplayCallbacks* callbacks);
void ui_quit(void);
void ui_present(const guint16 *pixels, gint32 width, gint32 height);
void ui_resize(gint32 width, gint32 height);
void ui_toggle_fullscreen(void);
void ui_exit_fullscreen_if_needed(void);

#endif /* __INFINITY_UI__ */
