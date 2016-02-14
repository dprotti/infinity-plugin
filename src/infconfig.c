#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <fcntl.h>

#include "config.h"
#include "infconfig.h"
#include "gettext.h"
#include "config-dialog.h"

#define DEFAULT_WIDTH                   512
#define DEFAULT_HEIGHT                  288
#define DEFAULT_TIME_EFFECT             100
#define DEFAULT_TIME_PALETTE            100
#define DEFAULT_SCALE_FACTOR            1
#define DEFAULT_FPS                     15
#define DEFAULT_SHOW_TITLE              TRUE

#define INFINITY_CONFIG_DIR             "/.audacious/infinity"
#define INFINITY_CONFIG_FILE            (INFINITY_CONFIG_DIR "/infinity.ini")

typedef struct t_config {
	gint32		x, y;
	gint32		xres;
	gint32		yres;
	gint32		sres;
	gint32		teff;
	gint32		tcol;
	gint32		fps;
	gboolean	show_title;
} t_config;


static t_config config = { -1, -1, 0, 0, 0, 0, 0, 0, DEFAULT_SHOW_TITLE };
static ConfigDialog *configure_dialog = NULL;

/*
 * Here we save values selected on configure dialog, but which
 * have not been yet applied, so on cancel click we just forget
 * them.
 */
static t_config temp_config;

/*
 *
 * Private functions
 *
 */


/*
 * Callbacks for config_plugin_config_window
 */
static void scale_value_changed(GtkWidget *widget, gpointer data)
{
	temp_config.sres = ((gint32)(GTK_ADJUSTMENT(widget)->value));
}

static void teff_value_changed(GtkWidget *widget, gpointer data)
{
	temp_config.teff = ((gint32)(GTK_ADJUSTMENT(widget)->value));
}

static void tcol_value_changed(GtkWidget *widget, gpointer data)
{
	temp_config.tcol = ((gint32)(GTK_ADJUSTMENT(widget)->value));
}

static void on_fps_changed(GtkWidget *widget, gpointer data)
{
	gint32 value;

	value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(configure_dialog->spinbutton_fps));
	temp_config.fps = value;
	g_message("fps changed to %d", value);
}

static void on_show_title_toggled(GtkWidget *widget, gpointer data)
{
	temp_config.show_title = !temp_config.show_title;
}

/*static gboolean check_cfg_version (ConfigFile *f)
 * {
 *  gchar *vstr;
 *
 *  if (xmms_cfg_read_string (f, "infinity", "version", &vstr))
 *      if (!strcmp (vstr, PACKAGE_VERSION))
 *          return TRUE;
 *  return FALSE;
 *
 *  return TRUE;
 * }*/
static void sync_options()
{
	/*g_message ("Synchronizing options: sres %d, teff %d, tcol %d, fps %d",
	 * temp_config.sres, temp_config.teff, temp_config.tcol, temp_config.fps);*/
	config_set_sres(temp_config.sres);
	config_set_teff(temp_config.teff);
	config_set_tcol(temp_config.tcol);
	config_set_fps(temp_config.fps);
	config_set_show_title(temp_config.show_title);
}

static void apply_clicked(GtkWidget *w, GtkWidget *window)
{
	g_message("Applying changes ...");
	sync_options();
	config_plugin_save_prefs();
}

static void ok_clicked(GtkWidget *w, GtkWidget *window)
{
	apply_clicked(w, window);
	config_dialog_hide(configure_dialog);
}

static void cancel_clicked(GtkWidget *w, GtkWidget *window)
{
	g_message("Cancelling changes...");
	config_dialog_hide(configure_dialog);
}

static void on_config_dialog_destroy(GtkWidget *widget, gpointer data)
{
	config_dialog_destroy(configure_dialog);
	configure_dialog = NULL;
}

/*
 * This function set widgets on configure dialog according with
 * saved values. This way after cancel on a configure dialog, if
 * we reopen it we get the saved values.
 */
static void set_config_values(ConfigDialog *config_dialog)
{
	GtkAdjustment *teadj, *tcadj, *scale_adj;

	g_return_if_fail(configure_dialog != NULL);

	teadj = gtk_range_get_adjustment(GTK_RANGE(config_dialog->hscale_effect));
	gtk_adjustment_set_value(teadj, config_get_teff());

	tcadj = gtk_range_get_adjustment(GTK_RANGE(config_dialog->hscale_palette));
	gtk_adjustment_set_value(tcadj, config_get_tcol());

	scale_adj = gtk_range_get_adjustment(GTK_RANGE(config_dialog->hscale_scale_factor));
	gtk_adjustment_set_value(scale_adj, config_get_sres());

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_dialog->spinbutton_fps), config_get_fps());

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(config_dialog->checkbutton_show_title), config_get_show_title());

	/* Memorize current values */
	temp_config.sres = config_get_sres();
	temp_config.teff = config_get_teff();
	temp_config.tcol = config_get_tcol();
	temp_config.fps = config_get_fps();
	temp_config.show_title = config_get_show_title();
}

