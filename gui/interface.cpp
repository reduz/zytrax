#include "interface.h"

void Interface::_track_edited(int p_track) {
	track_settings.show();
}

Interface::Interface(AudioEffectFactory *p_fx_factory) :
		track_settings(p_fx_factory),
		pattern_editor(&song, &undo_redo, &theme, &key_bindings) {

	fx_factory = p_fx_factory;
	add(main_vbox);
	main_vbox.pack_start(menu, Gtk::PACK_SHRINK);
	menu.show();

	file_menu_item.set_label("File");
	menu.add(file_menu_item);

	file_menu_item.set_submenu(file_menu);

	main_vbox.pack_start(top_hbox, Gtk::PACK_SHRINK);

	pattern_label.set_text("Pattern:");
	top_hbox.add(pattern_label);
	top_hbox.add(pattern);

	octave_label.set_text("Octave:");
	top_hbox.add(octave_label);
	top_hbox.add(octave);

	volume_label.set_text("Volume:");
	top_hbox.add(volume_label);
	top_hbox.add(volume);

	tempo_label.set_text("Tempo:");
	top_hbox.add(tempo_label);
	top_hbox.add(tempo);

	main_vbox.pack_end(main_hbox, Gtk::PACK_EXPAND_WIDGET);

	main_hbox.pack_end(pattern_editor, Gtk::PACK_EXPAND_WIDGET);
	pattern_editor.track_edited.connect(sigc::mem_fun(this, &Interface::_track_edited));

	show_all_children();

	//pattern_editor.init();
}

Interface::~Interface() {

	for (int i = 0; i < menu_items.size(); i++) {
		delete menu_items[i];
	}
}
