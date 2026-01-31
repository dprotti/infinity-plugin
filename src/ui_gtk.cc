#include "ui.h"
#include "input.h"

#include <gtk/gtk.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

namespace {

GtkWidget *window_instance = nullptr;
GtkWidget *drawing_area = nullptr;
std::vector<guint16> frame_buffer;
gint32 frame_width = 0;
gint32 frame_height = 0;
bool gtk_ready = false;
bool is_fullscreen = false;
std::mutex frame_mutex;

bool ensure_gtk_ready() {
	if (gtk_ready) {
		return true;
	}
	if (gtk_init_check(nullptr, nullptr) == FALSE) {
		g_message("Infinity: unable to initialize GTK");
		return false;
	}
	gtk_ready = true;
	return true;
}

void process_events() {
	while (gtk_events_pending()) {
		gtk_main_iteration_do(FALSE);
	}
}

gint current_scale_factor(GtkWidget *widget) {
	if (widget == nullptr) {
		return 1;
	}
	return std::max(gtk_widget_get_scale_factor(widget), 1);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer) {
	std::vector<guint16> frame_copy;
	gint32 width = 0;
	gint32 height = 0;
	{
		std::lock_guard<std::mutex> lock(frame_mutex);
		if (frame_buffer.empty()) {
			return FALSE;
		}
		frame_copy = frame_buffer;
		width = frame_width;
		height = frame_height;
	}

	const gint32 target_width = gtk_widget_get_allocated_width(widget);
	const gint32 target_height = gtk_widget_get_allocated_height(widget);
	if (target_width <= 0 || target_height <= 0) {
		return FALSE;
	}

	const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB16_565, width);
	std::vector<unsigned char> surface_buffer(static_cast<size_t>(stride) * height);
	const unsigned char *source = reinterpret_cast<const unsigned char *>(frame_copy.data());
	const int row_bytes = width * static_cast<int>(sizeof(guint16));
	for (gint32 row = 0; row < height; ++row) {
		std::memcpy(surface_buffer.data() + (stride * row),
			    source + (row_bytes * row),
			    row_bytes);
	}

	cairo_surface_t *surface = cairo_image_surface_create_for_data(
		surface_buffer.data(),
		CAIRO_FORMAT_RGB16_565,
		width,
		height,
		stride);

	cairo_save(cr);
	const double scale_x = static_cast<double>(target_width) / static_cast<double>(width);
	const double scale_y = static_cast<double>(target_height) / static_cast<double>(height);
	cairo_scale(cr, scale_x, scale_y);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_surface_destroy(surface);
	return FALSE;
}

void on_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer) {
	if (allocation == nullptr) {
		return;
	}
	const gint scale = current_scale_factor(widget);
	const gint32 pixel_width = allocation->width * scale;
	const gint32 pixel_height = allocation->height * scale;
	display_notify_resize(pixel_width, pixel_height);
}

gboolean on_delete_event(GtkWidget *, GdkEvent *, gpointer) {
	display_notify_close();
	return FALSE;
}

void on_show(GtkWidget *, gpointer) {
	display_notify_visibility(TRUE);
}

void on_hide(GtkWidget *, gpointer) {
	display_notify_visibility(FALSE);
}

gboolean on_key_press(GtkWidget *, GdkEventKey *event, gpointer) {
	switch (event->keyval) {
	case GDK_KEY_Right:
		infinity_queue_key(INFINITY_KEY_RIGHT);
		break;
	case GDK_KEY_Left:
		infinity_queue_key(INFINITY_KEY_LEFT);
		break;
	case GDK_KEY_Up:
		infinity_queue_key(INFINITY_KEY_UP);
		break;
	case GDK_KEY_Down:
		infinity_queue_key(INFINITY_KEY_DOWN);
		break;
	case GDK_KEY_z:
	case GDK_KEY_Z:
		infinity_queue_key(INFINITY_KEY_PREV);
		break;
	case GDK_KEY_x:
	case GDK_KEY_X:
		infinity_queue_key(INFINITY_KEY_PLAY);
		break;
	case GDK_KEY_c:
	case GDK_KEY_C:
		infinity_queue_key(INFINITY_KEY_PAUSE);
		break;
	case GDK_KEY_v:
	case GDK_KEY_V:
		infinity_queue_key(INFINITY_KEY_STOP);
		break;
	case GDK_KEY_b:
	case GDK_KEY_B:
		infinity_queue_key(INFINITY_KEY_NEXT);
		break;
	case GDK_KEY_F11:
		infinity_queue_key(INFINITY_KEY_FULLSCREEN);
		break;
	case GDK_KEY_Escape:
		infinity_queue_key(INFINITY_KEY_EXIT_FULLSCREEN);
		break;
	case GDK_KEY_F12:
		infinity_queue_key(INFINITY_KEY_NEXT_PALETTE);
		break;
	case GDK_KEY_space:
		infinity_queue_key(INFINITY_KEY_NEXT_EFFECT);
		break;
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
		infinity_queue_key(INFINITY_KEY_TOGGLE_INTERACTIVE);
		break;
	default:
		break;
	}
	return FALSE;
}

