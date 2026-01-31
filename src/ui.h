#ifndef __INFINITY_UI__
#define __INFINITY_UI__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

gboolean ui_init(gint32 width, gint32 height);
void ui_quit(void);
void ui_present(const guint16 *pixels, gint32 width, gint32 height);
void ui_resize(gint32 width, gint32 height);
void ui_toggle_fullscreen(void);
void ui_exit_fullscreen_if_needed(void);

void display_notify_resize(gint32 width, gint32 height);
void display_notify_close(void);
void display_notify_visibility(gboolean is_visible);

#ifdef __cplusplus
}
#endif

#endif /* __INFINITY_UI__ */
