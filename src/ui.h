#ifndef __INFINITY_UI__
#define __INFINITY_UI__

#include <functional>
#include <glib.h>

#include "input.h"

struct DisplayCallbacks {
    std::function<void(gint32 width, gint32 height)> notify_resize;
    std::function<void(void)> notify_close;
    std::function<void(gboolean is_visible)> notify_visibility;
    std::function<void(InfinityKey)> queue_key;
};

// These would resolve either to the Gtk or else the Qt implementation - chosen at compile time.

gboolean ui_init(gint32 width, gint32 height, const DisplayCallbacks& callbacks);
void ui_quit(void);
void ui_present(const guint16 *pixels, gint32 width, gint32 height);
void ui_resize(gint32 width, gint32 height);
void ui_toggle_fullscreen(void);
void ui_exit_fullscreen_if_needed(void);

#endif /* __INFINITY_UI__ */