static void connect_callbacks(ConfigDialog *configure_dialog)
{
	GtkAdjustment *teadj, *tcadj, *scale_adj;

	g_return_if_fail(configure_dialog != NULL);

	gtk_signal_connect(GTK_OBJECT(configure_dialog->window_main), "destroy",
			   GTK_SIGNAL_FUNC(on_config_dialog_destroy), NULL);

	teadj = gtk_range_get_adjustment(GTK_RANGE(configure_dialog->hscale_effect));
	tcadj = gtk_range_get_adjustment(GTK_RANGE(configure_dialog->hscale_palette));
	scale_adj = gtk_range_get_adjustment(GTK_RANGE(configure_dialog->hscale_scale_factor));

	gtk_signal_connect(GTK_OBJECT(teadj), "value-changed",
			   GTK_SIGNAL_FUNC(teff_value_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(tcadj), "value-changed",
			   GTK_SIGNAL_FUNC(tcol_value_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(scale_adj), "value-changed",
			   GTK_SIGNAL_FUNC(scale_value_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(configure_dialog->spinbutton_fps), "changed",
			   GTK_SIGNAL_FUNC(on_fps_changed), NULL);
	gtk_signal_connect(GTK_OBJECT(configure_dialog->checkbutton_show_title), "toggled",
			   GTK_SIGNAL_FUNC(on_show_title_toggled), NULL);

	gtk_signal_connect(GTK_OBJECT(configure_dialog->button_ok), "clicked",
			   GTK_SIGNAL_FUNC(ok_clicked), NULL);
	gtk_signal_connect(GTK_OBJECT(configure_dialog->button_cancel), "clicked",
			   GTK_SIGNAL_FUNC(cancel_clicked), NULL);
	gtk_signal_connect(GTK_OBJECT(configure_dialog->button_apply), "clicked",
			   GTK_SIGNAL_FUNC(apply_clicked), NULL);
}

/*
 *
 * Public functions
 *
 */
void config_plugin_load_prefs(void)
{
	gint value;
	GKeyFile *kf;
	gchar *config_file_path;
	gboolean error, must_update, config_file_usable;

	kf = g_key_file_new();
	config_file_path = g_strconcat(g_get_home_dir(), INFINITY_CONFIG_FILE, NULL);
	config_file_usable = g_key_file_load_from_file(kf, config_file_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
	error = must_update = FALSE;
	if (config_file_usable) {
		if (TRUE) {
			//TODO if (check_cfg_version (f)) {
			//    g_message("version Ok");
			if ((value = g_key_file_get_integer(kf, "Preferences", "xres", NULL)) > 0)
				config_set_xres(value);
			else
				error = TRUE;
			if ((value = g_key_file_get_integer(kf, "Preferences", "yres", NULL)) > 0)
				config_set_yres(value);
			else
				error = TRUE;
			if ((value = g_key_file_get_integer(kf, "Preferences", "teff", NULL)) > 0)
				config_set_teff(value);
			else
				error = TRUE;
			if ((value = g_key_file_get_integer(kf, "Preferences", "tcol", NULL)) > 0)
				config_set_tcol(value);
			else
				error = TRUE;
			if ((value = g_key_file_get_integer(kf, "Preferences", "sres", NULL)) > 0)
				config_set_sres(value);
			else
				error = TRUE;
			if ((value = g_key_file_get_integer(kf, "Preferences", "fps", NULL)) > 0)
				config_set_fps(value);
			else
				error = TRUE;
			//config_set_show_title (g_key_file_get_boolean (kf, "Preferences", "show_title", NULL));

			/* FIXME for now these are not used */
			//xmms_cfg_read_int (f, "infinity", "xorig", &value);
			//config_set_x (value);
			//xmms_cfg_read_int (f, "infinity", "yorig", &value);
			//config_set_y (value);
		} else {
			config_set_default_values();
			must_update = TRUE;
		}
	} else {
		config_set_default_values();
		must_update = TRUE;
	}
	g_free(config_file_path);
	g_key_file_free(kf);

	if (error || must_update) {
		if (error)
			g_message("Fixing errors on Infinity's configure file...");
		else
			g_message("Updating Infinity's configure file...");

		config_plugin_save_prefs();
	}
}

void config_plugin_save_prefs(void)
{
	gchar *config_file_path, *config_dir_path;
	GKeyFile *kf;
	gboolean ok;
	gchar *preferences;
	gsize length;

	kf = g_key_file_new();
	config_dir_path = g_strconcat(g_get_home_dir(), INFINITY_CONFIG_DIR, NULL);
	config_file_path = g_strconcat(g_get_home_dir(), INFINITY_CONFIG_FILE, NULL);
	ok = g_key_file_load_from_file(kf, config_file_path, G_KEY_FILE_KEEP_COMMENTS, NULL);
	if (!ok) {
		g_message(_("Infinity plugin .ini file not found"));
		if (!g_file_test(config_dir_path, G_FILE_TEST_EXISTS)) {
			g_message(_("Creating Infinity plugin config directory '%s'"), INFINITY_CONFIG_DIR);
			if (g_mkdir_with_parents(config_dir_path, S_IRWXU) != 0) {
				g_warning(_("Cannot save preferences: unable to create directory '%s'"), config_dir_path);
				g_free(config_file_path);
				g_free(config_dir_path);
				g_key_file_free(kf);
				return;
			}
		}
	}
	if (!config_is_initialized())
		config_set_default_values();
	//g_key_file_set_value (kf, "Preferences", "version", PACKAGE_VERSION);
	g_key_file_set_integer(kf, "Preferences", "xres", config_get_xres());
	g_key_file_set_integer(kf, "Preferences", "yres", config_get_yres());
	g_key_file_set_integer(kf, "Preferences", "teff", config_get_teff());
	g_key_file_set_integer(kf, "Preferences", "tcol", config_get_tcol());
	g_key_file_set_integer(kf, "Preferences", "sres", config_get_sres());
	g_key_file_set_integer(kf, "Preferences", "fps", config_get_fps());
	//g_key_file_set_boolean (kf, "Preferences", "show_title", config_get_show_title());
	preferences = g_key_file_to_data(kf, &length, NULL);
	ok = g_file_set_contents(config_file_path, preferences, length, NULL);
	if (!ok)
		g_warning(_("Error saving preferences"));
	g_free(preferences);
	g_free(config_file_path);
	g_free(config_dir_path);
	g_key_file_free(kf);
}

void config_set_default_values(void)
{
	config_set_x(20);
	config_set_y(10);
	config_set_xres(DEFAULT_WIDTH);
	config_set_yres(DEFAULT_HEIGHT);
	config_set_teff(DEFAULT_TIME_EFFECT);
	config_set_tcol(DEFAULT_TIME_PALETTE);
	config_set_sres(DEFAULT_SCALE_FACTOR);
	config_set_fps(DEFAULT_FPS);
	config_set_show_title(DEFAULT_SHOW_TITLE);
}


gboolean config_is_initialized(void)
{
	/* FIXME mhm, ugly thing... */
	//return ((config.x > -1) && (config.y > -1) &&
	return (config.xres != 0) && (config.yres != 0) &&
	       (config.sres != 0) && (config.teff != 0) &&
	       (config.tcol != 0) && (config.fps != 0);
}


void config_plugin_config_window(void)
{
#if ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	if (configure_dialog) {
		set_config_values(configure_dialog);
		config_dialog_show(configure_dialog);
		return;
	}

	config_plugin_load_prefs();
	if (!config_is_initialized())
		config_set_default_values();

	configure_dialog = config_dialog_new();

	set_config_values(configure_dialog);

	connect_callbacks(configure_dialog);

	config_dialog_show(configure_dialog);
}

void config_set_x(gint32 value)
{
	config.x = value;
}

void config_set_y(gint32 value)
{
	config.y = value;
}

void config_set_xres(gint32 value)
{
	config.xres = value;
}

void config_set_yres(gint32 value)
{
	config.yres = value;
}

void config_set_teff(gint32 value)
{
	config.teff = value;
}

void config_set_tcol(gint32 value)
{
	config.tcol = value;
}

void config_set_sres(gint32 value)
{
	config.sres = value;
}

void config_set_fps(gint32 value)
{
	config.fps = value;
}

void config_set_show_title(gboolean value)
{
	config.show_title = value;
}

gint32 config_get_x(void)
{
	return config.x;
}

gint32 config_get_y(void)
{
	return config.y;
}

gint32 config_get_xres(void)
{
	return config.xres;
}

gint32 config_get_yres(void)
{
	return config.yres;
}

gint32 config_get_teff(void)
{
	return config.teff;
}

gint32 config_get_tcol(void)
{
	return config.tcol;
}

gint32 config_get_sres(void)
{
	return config.sres;
}

gint32 config_get_fps(void)
{
	return config.fps;
}

gboolean config_get_show_title(void)
{
	return config.show_title;
}
