#ifndef EFFECT_EDITOR_PRESETS_H
#define EFFECT_EDITOR_PRESETS_H

#include "gui/effect_editor_midi.h"
#if 0
class EffectEditorPresets : public Gtk::VBox {


	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	Gtk::VBox vbox;
	Gtk::Button new_preset;
	Gtk::Button open_preset;
	Gtk::Button copy_preset;
	Gtk::Button save_preset;

	ModelColumns model_columns;

	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	Gtk::Label patch_search_label;
	Gtk::Entry patch_search_entry;
	Gtk::HBox patch_search_hbox;

	Gtk::ScrolledWindow scroll;


	struct PatchListTree : public Gtk::TreeView {

		//hide on escape
		virtual bool on_key_press_event(GdkEventKey *key_event) override;
		virtual bool on_key_release_event(GdkEventKey *key_event) override;

	};

	PatchListTree tree;

	void _open_soundfont();
	void _selection_changed();
	void _update_patches();

	AudioSynthPresets *synth_sf2;
	EffectEditor *editor;

	bool updating;

public:
	EffectEditorPresets(AudioSynthPresets *p_sf2, EffectEditor *p_editor);
	~EffectEditorPresets();
};
#endif
#endif // EFFECT_EDITOR_PRESETS_H
