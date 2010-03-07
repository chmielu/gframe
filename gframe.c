/*
 *	gframe.c - Simple photo frame for your gnome desktop!
 *
 *	Copyright 2010 Robert Chmielowiec <robert@chmielowiec.net>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *	MA 02110-1301, USA.
 *
 *	Compile with:
 * 	cc `pkg-config --cflags --libs gtk+-2.0` gframe.c -o gframe
 *
 */

#include <gtk/gtk.h>

#ifdef DEBUG
# define f_print(frm, ...) g_print(frm "\n", __VA_ARGS__)
#else
# define f_print(frm, ...)
#endif /* DEBUG */

static void 	callback_destroy	(GtkWidget *widget, gpointer data);
static gint	callback_preferences	(GtkWidget *widget, GtkWidget *image);
static gint	callback_button		(GtkWidget *widget, GdkEvent *event);
static gchar *	f_get_config_path	(void);
static gchar *	f_get_photo_path	(void);
static gchar *	f_get_photo_path_from_dialog (void);
static gboolean	f_set_config		(gchar *path, gchar *content);

int main(int argc, char **argv) {
	GtkWidget *window, *image, *event, *menu, *menuitem;
	gchar *path = NULL;

	gtk_init(&argc, &argv);

	path = f_get_photo_path();
	g_return_val_if_fail((path != NULL), 1);

	f_print("File: %s", path);

	event = gtk_event_box_new();
	image = gtk_image_new_from_file(path);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_free(path);

	menu = gtk_menu_new();

	/* change image button */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(menuitem, "activate",
		G_CALLBACK(callback_preferences), image);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);

	/* quit button */
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	g_signal_connect(menuitem, "activate",
		G_CALLBACK(callback_destroy), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);

	gtk_container_add(GTK_CONTAINER(event), image);

	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
	gtk_window_set_keep_below(GTK_WINDOW(window), TRUE);
	gtk_window_stick(GTK_WINDOW(window));

	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	gtk_container_add(GTK_CONTAINER(window), event);

	g_signal_connect_swapped(window, "button_press_event",
		G_CALLBACK(callback_button), menu);

	g_signal_connect(G_OBJECT(window), "destroy",
		G_CALLBACK(callback_destroy), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

static gint callback_button(GtkWidget *widget, GdkEvent *event) {
	GtkMenu *menu;
	GdkEventButton *button;

	g_return_val_if_fail(event != NULL, FALSE);

	menu = GTK_MENU(widget);
	if (event->type == GDK_BUTTON_PRESS) {
		button = (GdkEventButton *)event;
		if (button->button == 3) {
			gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
				button->button, button->time);
			return TRUE;
		}
	}
	return FALSE;
}

static gint callback_preferences(GtkWidget *widget, GtkWidget *image) {
	gchar *filename = f_get_photo_path_from_dialog();
	gchar *path;

	if (!filename)
		return FALSE;

	gtk_image_clear(GTK_IMAGE(image));
	gtk_image_set_from_file(GTK_IMAGE(image), filename);

	f_print("File: %s", filename);

	path = f_get_config_path();
	f_set_config(path, filename);

	return TRUE;
}

static void callback_destroy(GtkWidget *widget, gpointer data) {
	gtk_main_quit();
}

static gchar *f_get_photo_path_from_dialog(void) {
	gchar *filename = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Choose File",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	gtk_widget_destroy(dialog);
	return filename;
}

static gchar *f_get_photo_path(void) {
	gchar *config_path = f_get_config_path();
	gchar *config, *filename = NULL;

	f_print("Config path: %s", config_path);

	if (g_file_test(config_path, G_FILE_TEST_EXISTS)) {
		if (!g_file_get_contents(config_path, &config, NULL, NULL)) {
			f_print("Error: cannot read from file: %s",
				config_path);
		} else {
			f_print("Config: %s", config);

			if (g_path_is_absolute(config)) {
				filename = g_strdup(config);
				g_free(config);
			}
		}
	} else {
		config = f_get_photo_path_from_dialog();
		if (!config)
			return NULL;

		filename = g_strdup(config);
		f_set_config(config_path, filename);
	}
	return filename;
}

static gboolean f_set_config(gchar *path, gchar *content) {
	if (!g_file_set_contents(path, content, -1, NULL)) {
		f_print("Error: cannot write to file: %s.", path);
		return FALSE;
	}
	return TRUE;
}

static gchar *f_get_config_path(void) {
	static gchar *path = NULL;

	if (path == NULL)
		path = g_build_filename(g_get_user_config_dir(),
			"frame.conf", NULL);
	return path;
}
