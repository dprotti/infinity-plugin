#ifndef __CONFIG_DIALOG_H__
#define __CONFIG_DIALOG_H__


#include <gtk/gtk.h>


/**
 * Here we have all the widgets of config dialog we need to interact to.
 */
typedef struct {
	GtkWidget *	window_main;

	/* General stuff */
	GtkWidget *	hscale_effect;          /**< GtkHScale */
	GtkWidget *	hscale_palette;         /**< GtkHScale */
	GtkWidget *	hscale_scale_factor;    /**< GtkHScale */
	GtkWidget *	spinbutton_fps;         /**< GtkSpinButton */
	GtkWidget *	checkbutton_show_title; /** GtkCheckButton */

	/* Buttons  */
	GtkWidget *	button_ok;      /**< GtkButton */
	GtkWidget *	button_cancel;  /**< GtkButton */
	GtkWidget *	button_apply;   /**< GtkButton */
} ConfigDialog;


ConfigDialog *config_dialog_new(void);
void config_dialog_destroy(ConfigDialog *dialog);

void config_dialog_show(ConfigDialog *dialog);
void config_dialog_hide(ConfigDialog *dialog);

#endif
