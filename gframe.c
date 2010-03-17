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

#define f_menu_append_from_stock(stock,callback,arg) do { \
		menuitem = gtk_image_menu_item_new_from_stock(stock, NULL); \
		g_signal_connect(menuitem, "activate", \
			G_CALLBACK(callback), arg); \
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem); \
		gtk_widget_show(menuitem); \
	} while (0);

static void 	callback_destroy	(GtkWidget *widget, gpointer data);
static gint	callback_open		(GtkWidget *widget, GtkWidget *image);
static gint	callback_button		(GtkWidget *widget, GdkEvent *event);
static void 	callback_move		(GtkWidget *widget,
						GdkEventConfigure *event);
static gchar *	f_get_config_path	(void);
static gchar *	f_get_photo_path	(void);
static gchar *	f_get_photo_path_from_dialog (void);
static GdkPixbuf *f_get_pixbuf_at_scale	(gchar *path, gint size);
static GtkWidget *f_get_main_window	(void);
static gboolean	f_set_config		(gchar *path, gchar *group, gchar *key,
						gchar *content);
static void	f_set_position		(gboolean same);

static gint wx, wy;

int
main(int argc, char **argv) {
	GtkWidget *menuitem; /* used by f_menu_append_from_stock() */
	gchar *path = f_get_photo_path();

	g_return_val_if_fail((path != NULL), 1);

	gtk_init(&argc, &argv);

	GtkWidget *event = gtk_event_box_new();
	GdkPixbuf *pixbuf = f_get_pixbuf_at_scale(path, 300);
	GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
	GtkWidget *window = f_get_main_window();
	GtkWidget *menu = gtk_menu_new();

	g_object_unref(pixbuf);

	f_menu_append_from_stock(GTK_STOCK_OPEN, callback_open, image);
	f_menu_append_from_stock(GTK_STOCK_QUIT, callback_destroy, NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	gtk_container_add(GTK_CONTAINER(event), image);
	gtk_container_add(GTK_CONTAINER(window), event);

	g_signal_connect_swapped(window, "button_press_event",
		G_CALLBACK(callback_button), menu);
	g_signal_connect(window, "destroy", G_CALLBACK(callback_destroy), NULL);
	g_signal_connect(window, "configure-event",
		G_CALLBACK(callback_move), NULL);

	f_set_position(FALSE);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}

static gint
callback_button(GtkWidget *widget, GdkEvent *event) {
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

static gint
callback_open(GtkWidget *widget, GtkWidget *image) {
	GtkWidget *window = f_get_main_window();
	gchar *filename = f_get_photo_path_from_dialog();
	gchar *path = f_get_config_path();

	if (!filename)
		return FALSE;

	GdkPixbuf *pixbuf = f_get_pixbuf_at_scale(filename, 300);

	f_print("File: %s", filename);
	f_set_config(path, "preferences", "photo_path", filename);

	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
	g_object_unref(pixbuf);

	if (window) {
		gtk_widget_hide_all(window);
		gtk_window_resize(GTK_WINDOW(window),
			gdk_pixbuf_get_width(pixbuf),
			gdk_pixbuf_get_height(pixbuf));
		gtk_widget_show_all(window);
	}
	f_set_position(TRUE);
	return TRUE;
}

static void
callback_move(GtkWidget *widget, GdkEventConfigure *event) {
	wx = event->x;
	wy = event->y;
}

static void
callback_destroy(GtkWidget *widget, gpointer data) {
	GKeyFile *keyfile = g_key_file_new();
	gchar *config_path = f_get_config_path();
	FILE *fd;

	g_key_file_load_from_file(keyfile, config_path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_integer(keyfile, "preferences", "x", wx);
	g_key_file_set_integer(keyfile, "preferences", "y", wy);

	if ((fd = fopen(config_path, "w")) != NULL) {
		fputs(g_key_file_to_data(keyfile, NULL, NULL), fd);
		fclose(fd);
	}
	g_key_file_free(keyfile);
	gtk_main_quit();
}

static gchar *
f_get_photo_path_from_dialog(void) {
	gchar *filename = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Choose File", NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	gtk_widget_destroy(dialog);
	return filename;
}

static GdkPixbuf *
f_get_pixbuf_at_scale(gchar *path, gint scale) {
	GdkPixbuf *pixbuf;
	gint width, height;

	gdk_pixbuf_get_file_info(path, &width, &height);

	if (width > 300 || height > 300) {
		if ((width - height) >= 0)
			pixbuf = gdk_pixbuf_new_from_file_at_scale(path,
				scale, -1, TRUE, NULL);
		else if ((width - height) < 0)
			pixbuf = gdk_pixbuf_new_from_file_at_scale(path,
				-1, scale, TRUE, NULL);
	} else if (width > 0 && height > 0)
		pixbuf = gdk_pixbuf_new_from_file(path, NULL);
	else
		return NULL;
	return pixbuf;
}

static gchar *
f_get_photo_path(void) {
	GKeyFile *keyfile = g_key_file_new();
	gchar *config_path = f_get_config_path();
	gchar *photo_path = NULL;

	f_print("Config path: %s", config_path);

	if (g_file_test(config_path, G_FILE_TEST_EXISTS)) {
		if (!g_key_file_load_from_file(keyfile, config_path,
			G_KEY_FILE_NONE, NULL))
			f_print("Error: cannot read from file: %s",
				config_path);
		else
			photo_path = g_key_file_get_string(keyfile,
				"preferences", "photo_path", NULL);
	} else {
		photo_path = f_get_photo_path_from_dialog();
		if (!photo_path) {
			g_key_file_free(keyfile);
			return NULL;
		}
		f_set_config(config_path, "preferences",
			"photo_path", photo_path);
	}
	g_key_file_free(keyfile);
	return photo_path;
}

static gchar *
f_get_config_path(void) {
	static gchar *path = NULL;

	if (path == NULL)
		path = g_build_filename(g_get_user_config_dir(),
			"frame.conf", NULL);
	return path;
}

static GtkWidget *
f_get_main_window(void) {
	static GtkWindow *window = NULL;

	if (window == NULL)  {
		window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));

		gtk_window_set_decorated(window, FALSE);
		gtk_window_set_skip_pager_hint(window, TRUE);
		gtk_window_set_skip_taskbar_hint(window, TRUE);
		gtk_window_set_keep_below(window, TRUE);
		gtk_window_stick(window);
		gtk_window_set_title(window, "gframe");
	}
	return GTK_WIDGET(window);
}

static gboolean
f_set_config(gchar *path, gchar *group, gchar *key, gchar *content) {
	GKeyFile *keyfile = g_key_file_new();
	FILE *fd;
	gint ret = FALSE;

	g_key_file_load_from_file(keyfile, path, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(keyfile, group, key, content);

	if ((fd = fopen(path, "w")) != NULL) {
		fputs(g_key_file_to_data(keyfile, NULL, NULL), fd);
		fclose(fd);
		ret = TRUE;
	} else
		f_print("Error: cannot write to file: %s.", path);
	g_key_file_free(keyfile);
	return ret;
}

static void
f_set_position(gboolean same) {
	GtkWindow *window = GTK_WINDOW(f_get_main_window());
	GKeyFile *keyfile = g_key_file_new();
	gchar *config_path = f_get_config_path();

	if (!same) {
		if (!g_key_file_load_from_file(keyfile, config_path,
			G_KEY_FILE_NONE, NULL)) {
			f_print("Error: cannot read from file: %s", config_path);
		} else {
			wx = g_key_file_get_integer(keyfile,
				"preferences", "x", NULL);
			wy = g_key_file_get_integer(keyfile,
				"preferences", "y", NULL);
		}
	} else {
		gtk_window_get_position(window, &wx, &wy);
	}
	gtk_window_move(window, wx, wy);
}
