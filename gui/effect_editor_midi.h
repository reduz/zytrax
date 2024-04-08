#ifndef EFFECT_EDITOR_MIDI_H
#define EFFECT_EDITOR_MIDI_H

#include "effect_editor.h"
#include "engine/audio_effect_midi.h"
#include <gtkmm.h>

class EffectEditorMIDI : public Gtk::Notebook {

	EffectEditor *effect_editor;
	AudioEffectMIDI *midi_effect;

	class MonoModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		MonoModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
	};

	MonoModelColumns mono_model_columns;
	Glib::RefPtr<Gtk::ListStore> mono_list_store;

	class CCModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		CCModelColumns() {
			add(name);
			add(visible);
			add(index);
			add(use_default_value);
			add(default_value);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<bool> visible;
		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<bool> use_default_value;
		Gtk::TreeModelColumn<int> default_value;
	};

	CCModelColumns cc_model_columns;
	Gtk::VBox cc_vbox;
	Gtk::Grid midi_grid;
	Gtk::Label midi_channel_label;
	Gtk::SpinButton midi_channel_spinbox;
	Gtk::Label midi_pitch_bend_range_label;
	Gtk::SpinButton midi_pitch_bend_range_spinbox;
	Gtk::Label midi_mono_label;
	Gtk::ComboBox midi_mono_combo;
	Gtk::VSeparator cc_separator;
	Glib::RefPtr<Gtk::ListStore> cc_list_store;
	Glib::RefPtr<Gtk::TreeSelection> cc_tree_selection;
	Gtk::ScrolledWindow cc_scroll;
	Gtk::CellRendererToggle cc_enabled_check;
	Gtk::CellRendererText cc_enabled_text;

	Gtk::CellRendererToggle cc_defval_enabled_check;
	Gtk::CellRendererSpin cc_defval_edit_check;

	Gtk::TreeViewColumn cc_column;
	Gtk::TreeViewColumn cc_defval_column;
	Gtk::TreeView cc_tree;

	Gtk::VBox macro_vbox;
	class MacroModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		MacroModelColumns() {
			add(label);
			add(text);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::ustring> text;
		Gtk::TreeModelColumn<int> index;
	};
	MacroModelColumns macro_model_columns;
	Glib::RefPtr<Gtk::ListStore> macro_list_store;
	Glib::RefPtr<Gtk::TreeSelection> macro_tree_selection;
	Gtk::TreeViewColumn macro_column;
	Gtk::CellRendererText macro_column_text;
	Gtk::ScrolledWindow macro_scroll;
	Gtk::TreeView macro_tree;

	void _mono_changed();

	void _cc_toggled(const Glib::ustring &path);
	void _cc_defval_toggled(const Glib::ustring &path);
	void _cc_defval_edited(const Glib::ustring& path, const Glib::ustring& new_text);

	void _midi_channel_changed();
	void _midi_pitch_bend_range_changed();
	void _macro_edited(const Glib::ustring &path, const Glib::ustring &text);

	String _get_text_from_hex(const Vector<uint8_t> &p_hex);

public:
	EffectEditorMIDI(AudioEffectMIDI *p_effect, EffectEditor *p_effect_editor);
};

#endif // EFFECT_EDITOR_MIDI_H
