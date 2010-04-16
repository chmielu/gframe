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
 */

#include <gtk/gtk.h>
#include <signal.h>

#ifdef DEBUG
# define f_print(frm, ...) g_print (frm "\n", __VA_ARGS__)
#else
# define f_print(frm, ...)
#endif /* DEBUG */

#define f_menu_append_from_stock(stock,cb,arg) do { \
		menuitem = gtk_image_menu_item_new_from_stock (stock, NULL); \
		g_signal_connect (menuitem, "activate", G_CALLBACK (cb), arg); \
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem); \
		gtk_widget_show (menuitem); \
	} while (0)
#define f_menu_append_separator() do { \
		menuitem = gtk_separator_menu_item_new (); \
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem); \
		gtk_widget_show (menuitem); \
	} while (0)

/* this is so stupid, but without setting hints to FALSE it doesn't work :-( */
#define f_window_set_skip_hint(what,widget) do { \
		gtk_window_set_skip_##what##_hint (GTK_WINDOW (widget), FALSE); \
		gtk_window_set_skip_##what##_hint (GTK_WINDOW (widget), TRUE); \
	} while (0)

#define f_window_get_image(wid) \
	gtk_bin_get_child (GTK_BIN (gtk_bin_get_child (GTK_BIN (wid))))

enum { CONFIG_STRING, CONFIG_INT, CONFIG_LAST }; /* f_{set,get}_config type */

static gint	callback_destroy	(GtkWidget *widget);
static gint	callback_open		(GtkWidget *widget, GtkWidget *image);
static gint	callback_button		(GtkWidget *widget, GdkEvent *event);
static gint	callback_pref		(GtkWidget *widget, GtkWidget *window);
static void 	callback_move		(GtkWidget *widget, GdkEventConfigure *event);
static gpointer	f_get_config		(gint type, gchar *group, gchar *key);
static gchar *	f_get_config_path	(void);
static gchar *	f_get_photo_path	(void);
static gchar *	f_get_photo_path_from_dialog (void);
static GdkPixbuf *f_get_pixbuf_at_scale	(gchar *path);
static GtkWidget *f_get_main_window	(void);
static void	f_set_window_hints	(GtkWindow *window, gboolean new);
static gboolean	f_set_config		(gint type, gchar *group, gchar *key, gpointer content);
static void	f_set_position		(gboolean same);
static void	f_window_redraw 	(gchar *filename, GtkWidget *image);

static gint wx, wy;

int
main (int argc, char **argv) {
	GtkWidget *menuitem; /* used by f_menu_append_from_stock () */
	gchar *path;

	signal(SIGTERM, (__sighandler_t)callback_destroy);
	signal(SIGINT, (__sighandler_t)callback_destroy);

	gtk_init (&argc, &argv);

	g_return_val_if_fail ((path = f_get_photo_path ()) != NULL, 1);

	GtkWidget *event = gtk_event_box_new ();
	GdkPixbuf *pixbuf = f_get_pixbuf_at_scale (path);
	GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);
	GtkWidget *window = f_get_main_window ();
	GtkWidget *menu = gtk_menu_new ();

	gdk_pixbuf_unref (pixbuf);

	f_menu_append_from_stock (GTK_STOCK_PREFERENCES, callback_pref, window);
	f_menu_append_from_stock (GTK_STOCK_OPEN, callback_open, image);
	f_menu_append_separator ();
	f_menu_append_from_stock (GTK_STOCK_QUIT, callback_destroy, NULL);

	gtk_container_set_border_width (GTK_CONTAINER (window), 5);
	gtk_container_add (GTK_CONTAINER (event), image);
	gtk_container_add (GTK_CONTAINER (window), event);

	g_signal_connect_swapped (window, "button_press_event", G_CALLBACK (callback_button), menu);
	g_signal_connect (window, "destroy", G_CALLBACK (callback_destroy), NULL);
	g_signal_connect (window, "configure-event", G_CALLBACK (callback_move), NULL);

	f_set_position (FALSE);

	gtk_widget_show_all (window);
	gtk_main ();

	return 0;
}

