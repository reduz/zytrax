#include "effect_editor_midi_device.h"
#include "engine/midi_driver_manager.h"
#include "engine/sound_driver_manager.h"
#include "gui/settings_dialog.h"
#include "gui/interface.h"

void EffectEditorMIDIDevice::_favorite_changed(const Glib::ustring &path) {

	Gtk::TreeIter iter = patch_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool fav = (*iter)[patch_model_columns.favorite];
	(*iter)[patch_model_columns.favorite] = !fav;
	MIDIBankManager::set_favorite(
				Glib::ustring((*iter)[patch_model_columns.device_name]).raw(),
				Glib::ustring((*iter)[patch_model_columns.bank_name]).raw(),
				(*iter)[patch_model_columns.bank_msb],
				(*iter)[patch_model_columns.bank_lsb],
				(*iter)[patch_model_columns.patch_index],
				!fav);

	SettingsDialog::get_singleton()->save_settings();
}

bool EffectEditorMIDIDevice::PatchListTree::on_key_press_event(GdkEventKey *key_event) {
	if (::Interface::get_singleton()->play_keyboard_note_for_pattern(key_event,true)) {
		return true;
	}

	return Gtk::TreeView::on_key_press_event(key_event);


}

bool EffectEditorMIDIDevice::PatchListTree::on_key_release_event(GdkEventKey *key_event)  {

	if (::Interface::get_singleton()->play_keyboard_note_for_pattern(key_event,false)) {
		return true;
	}

	return Gtk::TreeView::on_key_release_event(key_event);
}

void EffectEditorMIDIDevice::_port_selection_changed() {

	Gtk::TreeModel::iterator iter = port.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			uint32_t hash = row[port_model_columns.hash];
			effect_midi_device->set_port_hash(hash);
		}
	}

	effect_editor_midi.update_channel_warning();
}

void EffectEditorMIDIDevice::_device_selection_changed() {

	if (updating) {
		return;
	}

	Gtk::TreeModel::iterator iter = device_tree_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	Glib::ustring s = row[device_model_columns.name];
	String selected;
	selected.parse_utf8(s.c_str());

	effect_midi_device->set_device_layout_name(selected);

	_update_patches();

	effect_editor_midi.update_channel_warning();

}

void EffectEditorMIDIDevice::_patch_selection_changed() {

	if (updating) {
		return;
	}

	Gtk::TreeModel::iterator iter = patch_tree_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	effect_midi_device->set_patch_index(row[patch_model_columns.patch_index]);
	String p;
	p.parse_utf8(Glib::ustring(row[patch_model_columns.patch_name]).c_str());
	effect_midi_device->set_patch_name(p);
	effect_midi_device->set_bank_lsb(row[patch_model_columns.bank_lsb]);
	effect_midi_device->set_bank_msb(row[patch_model_columns.bank_msb]);
	p.parse_utf8(Glib::ustring(row[patch_model_columns.bank_name]).c_str());
	effect_midi_device->set_bank_name(p);
}


void EffectEditorMIDIDevice::_update_ports() {

	port_list_store->clear();

	if (SoundDriverManager::get_driver()==nullptr) {
		return;
	}

	Vector<SoundDriver::MidiDeviceInfo> devices = SoundDriverManager::get_driver()->get_midi_devices();

	for (int i = 0; i < devices.size() ; i++) {

		Gtk::TreeModel::iterator iter = port_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[port_model_columns.name] = devices[i].name.utf8().get_data();
		row[port_model_columns.hash] = devices[i].hash;
		row[port_model_columns.index] = i;

		if (effect_midi_device->get_port_hash() == devices[i].hash) {
			port.set_active(row);
		}
	}

	last_midi_ports_hash = SoundDriverManager::get_driver()->get_midi_devices_hash();

	updating = false;
}

void EffectEditorMIDIDevice::_update_devices() {

	updating = true;
	device_list_store->clear();

	for (int i = -1; i < MIDIBankManager::MAX_DEVICE_FILES ; i++) {

		if (i >= 0 && MIDIBankManager::get_device_file_path(i)=="") {
			continue;
		}

		Gtk::TreeModel::iterator iter = device_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		String name = MIDIBankManager::get_device_file_name(i);
		row[device_model_columns.name] = name.utf8().get_data();
		row[device_model_columns.index] = i;

		if (effect_midi_device->get_device_layout_name()==name) {
			device_tree_selection->select(row);
		}
	}

	updating = false;
}

