#ifndef EFFECT_EDITOR_MIDI_DEVICE_H
#define EFFECT_EDITOR_MIDI_DEVICE_H

#include "gui/effect_editor_midi.h"
#include "effects/internal/effect_midi_device.h"

class EffectEditorMIDIDevice : public Gtk::VBox {

	EffectEditorMIDI effect_editor_midi;


	Gtk::ComboBox port;

	class PortModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PortModelColumns() {
			add(name);
			add(index);
			add(hash);
		}

		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<uint32_t> hash;
		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	Glib::RefPtr<Gtk::ListStore> port_list_store;
	Glib::RefPtr<Gtk::TreeSelection> port_tree_selection;
	PortModelColumns port_model_columns;

	class DeviceModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		DeviceModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	Glib::RefPtr<Gtk::ListStore> device_list_store;
	Glib::RefPtr<Gtk::TreeSelection> device_tree_selection;
	DeviceModelColumns device_model_columns;

	class PatchModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PatchModelColumns() {
			add(device_name);
			add(bank_name);
			add(bank_msb);
			add(bank_lsb);
			add(patch_index);
			add(patch_name);
			add(favorite);
		}

		Gtk::TreeModelColumn<Glib::ustring> device_name;
		Gtk::TreeModelColumn<Glib::ustring> bank_name;
		Gtk::TreeModelColumn<int> patch_index;
		Gtk::TreeModelColumn<int> bank_msb;
		Gtk::TreeModelColumn<int> bank_lsb;
		Gtk::TreeModelColumn<Glib::ustring> patch_name;
		Gtk::TreeModelColumn<bool> favorite;
	};

	Gtk::CellRendererToggle patch_favorite_check;
	Gtk::TreeViewColumn patch_favorite_column;

	Glib::RefPtr<Gtk::ListStore> patch_list_store;
	Glib::RefPtr<Gtk::TreeSelection> patch_tree_selection;
	PatchModelColumns patch_model_columns;

	Gtk::VBox main_vbox;

	Gtk::HPaned patch_list_hpaned;

	Gtk::ScrolledWindow patch_list_scroll;

	Gtk::Frame port_frame;

	Gtk::VBox device_vbox;
	Gtk::Frame device_frame;

	Gtk::VBox patch_list_vbox;
	Gtk::Label patch_search_label;
	Gtk::Entry patch_search_entry;
	Gtk::HBox patch_search_hbox;
	Gtk::CheckButton patch_search_favorite;

	Gtk::Frame patch_list_frame;
	Gtk::Frame patch_frame;

	Gtk::TreeView device_tree;

	struct PatchListTree : public Gtk::TreeView {

		//hide on escape
		virtual bool on_key_press_event(GdkEventKey *key_event) override;
		virtual bool on_key_release_event(GdkEventKey *key_event) override;

	};

	PatchListTree patch_list_tree;

	Gtk::HBox patch_hbox;

	Gtk::Entry port_name;
	Gtk::Entry device_name;
	Gtk::Entry bank_name;
	Gtk::SpinButton patch_msb;
	Gtk::SpinButton patch_lsb;
	Gtk::SpinButton patch_index;
	Gtk::Entry patch_name;

	void _favorite_changed(const Glib::ustring &path);
	void _port_selection_changed();
	void _device_selection_changed();
	void _patch_selection_changed();
	void _update_ports();
	void _update_devices();
	void _update_patches();
	int device_selected = 0;

	AudioEffectMIDIDevice *effect_midi_device;
	EffectEditor *editor;

	bool updating;

	bool _check_midi_ports_changes();
	uint32_t last_midi_ports_hash = 0;
	void _bank_list_changed();

public:
	EffectEditorMIDIDevice(AudioEffectMIDIDevice *p_midi_device, EffectEditor *p_editor);
	~EffectEditorMIDIDevice();
};

Gtk::Widget *create_midi_device_editor(AudioEffect *p_fx, EffectEditor *p_editor);

#endif // EFECT_EDITOR_MIDIDevice_H
