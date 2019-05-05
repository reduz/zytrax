#include "icons.h"

#include "gui_icons.gen.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Gtk::Image create_image_from_icon(const String &p_name) {

	Gtk::Image image;
	for (int i = 0; i < gui_icons_count; i++) {
		if (gui_icons_names[i] == p_name) {
			int w, h, c;

			unsigned char *data = stbi_load_from_memory((const stbi_uc *)gui_icons_sources[i], gui_icons_sizes[i], &w, &h, &c, 4);

			if (data) {
				Glib::RefPtr<Gdk::Pixbuf> ref = Gdk::Pixbuf::create_from_data((const guint8 *)data, Gdk::COLORSPACE_RGB, true, 8, w, h, w * 4);
				image = Gtk::Image(ref);
			}
			break;
		}
	}

	return image;
}