void EffectEditorMIDIDevice::_update_patches() {

	updating = true;
	patch_list_store->clear();

	String device_name = effect_midi_device->get_device_layout_name();
	String filter;
	filter.parse_utf8(patch_search_entry.get_text().c_str());

	for (int i = 0; i < MIDIBankManager::get_bank_count() ; i++) {

		if (MIDIBankManager::get_bank_device(i) != device_name) {
			continue;
		}

		String bank_name = MIDIBankManager::get_bank_name(i);

		for (int j = 0; j < MIDIBankManager::get_bank_patch_count(i) ; j++) {

			String patch_name = MIDIBankManager::get_bank_patch_name(i,j);
			int patch_index = MIDIBankManager::get_bank_patch_index(i,j);
			int bank_msb = MIDIBankManager::get_bank_patch_msb(i,j);
			int bank_lsb = MIDIBankManager::get_bank_patch_lsb(i,j);
			bool favorite = MIDIBankManager::is_favorite(device_name.utf8().get_data(),  bank_name.utf8().get_data(), bank_msb, bank_lsb, MIDIBankManager::get_bank_patch_index(i,j));

			if (patch_search_favorite.get_active() && !favorite) {
				continue;
			}

			if (filter != String() && patch_name.findn(filter)==-1) {
				continue;
			}

			Gtk::TreeModel::iterator iter = patch_list_store->append();
			Gtk::TreeModel::Row row = *iter;


			row[patch_model_columns.device_name] = device_name.utf8().get_data();
			row[patch_model_columns.bank_name] = bank_name.utf8().get_data();
			row[patch_model_columns.patch_name] = patch_name.utf8().get_data();
			row[patch_model_columns.patch_index] = MIDIBankManager::get_bank_patch_index(i,j);
			row[patch_model_columns.bank_msb] = bank_msb;
			row[patch_model_columns.bank_lsb] = bank_lsb;
			row[patch_model_columns.favorite] = favorite;

			if (bank_name == effect_midi_device->get_bank_name() &&
					patch_index == effect_midi_device->get_patch_index() &&
					bank_msb == effect_midi_device->get_bank_msb() &&
					bank_lsb == effect_midi_device->get_bank_lsb() ) {
				// The actual selected patch
				patch_tree_selection->select(row);
				patch_list_tree.scroll_to_row(patch_list_tree.get_model()->get_path(row),0.5);
			}
		}
	}

	// NRPN list

	Vector<AudioEffectMIDI::NRPNInfo> nrpns;

	for (int i = 0; i < MIDIBankManager::get_nrpn_count(); i++) {
		if (MIDIBankManager::get_nrpn_device_name(i) != device_name) {
			continue;
		}

		AudioEffectMIDI::NRPNInfo ninfo;
		ninfo.msb = MIDIBankManager::get_nrpn_msb(i);
		ninfo.lsb = MIDIBankManager::get_nrpn_lsb(i);
		ninfo.descriptor = MIDIBankManager::get_nrpn_name(i);
		ninfo.default_value=0;
		nrpns.push_back(ninfo);
	}

	effect_editor_midi.set_nrpns(nrpns);

	updating = false;
}

bool EffectEditorMIDIDevice::_check_midi_ports_changes() {
	if (SoundDriverManager::get_driver() && SoundDriverManager::get_driver()->get_midi_devices_hash()!=last_midi_ports_hash) {
		_update_ports();
	}
	return true;
}

void EffectEditorMIDIDevice::_bank_list_changed() {
	_update_devices();
	_update_patches();
}

