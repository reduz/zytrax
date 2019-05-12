#ifndef EFFECT_EDITOR_H
#define EFFECT_EDITOR_H

#include "engine/song.h"
#include <gtkmm.h>

class EffectEditor : public Gtk::Window {

	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(visible);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<bool> visible;
		Gtk::TreeModelColumn<int> index;
	};

	Gtk::VBox main_vbox;

	Gtk::HPaned split;

	ModelColumns model_columns;

	AudioEffectFactory *fx_factory;
	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	Gtk::ScrolledWindow automation_scroll;

	Gtk::CellRendererToggle cell_render_check;
	Gtk::CellRendererText cell_render_text;
	Gtk::TreeViewColumn column;
	Gtk::TreeView tree;
	Gtk::VBox effect_vbox;
	Gtk::Widget *editor;

	Song *song;
	int track_index;
	Track *track;
	AudioEffect *effect;

	void _automation_toggled(const Glib::ustring &path);
	bool updating_automation;

public:
	sigc::signal4<void, Track *, AudioEffect *, int, bool> toggle_automation_visibility;

	void update_automations();
	void edit(AudioEffect *p_effect, Track *p_track, Gtk::Widget *p_editor);
	EffectEditor();
};

typedef Gtk::Widget *(*EffectEditorPluginFunc)(AudioEffect *, EffectEditor *);

#endif // EFFECT_EDITOR_H
