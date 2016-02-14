#include "config.h"
#include "config-dialog.h"
#include "infconfig.h"
#include "gettext.h"


ConfigDialog *config_dialog_new(void)
{
	ConfigDialog *config_win;
	GtkTooltips *tooltips;

	GtkWidget *window_main;
	GtkWidget *vbox_main;
	GtkWidget *label_scale_factor;
	GtkWidget *hscale_scale_factor;
	GtkWidget *frame_general;
	GtkWidget *table_general;
	GtkWidget *label_effect;
	GtkWidget *label_palette;
	GtkWidget *label_fps;
	GtkWidget *checkbutton_show_title;
	GtkWidget *hscale_effect;
	GtkWidget *hscale_palette;
	GtkObject *spinbutton_fps_adj;
	GtkWidget *spinbutton_fps;
	GtkWidget *hbuttonbox_main;
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
	GtkWidget *button_apply;

#if ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	window_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_object_set_data(GTK_OBJECT(window_main), "window_main", window_main);
	gtk_window_set_title(GTK_WINDOW(window_main), _("Infinity Configuration"));
	gtk_window_set_type_hint(GTK_WINDOW(window_main), GDK_WINDOW_TYPE_HINT_DIALOG);

	vbox_main = gtk_vbox_new(FALSE, 0);
	gtk_widget_ref(vbox_main);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "vbox_main", vbox_main,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(vbox_main);
	gtk_container_add(GTK_CONTAINER(window_main), vbox_main);
	gtk_container_set_border_width(GTK_CONTAINER(vbox_main), 2);

	frame_general = gtk_frame_new(_("Parameters"));
	gtk_widget_ref(frame_general);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "frame_general", frame_general,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(frame_general);
	gtk_box_pack_start(GTK_BOX(vbox_main), frame_general, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame_general), 2);

	table_general = gtk_table_new(5, 3, FALSE);
	gtk_widget_ref(table_general);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "table_general", table_general,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(table_general);
	gtk_container_add(GTK_CONTAINER(frame_general), table_general);
	gtk_container_set_border_width(GTK_CONTAINER(table_general), 2);
	gtk_table_set_row_spacings(GTK_TABLE(table_general), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table_general), 2);

	label_effect = gtk_label_new(_("Effect period:"));
	gtk_widget_ref(label_effect);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "label_effect", label_effect,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(label_effect);
	gtk_table_attach(GTK_TABLE(table_general), label_effect, 0, 1, 0, 1,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_effect), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label_effect), 1, 0);

	label_palette = gtk_label_new(_("Palette period:"));
	gtk_widget_ref(label_palette);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "label_palette", label_palette,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(label_palette);
	gtk_table_attach(GTK_TABLE(table_general), label_palette, 0, 1, 1, 2,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_palette), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label_palette), 1, 0);

	label_scale_factor = gtk_label_new(_("Scale Factor"));
	gtk_widget_ref(label_scale_factor);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "label_scale_factor", label_scale_factor,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(label_scale_factor);
	gtk_table_attach(GTK_TABLE(table_general), label_scale_factor, 0, 1, 2, 3,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_scale_factor), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label_scale_factor), 1, 0);

	label_fps = gtk_label_new(_("Maximum Frames per Second:"));
	gtk_widget_ref(label_fps);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "label_fps", label_fps,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(label_fps);
	gtk_table_attach(GTK_TABLE(table_general), label_fps, 0, 1, 3, 4,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label_fps), 0, 0.5);
	gtk_misc_set_padding(GTK_MISC(label_fps), 1, 0);

	checkbutton_show_title = gtk_check_button_new_with_label(_("Show current play title"));
	gtk_widget_ref(checkbutton_show_title);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "checkbutton_show_title", checkbutton_show_title,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(checkbutton_show_title);
	gtk_table_attach(GTK_TABLE(table_general), checkbutton_show_title, 0, 1, 4, 5,
			 (GtkAttachOptions)(GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);

	tooltips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tooltips, checkbutton_show_title,
			     _("Check it if you want to see the current play "
			       "title on the Infinity's title bar"), NULL);

	//spinbutton_fps_adj = gtk_adjustment_new (30, 5, 150, 1, 10, 0);
	spinbutton_fps_adj = gtk_adjustment_new(config_get_fps(), 5, 150, 1, 10, 0);
	spinbutton_fps = gtk_spin_button_new(GTK_ADJUSTMENT(spinbutton_fps_adj), 1, 0);
	gtk_widget_ref(spinbutton_fps);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "spinbutton_fps", spinbutton_fps,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(spinbutton_fps);
	gtk_table_attach(GTK_TABLE(table_general), spinbutton_fps, 2, 3, 3, 4,
			 (GtkAttachOptions)(0),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spinbutton_fps), GTK_UPDATE_IF_VALID);

	hscale_effect = gtk_hscale_new(GTK_ADJUSTMENT(gtk_adjustment_new(100, 50, 400, 0, 0, 0)));
	gtk_widget_ref(hscale_effect);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "hscale_effect", hscale_effect,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(hscale_effect);
	gtk_table_attach(GTK_TABLE(table_general), hscale_effect, 1, 3, 0, 1,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(hscale_effect), GTK_POS_LEFT);
	gtk_scale_set_digits(GTK_SCALE(hscale_effect), 0);

	gtk_tooltips_set_tip(tooltips, hscale_effect,
			     _("Time to change between an effect and the "
			       "next one, in milliseconds."), NULL);

	hscale_palette = gtk_hscale_new(GTK_ADJUSTMENT(gtk_adjustment_new(100, 50, 400, 0, 0, 0)));
	gtk_widget_ref(hscale_palette);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "hscale_palette", hscale_palette,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(hscale_palette);
	gtk_table_attach(GTK_TABLE(table_general), hscale_palette, 1, 3, 1, 2,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(hscale_palette), GTK_POS_LEFT);
	gtk_scale_set_digits(GTK_SCALE(hscale_palette), 0);

	gtk_tooltips_set_tip(tooltips, hscale_palette,
			     _("Time to change between a color and the "
			       "next one, in milliseconds."), NULL);

	hscale_scale_factor = gtk_hscale_new(GTK_ADJUSTMENT(gtk_adjustment_new(1, 1, 2, 0, 0, 0)));
	gtk_widget_ref(hscale_scale_factor);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "hscale_scale_factor", hscale_scale_factor,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(hscale_scale_factor);
	gtk_table_attach(GTK_TABLE(table_general), hscale_scale_factor, 1, 3, 2, 3,
			 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions)(0), 0, 0);
	gtk_scale_set_value_pos(GTK_SCALE(hscale_scale_factor), GTK_POS_LEFT);
	gtk_scale_set_digits(GTK_SCALE(hscale_scale_factor), 0);

	gtk_tooltips_set_tip(tooltips, hscale_scale_factor,
			     _("Change only if the system doesn't have "
			       "enough power to show the screen at "
			       "a reasonably speed/size relation."), NULL);

	hbuttonbox_main = gtk_hbutton_box_new();
	gtk_widget_ref(hbuttonbox_main);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "hbuttonbox_main", hbuttonbox_main,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(hbuttonbox_main);
	gtk_box_pack_start(GTK_BOX(vbox_main), hbuttonbox_main, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbuttonbox_main), 2);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox_main), GTK_BUTTONBOX_END);

	button_ok = gtk_button_new_with_label(_("Ok"));
	gtk_widget_ref(button_ok);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "button_ok", button_ok,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(button_ok);
	gtk_container_add(GTK_CONTAINER(hbuttonbox_main), button_ok);
	GTK_WIDGET_SET_FLAGS(button_ok, GTK_CAN_DEFAULT);

	gtk_tooltips_set_tip(tooltips, button_ok,
			     _("For to toggle to fullscreen mode, select "
			       "the Infinity plugin window, and press "
			       "the Tab key. Pressing Tab again will return "
			       "to window mode."), NULL);

	button_cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_widget_ref(button_cancel);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "button_cancel", button_cancel,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(button_cancel);
	gtk_container_add(GTK_CONTAINER(hbuttonbox_main), button_cancel);
	GTK_WIDGET_SET_FLAGS(button_cancel, GTK_CAN_DEFAULT);

	button_apply = gtk_button_new_with_label(_("Apply"));
	gtk_widget_ref(button_apply);
	gtk_object_set_data_full(GTK_OBJECT(window_main), "button_apply", button_apply,
				 (GtkDestroyNotify)gtk_widget_unref);
	gtk_widget_show(button_apply);
	gtk_container_add(GTK_CONTAINER(hbuttonbox_main), button_apply);
	GTK_WIDGET_SET_FLAGS(button_apply, GTK_CAN_DEFAULT);

	gtk_widget_grab_default(button_cancel);

	config_win = g_new0(ConfigDialog, 1);

	config_win->window_main = window_main;

	config_win->hscale_effect = hscale_effect;
	config_win->hscale_palette = hscale_palette;
	config_win->hscale_scale_factor = hscale_scale_factor;
	config_win->spinbutton_fps = spinbutton_fps;
	config_win->checkbutton_show_title = checkbutton_show_title;

	config_win->button_ok = button_ok;
	config_win->button_cancel = button_cancel;
	config_win->button_apply = button_apply;

	gtk_tooltips_enable(tooltips);

	return config_win;
}

void config_dialog_destroy(ConfigDialog *dialog)
{
	g_return_if_fail(dialog != NULL);

	g_free(dialog);
}

void config_dialog_show(ConfigDialog *dialog)
{
	g_return_if_fail(dialog != NULL);

	gtk_window_present(GTK_WINDOW(dialog->window_main));
}

void config_dialog_hide(ConfigDialog *dialog)
{
	g_return_if_fail(dialog != NULL);

	gtk_widget_hide(dialog->window_main);
}
