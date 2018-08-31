#ifndef TRACK_SETTINGS_H
#define TRACK_SETTINGS_H

#include <gtkmm.h>

#include "engine/song.h"
class AddEffectDialog : public Gtk::Window {

	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(provider);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> provider;
		Gtk::TreeModelColumn<int> index;
	};

	ModelColumns model_columns;

	AudioEffectFactory *fx_factory;
	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	Gtk::ScrolledWindow scroll;
	Gtk::VBox vbox;
	Gtk::Label effect_label;
	Gtk::TreeView tree;
	Gtk::Label description_label;
	Gtk::TextView description;
	Glib::RefPtr<Gtk::TextBuffer> description_text;
	Gtk::HBox add_button_hb;
	Gtk::Button add_button;
	Gtk::Label empty[2];

	void _selection_changed();
	void _activated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *p_column);
	void _add_effect_pressed();

public:
	void update_effect_list();
	AddEffectDialog(AudioEffectFactory *p_fx_factory);
};

class TrackSettings : public Gtk::Window {

	Gtk::HBox main_hbox;
	Gtk::VBox left_vbox;
	Gtk::Label track_list_label;
	Gtk::ComboBox track_list;
	Gtk::Label rack_label;
	Gtk::ListBox rack;

	Gtk::HBox rack_menu_hb;
	Gtk::Button add_effect;
	Gtk::Button remove_effect;
	Gtk::Button move_effect_up;
	Gtk::Button move_effect_down;

	Gtk::ListBox automations;
	Gtk::VBox right_vbox;

	AddEffectDialog add_effect_dialog;

	AudioEffectFactory *fx_factory;

	void _add_effect();

public:
	TrackSettings(AudioEffectFactory *p_fx_factory);
};

#endif // TRACK_SETTINGS_H
