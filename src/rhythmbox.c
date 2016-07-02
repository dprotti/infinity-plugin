/*
 * Copyright (C) 2016 - Duilio Protti
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * The Rhythmbox authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Rhythmbox. This permission is above and beyond the permissions granted
 * by the GPL license by which Rhythmbox is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */
//#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <plugins/rb-plugin-macros.h>
#include <lib/rb-debug.h>
#include <shell/rb-shell.h>
#include <widgets/rb-dialog.h>

#include "infinity.h"

#define RB_TYPE_INFINITY_PLUGIN		(rb_infinity_plugin_get_type ())
#define rb_infinity_plugin(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), RB_TYPE_INFINITY_PLUGIN, RBInfinityPlugin))
#define rb_infinity_plugin_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), RB_TYPE_INFINITY_PLUGIN, RBInfinityPluginClass))
#define RB_IS_INFINITY_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), RB_TYPE_INFINITY_PLUGIN))
#define RB_IS_INFINITY_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), RB_TYPE_INFINITY_PLUGIN))
#define rb_infinity_plugin_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), RB_TYPE_INFINITY_PLUGIN, RBInfinityPluginClass))

typedef struct
{
	PeasExtensionBase parent;
} RBInfinityPlugin;

typedef struct
{
	PeasExtensionBaseClass parent_class;
} RBInfinityPluginClass;


G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

static void rb_infinity_plugin_init (RBInfinityPlugin *plugin);

RB_DEFINE_PLUGIN(RB_TYPE_INFINITY_PLUGIN, RBInfinityPlugin, rb_infinity_plugin,)

static gint32 get_width() { return 640; }
static void	  set_width(gint32 width) { g_message("set_width TODO"); }
static gint32 get_height() { return 480; }
static void	  set_height(gint32 height) { g_message("set_height TODO"); }
static gint32 get_scale() { return 1; }
static gint32 get_max_fps() { return 30; }
static gint32 get_effect_interval() { return 100; }
static gint32 get_color_interval() { return 100; }

static InfParameters params = {
    .get_width = get_width,
    .set_width = set_width,
    .get_height = get_height,
    .set_height = set_height,
    .get_scale = get_scale,
    .get_effect_interval = get_effect_interval,
    .get_color_interval = get_color_interval,
    .get_max_fps = get_max_fps
};

static void notify_critical_error (const gchar *message) { g_message("notify_critical_error TODO"); }
static void disable_plugin() { g_message("disable_plugin TODO"); }

static Player player = {
	.notify_critical_error = notify_critical_error,
	.disable_plugin = disable_plugin
};

static void
rb_infinity_plugin_init (RBInfinityPlugin *plugin)
{
	g_message("RBInfinityPlugin initialising");
	rb_debug ("RBInfinityPlugin initialising");
}

static void
impl_activate (PeasActivatable *plugin)
{
	RBShell *shell;

	g_object_get (plugin, "object", &shell, NULL);
	g_message("Infinity plugin activated, with shell %p", shell);
	//rb_error_dialog (NULL, "Sample Plugin", "Sample plugin activated, with shell %p", shell);
	g_object_unref (shell);
	infinity_init(&params, &player);
}

static void
impl_deactivate	(PeasActivatable *plugin)
{
	g_message("Infinity plugin deactivated");
	//rb_error_dialog (NULL, "Sample Plugin", "Sample plugin deactivated");
	infinity_finish();
}

//G_MODULE_EXPORT void
EXPORT void
peas_register_types (PeasObjectModule *module)
{
	rb_infinity_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
						    PEAS_TYPE_ACTIVATABLE,
						    RB_TYPE_INFINITY_PLUGIN);
}
