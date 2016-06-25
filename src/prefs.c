#include <string.h>
#include <glib.h>

#include "config.h"
#include "prefs.h"

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

static t_config config;

void config_save_prefs(void)
{
	// TODO until it gets repaired, plugin forgets its window size when quit
	g_message("TODO config_save_prefs()");
}

gboolean config_is_initialized(void)
{
	return (config.xres != 0) && (config.yres != 0) &&
	       (config.sres != 0) && (config.teff != 0) &&
	       (config.tcol != 0) && (config.fps != 0);
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
