#ifndef EFECT_EDITOR_DEFAULT_H
#define EFECT_EDITOR_DEFAULT_H

#include "engine/audio_effect.h"
#include "globals/map.h"
#include "gui/effect_editor.h"
#include <gtkmm.h>
class EffectEditorDefault : public Gtk::ScrolledWindow {

	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
	};

	ModelColumns model_columns;
	Glib::RefPtr<Gtk::ListStore> list_store;

	struct Combo {
		Glib::RefPtr<Gtk::ListStore> list_store;
		//Glib::RefPtr<Gtk::TreeSelection> tree_selection;
		Gtk::ComboBox *combo;
	};

	Gtk::Grid effect_grid;

	Vector<Gtk::Widget *> effects;
	Map<int, Gtk::HScale *> scales;
	Map<int, Gtk::Label *> labels;
	Map<int, Combo> combos;
	Map<int, Gtk::CheckButton *> buttons;

	Vector<Gtk::Widget *> widgets;
	AudioEffect *effect;

	void _combo_changed(int p_idx);
	void _scale_changed(int p_idx);
	void _toggle_clicked(int p_idx);

public:
	EffectEditorDefault(AudioEffect *p_effect);
	~EffectEditorDefault();
};

Gtk::Widget *create_default_editor_func(AudioEffect *p_effect, EffectEditor *p_editor);

#endif // EFECT_EDITOR_DEFAULT_H
