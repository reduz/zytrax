#ifndef INTERFACE_H
#define INTERFACE_H

#include "engine/song.h"
#include "gui/pattern_editor.h"
#include <gtkmm.h>

class Interface : public Gtk::Window {

	enum {
		FILE_NEW,
		FILE_OPEN,
		FILE_SAVE,
		FILE_SAVE_AS,
		FILE_QUIT,
		SETTINGS_CONFIG,
		SETTINGS_ABOUT

	};
	Gtk::MenuBar menu;
	Gtk::MenuItem file_menu_item;
	Gtk::Menu file_menu;
	Gtk::Menu settings_menu;

	Vector<Gtk::MenuItem *> menu_items;

	/* Boxes */
	Gtk::VBox main_vbox;
	Gtk::HBox top_hbox;
	Gtk::HBox main_hbox;
	/* Labels */
	Gtk::Label pattern_label;
	Gtk::Label octave_label;
	Gtk::Label volume_label;
	Gtk::Label tempo_label;

	Gtk::SpinButton pattern;
	Gtk::SpinButton octave;
	Gtk::SpinButton volume;
	Gtk::SpinButton tempo;

	/* Editors */

	UndoRedo undo_redo;
	Song song;

	Theme theme;
	KeyBindings key_bindings;
	PatternEditor pattern_editor;

	/* Data */

public:
	Interface();
	~Interface();
};

#endif // INTERFACE_H