static gint
callback_button (GtkWidget *widget, GdkEvent *event) {
	GtkMenu *menu;
	GdkEventButton *button;

	g_return_val_if_fail (event != NULL, FALSE);

	menu = GTK_MENU (widget);
	if (event->type == GDK_BUTTON_PRESS) {
		button = (GdkEventButton *)event;
		if (button->button == 3) {
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
				button->button, button->time);
			return TRUE;
		}
	}
	return FALSE;
}

static gint
callback_open (GtkWidget *widget G_GNUC_UNUSED, GtkWidget *image) {
	gchar *filename = f_get_photo_path_from_dialog ();

	g_return_val_if_fail ((filename != NULL), FALSE);

	f_print ("File: %s", filename);
	f_set_config (CONFIG_STRING, "preferences", "photo_path", filename);
	f_window_redraw (filename, image);
	f_set_position (TRUE);
	return TRUE;
}

static gint
callback_pref (GtkWidget *widget G_GNUC_UNUSED, GtkWidget *window) {
	GtkWidget *dialog, *content_area, *label, *spin_button, *hbox;
	gint max_size = 0;

	dialog = gtk_dialog_new_with_buttons ("Settings",
		GTK_WINDOW (window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	hbox = gtk_hbox_new (TRUE, 0);
	label = gtk_label_new ("Maximum size:");
	spin_button = gtk_spin_button_new_with_range (0, 3000, 1);

	max_size = (gint)f_get_config (CONFIG_INT, "preferences", "max_size");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button), max_size);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	gtk_container_add (GTK_CONTAINER (hbox), label);
	gtk_container_add (GTK_CONTAINER (hbox), spin_button);
	gtk_widget_show_all (hbox);
	gtk_container_add (GTK_CONTAINER (content_area), hbox);

	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result) {
		case GTK_RESPONSE_OK:
			max_size = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (spin_button));
			f_set_config (CONFIG_INT, "preferences", "max_size", &max_size);
			f_window_redraw(
				f_get_config (CONFIG_STRING, "preferences", "photo_path"),
				f_window_get_image (window));
			f_set_position (TRUE);
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
	return TRUE;
}

static void
callback_move (GtkWidget *widget G_GNUC_UNUSED, GdkEventConfigure *event) {
	wx = event->x;
	wy = event->y;
}

static gint
callback_destroy (GtkWidget *widget G_GNUC_UNUSED) {
	f_set_config (CONFIG_INT, "preferences", "x", &wx);
	f_set_config (CONFIG_INT, "preferences", "y", &wy);

	gtk_main_quit ();
	return 0;
}

