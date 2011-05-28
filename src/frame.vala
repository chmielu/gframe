/*
 * frame.vala
 *
 * Robert Chmielowiec 2011
 * robert@chmielowiec.net
 */


using Gtk;

class Frame : Window {

	private static Image image;
	private static Settings settings;

	construct {
		// set default settings
		title = "GFrame";
		decorated = false;

		// for dragging
		events |= Gdk.EventMask.BUTTON_PRESS_MASK;

		// hide the window in pager & taskbar
		skip_pager_hint = true;
		skip_taskbar_hint = true;

		// init the settings
		settings = new Settings ();

		set_default_size (settings.size, settings.size);
		border_width = settings.border;

		// move to the saved position
		move (settings.x, settings.y);

		// keep the frame below other windows
		set_keep_below (true);

		// make the window visible on all desktops
		stick ();

		// connect destroy signal
		destroy.connect (Gtk.main_quit);

		// load saved image
		load_image ();
	}

	public override bool button_press_event (Gdk.EventButton e) {
		// move
		if (e.button == 1)
			begin_move_drag ((int) e.button, (int) e.x_root, (int) e.y_root,
			                 e.time);

//~ 		// TODO: resize
//~ 		else if (e.button == 2)
//~ 			begin_resize_drag (Gdk.WindowEdge.SOUTH_EAST, (int) e.button,
//~ 			                   (int) e.x_root, (int) e.y_root, e.time);

		// settings
		else if (e.button == 3)
			show_dialog ();

		return true;
	}

	public override bool enter_notify_event (Gdk.EventCrossing e) {
		settings.x = (int) (e.x_root - e.x);
		settings.y = (int) (e.y_root - e.y);
		return false;
	}

	private void load_image () {
		Gdk.Pixbuf pixbuf;
		int width, height, size = settings.size;
		var file = settings.path;

		if (file == null || file == "")
			return;

		// get the image width and height
		Gdk.Pixbuf.get_file_info (file, out width, out height);
		try {
			if (width > size || height > size) {
				// if width is bigger then height scale
				// the image to width = max_size
				if ((width - height >= 0)) {
					pixbuf = new Gdk.Pixbuf.from_file_at_scale (file, size, -1,
					                                            true);
				} else {
					pixbuf = new Gdk.Pixbuf.from_file_at_scale (file, -1, size,
					                                            true);
				}
			// if width and height is smaller then max_size - do not scale
			} else {
				pixbuf = new Gdk.Pixbuf.from_file (file);
			}
		} catch (Error e) {
			error ("Error: %s", e.message);
		}

		// remove the image when updating
		if (get_child () != null) {
			remove (image);
			image.destroy ();
		}

		image = new Image.from_pixbuf (pixbuf);
		image.show ();

		// add scaled image to the window and resize it
		add (image);
		resize (2 * settings.border + pixbuf.width,
		        2 * settings.border + pixbuf.height);
	}

	private void show_dialog () {
		Dialog dialog;
		SpinButton button_size, button_border;
		FileChooserButton button_file;

		// load GUI from the glade file
		try {
			var builder = new Builder ();
			builder.add_from_file ("dialog.ui");

			dialog = builder.get_object ("dialog1") as Dialog;
			button_file = builder.get_object ("filechooserbutton1")
			              as FileChooserButton;
			button_size = builder.get_object ("spinbutton2") as SpinButton;
			button_border = builder.get_object ("spinbutton1") as SpinButton;
		} catch (Error e) {
			error ("Could not load UI: %s", e.message);
		}

		// set buttons values
		button_size.set_range (0, double.MAX);
		button_size.set_increments (1f, 1f);
		button_size.value = settings.size;

		button_border.set_range (0, double.MAX);
		button_border.set_increments (1f, 1f);
		button_border.value = border_width;

		string file = settings.path;
		if (file != null && file != "")
			button_file.set_filename (file);

		dialog.run ();

		// after close save the new settings
		border_width = settings.border = (int) button_border.value;
		settings.size = (int) button_size.value;

		if (button_file.get_filename () != null) {
			settings.path = button_file.get_filename ();
			load_image ();
		}

		settings.save ();
		dialog.destroy ();
	}
}