EffectEditorMIDIDevice::EffectEditorMIDIDevice(AudioEffectMIDIDevice *p_midi_device, EffectEditor *p_editor) :
		effect_editor_midi(p_midi_device, p_editor) {

	effect_midi_device = p_midi_device;
	editor = p_editor;

	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);
	effect_editor_midi.prepend_page(main_vbox, "MIDI Patch");


	main_vbox.pack_start(port_frame, Gtk::PACK_SHRINK);
	main_vbox.pack_start(patch_list_hpaned, Gtk::PACK_EXPAND_WIDGET);
	main_vbox.pack_start(patch_hbox, Gtk::PACK_SHRINK);

	// Port

	port_list_store = Gtk::ListStore::create(port_model_columns);
	port.set_model(port_list_store);
	port.pack_start(port_model_columns.name);

	port_frame.set_label("MIDI Ports:");
	port_frame.add(port);


	port.signal_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDIDevice::_port_selection_changed));

	// Device

	patch_list_hpaned.add(device_vbox);

	device_frame.set_label("Device Layout:");
	device_vbox.pack_start(device_frame, Gtk::PACK_EXPAND_WIDGET);
	device_frame.add(device_tree);

	device_list_store = Gtk::ListStore::create(device_model_columns);
	device_tree_selection = device_tree.get_selection();
	device_tree_selection->signal_changed().connect(sigc::mem_fun(this, &EffectEditorMIDIDevice::_device_selection_changed));

	device_tree.set_model(device_list_store);
	device_tree.append_column("Name", device_model_columns.name);


	// Patch

	patch_list_frame.set_label("Patch List:");

	patch_list_hpaned.add(patch_list_vbox);
	patch_list_vbox.pack_start(patch_search_hbox, Gtk::PACK_SHRINK);
	patch_search_hbox.pack_start(patch_search_label,Gtk::PACK_SHRINK);
	patch_search_label.set_text("Search: ");
	patch_search_hbox.pack_start(patch_search_entry,Gtk::PACK_EXPAND_WIDGET);
	patch_search_entry.signal_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDIDevice::_update_patches));
	patch_search_favorite.set_label("Favorites");
	patch_search_hbox.pack_start(patch_search_favorite,Gtk::PACK_SHRINK);
	patch_search_favorite.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditorMIDIDevice::_update_patches));


	patch_list_vbox.pack_start(patch_list_frame, Gtk::PACK_EXPAND_WIDGET);

	patch_list_frame.add(patch_list_scroll);
	patch_list_scroll.add(patch_list_tree);

	patch_list_store = Gtk::ListStore::create(patch_model_columns);
	patch_tree_selection = patch_list_tree.get_selection();
	patch_tree_selection->signal_changed().connect(sigc::mem_fun(this, &EffectEditorMIDIDevice::_patch_selection_changed));

	patch_list_tree.set_model(patch_list_store);
	patch_list_tree.append_column("Bank", patch_model_columns.bank_name);
	patch_list_tree.get_column(0)->set_expand(false);
	patch_list_tree.append_column("MSB", patch_model_columns.bank_msb);
	patch_list_tree.get_column(1)->set_expand(false);
	patch_list_tree.append_column("LSB", patch_model_columns.bank_lsb);
	patch_list_tree.get_column(2)->set_expand(false);
	patch_list_tree.append_column("#", patch_model_columns.patch_index);
	patch_list_tree.get_column(3)->set_expand(false);
	patch_list_tree.append_column("Patch Name", patch_model_columns.patch_name);
	patch_list_tree.get_column(4)->set_expand(true);

	patch_favorite_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditorMIDIDevice::_favorite_changed));
	patch_favorite_column.pack_start(patch_favorite_check);
	patch_favorite_column.add_attribute(patch_favorite_check.property_active(), patch_model_columns.favorite);
	patch_favorite_column.set_title("Fav");
	patch_list_tree.append_column(patch_favorite_column);
	patch_list_tree.get_column(5)->set_expand(false);


	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();
	//	set_default_size(width / 2, height / 2);
	patch_list_hpaned.set_size_request(width / 3, height / 2);

	patch_list_hpaned.set_position(width / 12);

	show_all_children();


	updating = false;

	_update_ports();
	_update_devices();
	_update_patches();

	Glib::signal_timeout().connect(sigc::mem_fun(*this,&EffectEditorMIDIDevice::_check_midi_ports_changes), 1000);
	SettingsDialog::get_singleton()->update_bank_list.connect(sigc::mem_fun(*this,&EffectEditorMIDIDevice::_bank_list_changed));

}

EffectEditorMIDIDevice::~EffectEditorMIDIDevice() {
}

Gtk::Widget *create_midi_device_editor(AudioEffect *p_fx, EffectEditor *p_editor) {
	if (p_fx->get_provider_id() != "internal" || p_fx->get_unique_id() != "midi_device") {
		return NULL;
	}
	return new EffectEditorMIDIDevice(static_cast<AudioEffectMIDIDevice *>(p_fx), p_editor);
}