static gchar *
f_get_photo_path_from_dialog (void) {
	gchar *filename = NULL;

	GtkWidget *dialog = gtk_file_chooser_dialog_new ("Choose File", NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

	gtk_widget_destroy (dialog);
	return filename;
}

static GdkPixbuf *
f_get_pixbuf_at_scale (gchar *path) {
	GdkPixbuf *pixbuf;
	gint width, height, max_size;

	max_size = (gint)f_get_config (CONFIG_INT, "preferences", "max_size");
	if (max_size < 0)
		max_size = 300;
	gdk_pixbuf_get_file_info (path, &width, &height);

	if (width > max_size || height > max_size) {
		if ((width - height) >= 0)
			pixbuf = gdk_pixbuf_new_from_file_at_scale (path,
				max_size, -1, TRUE, NULL);
		else if ((width - height) < 0)
			pixbuf = gdk_pixbuf_new_from_file_at_scale (path,
				-1, max_size, TRUE, NULL);
	} else if (width > 0 && height > 0)
		pixbuf = gdk_pixbuf_new_from_file (path, NULL);
	else
		return NULL;
	return pixbuf;
}

static gchar *
f_get_photo_path (void) {
	GKeyFile *keyfile = g_key_file_new ();
	gchar *photo_path = NULL;

	if (g_file_test (f_get_config_path (), G_FILE_TEST_EXISTS))
		photo_path = f_get_config (CONFIG_STRING, "preferences", "photo_path");
	else {
		photo_path = f_get_photo_path_from_dialog ();
		if (!photo_path) {
			g_key_file_free (keyfile);
			return NULL;
		}
		f_set_config (CONFIG_STRING, "preferences", "photo_path", photo_path);
	}
	g_key_file_free (keyfile);
	return photo_path;
}

static gpointer
f_get_config (gint type, gchar *group, gchar *key) {
	GKeyFile *keyfile = g_key_file_new ();
	gchar *path = f_get_config_path ();
	gpointer result = NULL;

	if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL)) {
		f_print ("Error: can't read from file: %s", path);
		goto err;
	}
	switch (type) {
		case CONFIG_STRING:
			result = g_key_file_get_string (keyfile, group, key, NULL);
			break;
		case CONFIG_INT:
			result = (gint *)g_key_file_get_integer (keyfile, group, key, NULL);
			break;
	}
err:
	g_key_file_free (keyfile);
	return result;
}

static gchar *
f_get_config_path (void) {
	static gchar *path = NULL;

	if (path == NULL)
		path = g_build_filename (g_get_user_config_dir (),
			"gframe.conf", NULL);
	return path;
}

static GtkWidget *
f_get_main_window (void) {
	static GtkWidget *window = NULL;

	if (window == NULL)  {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		f_set_window_hints (GTK_WINDOW (window), TRUE);
	}
	return window;
}

static void
f_set_window_hints (GtkWindow *window, gboolean new) {
	if (new) {
		gtk_window_set_decorated (window, FALSE);
		gtk_window_set_title (window, "gframe");
	}
	f_window_set_skip_hint (pager, window);
	f_window_set_skip_hint (taskbar, window);
	gtk_window_stick (window);
	gtk_window_set_keep_below (window, TRUE);
}

static gboolean
f_set_config (gint type, gchar *group, gchar *key, gpointer content) {
	FILE *fd;
	GKeyFile *keyfile = g_key_file_new ();
	gchar *path = f_get_config_path ();
	gint ret = FALSE;

	g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL);

	switch (type) {
		case CONFIG_STRING:
			g_key_file_set_string (keyfile, group, key, (gchar *)content);
			break;
		case CONFIG_INT:
			g_key_file_set_integer (keyfile, group, key, *((gint *)content));
			break;
	}

	if ((fd = fopen (path, "w")) != NULL) {
		fputs (g_key_file_to_data (keyfile, NULL, NULL), fd);
		fclose (fd);
		ret = TRUE;
	} else {
		f_print ("Error: cannot write to file: %s.", path);
	}
	g_key_file_free (keyfile);
	return ret;
}

static void
f_set_position (gboolean same) {
	GtkWindow *window = GTK_WINDOW (f_get_main_window ());
	if (!same) {
		wx = (gint)f_get_config (CONFIG_INT, "preferences", "x");
		wy = (gint)f_get_config (CONFIG_INT, "preferences", "y");
	} else
		gtk_window_get_position (window, &wx, &wy);
	gtk_window_move (window, wx, wy);
}

static void
f_window_redraw (gchar *filename, GtkWidget *image) {
	GtkWidget *window = f_get_main_window ();

	GdkPixbuf *pixbuf = f_get_pixbuf_at_scale (filename);
	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
	gdk_pixbuf_unref (pixbuf);

	gtk_widget_hide_all (window);
	gtk_window_resize (GTK_WINDOW (window),
		gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf));
	gtk_widget_show_all (window);
	f_set_window_hints (GTK_WINDOW (window), FALSE);
}