gboolean on_window_state(GtkWidget *, GdkEventWindowState *event, gpointer) {
	if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0) {
		return FALSE;
	}
	is_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
	return FALSE;
}

void notify_current_size() {
	if (drawing_area == nullptr) {
		return;
	}
	const gint scale = current_scale_factor(drawing_area);
	const gint32 width = gtk_widget_get_allocated_width(drawing_area) * scale;
	const gint32 height = gtk_widget_get_allocated_height(drawing_area) * scale;
	display_notify_resize(width, height);
}

} // namespace

gboolean ui_init(gint32 width, gint32 height)
{
	if (!ensure_gtk_ready()) {
		return FALSE;
	}

	if (window_instance != nullptr) {
		return TRUE;
	}

	window_instance = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window_instance), "Infinity");
	gtk_widget_set_size_request(window_instance, 200, 150);
	gtk_widget_realize(window_instance);

	const gint scale = current_scale_factor(window_instance);
	const gint32 logical_width = std::max(width / scale, 1);
	const gint32 logical_height = std::max(height / scale, 1);
	gtk_window_resize(GTK_WINDOW(window_instance), logical_width, logical_height);
	gtk_window_set_resizable(GTK_WINDOW(window_instance), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window_instance), logical_width, logical_height);

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_hexpand(drawing_area, TRUE);
	gtk_widget_set_vexpand(drawing_area, TRUE);
	gtk_container_add(GTK_CONTAINER(window_instance), drawing_area);

	gtk_widget_add_events(window_instance, GDK_KEY_PRESS_MASK);

	g_signal_connect(window_instance, "delete-event", G_CALLBACK(on_delete_event), nullptr);
	g_signal_connect(window_instance, "show", G_CALLBACK(on_show), nullptr);
	g_signal_connect(window_instance, "hide", G_CALLBACK(on_hide), nullptr);
	g_signal_connect(window_instance, "key-press-event", G_CALLBACK(on_key_press), nullptr);
	g_signal_connect(window_instance, "window-state-event", G_CALLBACK(on_window_state), nullptr);
	g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), nullptr);
	g_signal_connect(drawing_area, "size-allocate", G_CALLBACK(on_size_allocate), nullptr);

	gtk_widget_show_all(window_instance);
	process_events();
	return TRUE;
}

void ui_ensure_app(void)
{
	ensure_gtk_ready();
}

void ui_quit(void)
{
	if (window_instance == nullptr) {
		return;
	}
	gtk_widget_destroy(window_instance);
	window_instance = nullptr;
	drawing_area = nullptr;
}

void ui_present(const guint16 *pixels, gint32 width, gint32 height)
{
	if (drawing_area == nullptr || pixels == nullptr || width <= 0 || height <= 0) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(frame_mutex);
		frame_width = width;
		frame_height = height;
		frame_buffer.assign(pixels, pixels + (width * height));
	}
	gtk_widget_queue_draw(drawing_area);
	process_events();
}

void ui_resize(gint32 width, gint32 height)
{
	if (window_instance == nullptr) {
		return;
	}
	const gint scale = current_scale_factor(window_instance);
	const gint32 logical_width = std::max(width / scale, 1);
	const gint32 logical_height = std::max(height / scale, 1);
	gtk_window_resize(GTK_WINDOW(window_instance), logical_width, logical_height);
	process_events();
}

void ui_toggle_fullscreen(void)
{
	if (window_instance == nullptr) {
		return;
	}
	if (is_fullscreen) {
		gtk_window_unfullscreen(GTK_WINDOW(window_instance));
	} else {
		gtk_window_fullscreen(GTK_WINDOW(window_instance));
	}
	process_events();
	notify_current_size();
}

void ui_exit_fullscreen_if_needed(void)
{
	if (window_instance == nullptr || !is_fullscreen) {
		return;
	}
	gtk_window_unfullscreen(GTK_WINDOW(window_instance));
	process_events();
	notify_current_size();
}
