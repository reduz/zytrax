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
	Gtk::Label curve_exponent_label;
	Gtk::SpinButton curve_exponent_spinbox;
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
	Gtk::Image channel_warning_image;
	Gtk::Label channel_warning_text;
	bool channel_warning_visible = false;

	Gtk::CellRendererToggle cc_defval_enabled_check;
	Gtk::CellRendererSpin cc_defval_edit_check;

	Gtk::TreeViewColumn cc_column;
	Gtk::TreeViewColumn cc_defval_column;
	Gtk::TreeView cc_tree;

	Gtk::VBox nrpn_vbox;
	class NRPNModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		NRPNModelColumns() {
			add(enable);
			add(msb);
			add(lsb);
			add(name);
			add(description);
			add(default_value);
		}

		Gtk::TreeModelColumn<bool> enable;
		Gtk::TreeModelColumn<int> msb;
		Gtk::TreeModelColumn<int> lsb;
		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> description;
		Gtk::TreeModelColumn<int> default_value;
	};
	NRPNModelColumns nrpn_model_columns;
	Glib::RefPtr<Gtk::ListStore> nrpn_list_store;
	Glib::RefPtr<Gtk::TreeSelection> nrpn_tree_selection;
	Gtk::TreeViewColumn nrpn_column;
	Gtk::CellRendererText nrpn_column_text;
	Gtk::ScrolledWindow nrpn_scroll;

	Gtk::CellRendererToggle nrpn_enabled_check;
	Gtk::CellRendererText nrpn_enabled_text;

	Gtk::CellRendererSpin nrpn_defval_edit_check;

	//Gtk::TreeViewColumn nrpn_column;
	Gtk::TreeViewColumn nrpn_defval_column;

	Gtk::TreeView nrpn_tree;


	void _mono_changed();

	void _cc_toggled(const Glib::ustring &path);
	void _cc_defval_toggled(const Glib::ustring &path);
	void _cc_defval_edited(const Glib::ustring& path, const Glib::ustring& new_text);

	void _nrpn_toggled(const Glib::ustring &path);
	void _nrpn_defval_edited(const Glib::ustring& path, const Glib::ustring& new_text);

	void _midi_channel_changed();
	void _curve_exponent_changed();
	void _midi_pitch_bend_range_changed();

	String _get_text_from_hex(const Vector<uint8_t> &p_hex);


public:

	void update_channel_warning();

	void set_nrpns(const Vector<AudioEffectMIDI::NRPNInfo> &p_nrpn);
	EffectEditorMIDI(AudioEffectMIDI *p_effect, EffectEditor *p_effect_editor);
};

#endif // EFFECT_EDITOR_MIDI_H
