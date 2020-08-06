#ifndef EFECT_EDITOR_SF2_H
#define EFECT_EDITOR_SF2_H

#include "effects/sf2/synth_sf2.h"
#include "gui/effect_editor_midi.h"

class EffectEditorSF2 : public Gtk::VBox {

	EffectEditorMIDI effect_editor_midi;

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
	Gtk::Button open_soundfont;

	ModelColumns model_columns;

	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	Gtk::ScrolledWindow scroll;

	Gtk::TreeView tree;

	void _open_soundfont();
	void _selection_changed();
	void _update_patches();

	AudioSynthSF2 *synth_sf2;
	EffectEditor *editor;

	bool updating;

public:
	EffectEditorSF2(AudioSynthSF2 *p_sf2, EffectEditor *p_editor);
	~EffectEditorSF2();
};

Gtk::Widget *create_sf2_editor(AudioEffect *p_vst, EffectEditor *p_editor);

#endif // EFECT_EDITOR_SF2_H
