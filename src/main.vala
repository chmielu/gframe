/*
 * main.vala
 *
 * Robert Chmielowiec 2011
 * robert@chmielowiec.net
 */


static int main(string[] args) {
	Gtk.init (ref args);

	var frame = new Frame ();
	frame.show ();

	Gtk.main ();

	return 0;
}
