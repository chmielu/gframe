/*
 * settings.vala
 *
 * Robert Chmielowiec 2011
 * robert@chmielowiec.net
 */

using Gtk;

const string KEYFILENAME = "gframe.conf";

class Settings : GLib.Object {

	private KeyFile keyfile = new KeyFile ();
	private string file;

	public string path	{ set; get; default = null; }
	public int size		{ set; get; default = 300; }
	public int border	{ set; get; default = 0; }
	public int x		{ set; get; default = 0; }
	public int y		{ set; get; default = 0; }

	construct {
		file = Path.build_filename (Environment.get_user_config_dir (),
			KEYFILENAME);

		try {
			keyfile.load_from_file (file, KeyFileFlags.NONE);

			path = keyfile.get_string ("Frame", "path");
			size = keyfile.get_integer ("Frame", "size");
			border = keyfile.get_integer ("Frame", "border");

			x = keyfile.get_integer ("Position", "x");
			y = keyfile.get_integer ("Position", "y");
		} catch (Error e) {
			save ();
		}
	}

	public void save () {
		keyfile.set_string ("Frame", "path", path ?? "");
		keyfile.set_integer ("Frame", "size", size);
		keyfile.set_integer ("Frame", "border", border);

		keyfile.set_integer ("Position", "x", x);
		keyfile.set_integer ("Position", "y", y);

		try {
			FileUtils.set_contents (file, keyfile.to_data ());
		} catch (FileError e) {
			warning ("FileError: %s", e.message);
		}
	}

}