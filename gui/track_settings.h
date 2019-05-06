#ifndef TRACK_SETTINGS_H
#define TRACK_SETTINGS_H

#include <gtkmm.h>

#include "engine/song.h"

class AddEffectDialog : public Gtk::MessageDialog {

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

	Gtk::TreeView tree;
	Gtk::Label description_label;
	Gtk::TextView description;
	Glib::RefPtr<Gtk::TextBuffer> description_text;

	void _selection_changed();
	void _activated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *p_column);

public:
	void update_effect_list();
	int get_selected_effect_index();
	AddEffectDialog(AudioEffectFactory *p_fx_factory);
};

#endif // TRACK_SETTINGS_H
