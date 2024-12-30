#include "effect_editor_midi.h"
#include "engine/midi_driver_manager.h"
#include "gui/icons.h"
#include "gui/interface.h"
#include "effects/internal/effect_midi_device.h"

void EffectEditorMIDI::_mono_changed() {

	Gtk::TreeModel::iterator iter = midi_mono_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[mono_model_columns.index];

			midi_effect->set_mono_mode(AudioEffectMIDI::MonoMode(id));
		}
	}
}


void EffectEditorMIDI::_cc_toggled(const Glib::ustring &path) {

	Gtk::TreeIter iter = cc_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool visible = (*iter)[cc_model_columns.visible];
	(*iter)[cc_model_columns.visible] = !visible;
	midi_effect->set_cc_visible(MIDIEvent::CC(int((*iter)[cc_model_columns.index])), !visible);

	effect_editor->update_automations();
}

void EffectEditorMIDI::_cc_defval_toggled(const Glib::ustring &path) {

	Gtk::TreeIter iter = cc_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool def = (*iter)[cc_model_columns.use_default_value];
	(*iter)[cc_model_columns.use_default_value] = !def;
	midi_effect->set_cc_use_default_value(MIDIEvent::CC(int((*iter)[cc_model_columns.index])), !def);

}

void EffectEditorMIDI::_cc_defval_edited(const Glib::ustring& path, const Glib::ustring& new_text) {
	Gtk::TreeIter iter = cc_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	int cc = std::stoi(new_text);
	(*iter)[cc_model_columns.default_value] = cc;
	midi_effect->set_cc_default_value(MIDIEvent::CC(int((*iter)[cc_model_columns.index])), cc);

}

//

void EffectEditorMIDI::_nrpn_toggled(const Glib::ustring &path) {

	Gtk::TreeIter iter = nrpn_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool enable = (*iter)[nrpn_model_columns.enable];
	enable = !enable;
	(*iter)[nrpn_model_columns.enable] = enable;

	if (enable) {
		Glib::ustring desc = (*iter)[nrpn_model_columns.description];
		String name;
		name.parse_utf8(desc.c_str());
		midi_effect->set_nrpn(int((*iter)[nrpn_model_columns.msb]),int((*iter)[nrpn_model_columns.lsb]),int((*iter)[nrpn_model_columns.default_value]),name);
	} else {
		midi_effect->clear_nrpn(int((*iter)[nrpn_model_columns.msb]),int((*iter)[nrpn_model_columns.lsb]));
	}
}


void EffectEditorMIDI::_nrpn_defval_edited(const Glib::ustring& path, const Glib::ustring& new_text) {
	Gtk::TreeIter iter = nrpn_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	int nrpn = std::stoi(new_text);
	(*iter)[nrpn_model_columns.default_value] = nrpn;
	bool enable = (*iter)[nrpn_model_columns.enable];
	if (enable) {
		Glib::ustring desc = (*iter)[nrpn_model_columns.description];
		String name;
		name.parse_utf8(desc.c_str());
		midi_effect->set_nrpn(int((*iter)[nrpn_model_columns.msb]),int((*iter)[nrpn_model_columns.lsb]),int((*iter)[nrpn_model_columns.default_value]),name);
	}

	//midi_effect->set_nrpn_default_value(MIDIEvent::CC(int((*iter)[nrpn_model_columns.index])), nrpn);
}

void EffectEditorMIDI::_midi_channel_changed() {

	int channel = midi_channel_spinbox.get_adjustment()->get_value();
	midi_effect->set_midi_channel(channel - 1);
	update_channel_warning();
}

void EffectEditorMIDI::_curve_exponent_changed() {

	float exponent = curve_exponent_spinbox.get_adjustment()->get_value();
	midi_effect->set_curve_exponent(exponent);
}

void EffectEditorMIDI::_midi_pitch_bend_range_changed() {

	int pitch_bend_range = midi_pitch_bend_range_spinbox.get_adjustment()->get_value();
	midi_effect->set_pitch_bend_range(pitch_bend_range);
}

String EffectEditorMIDI::_get_text_from_hex(const Vector<uint8_t> &p_hex) {

	String nrpn_text;
	for (int j = 0; j < p_hex.size(); j++) {
		const char *hex = "0123456789ABCDEF";
		uint8_t b = p_hex[j];
		nrpn_text += String(String::CharType(hex[b >> 4]));
		nrpn_text += String(String::CharType(hex[b & 0xF]));
		nrpn_text += " ";
	}
	return nrpn_text;
}

void EffectEditorMIDI::set_nrpns(const Vector<AudioEffectMIDI::NRPNInfo> &p_nrpn) {

	Vector<AudioEffectMIDI::NRPNInfo> device_info = p_nrpn;
	Vector<AudioEffectMIDI::NRPNInfo> effect_info = midi_effect->get_nrpns();
	// Remove duplicate
	for(int i=0;i<device_info.size();i++) {
		for(int j=0;j<effect_info.size();j++) {
			if (effect_info[j].lsb == device_info[i].lsb && effect_info[j].msb == device_info[i].msb) {
				device_info[i] = effect_info[j];
				effect_info.remove(j);
				break;
			}
		}
	}
	// Append remaining (unused)
	for(int j=0;j<effect_info.size();j++) {
		device_info.push_back(effect_info[j]);
	}

	nrpn_list_store->clear();

	for(int i=0;i<device_info.size();i++) {

		Gtk::TreeModel::iterator iter = nrpn_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[nrpn_model_columns.enable] = midi_effect->has_nrpn(device_info[i].msb,device_info[i].lsb);
		row[nrpn_model_columns.msb] = device_info[i].msb;
		row[nrpn_model_columns.lsb] = device_info[i].lsb;
		char msb_lsb[6]={0,0,':',0,0,0};
		const char hex_table[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
		msb_lsb[0]=hex_table[(device_info[i].msb>>4)&0xF];
		msb_lsb[1]=hex_table[(device_info[i].msb)&0xF];
		msb_lsb[3]=hex_table[(device_info[i].lsb>>4)&0xF];
		msb_lsb[4]=hex_table[(device_info[i].lsb)&0xF];
		String name = msb_lsb;
		name+=" "+device_info[i].descriptor;
		row[nrpn_model_columns.name] = name.utf8().get_data();
		row[nrpn_model_columns.description] = device_info[i].descriptor.utf8().get_data();

		row[nrpn_model_columns.default_value] = device_info[i].default_value;
	}

}


void EffectEditorMIDI::update_channel_warning() {

	AudioEffectMIDIDevice *md = dynamic_cast<AudioEffectMIDIDevice*>(midi_effect);
	if (!md) {
		return;
	}

	bool show_warning = false;
	Song * song = ::Interface::get_singleton()->get_song();
	for(int i=0;i<song->get_track_count();i++) {
		Track *track = song->get_track(i);
		for(int j=0;j<track->get_audio_effect_count();j++) {
			AudioEffect * ae = track->get_audio_effect(j);
			AudioEffectMIDIDevice *md2 = dynamic_cast<AudioEffectMIDIDevice*>(ae);
			if (md2 == nullptr || md2 == md) {
				continue; // same, do not check
			}

			if (md2->get_midi_channel() == md->get_midi_channel() && md2->get_port_hash() == md->get_port_hash()) {

				show_warning = true;
				break;
			}
		}
	}
	if (channel_warning_visible == show_warning) {
		return;
	}

	if (show_warning) {
		midi_grid.attach(channel_warning_image, 2, 0, 1, 1);
		midi_grid.attach(channel_warning_text, 3, 0, 1, 1);
		channel_warning_text.show();
		channel_warning_image.show();
	} else {
		channel_warning_text.hide();
		channel_warning_image.hide();
		midi_grid.remove(channel_warning_image);
		midi_grid.remove(channel_warning_text);
	}

	channel_warning_visible = show_warning;
}


EffectEditorMIDI::EffectEditorMIDI(AudioEffectMIDI *p_effect, EffectEditor *p_editor) {

	midi_effect = p_effect;
	effect_editor = p_editor;

	append_page(cc_vbox, "MIDI Controls");
	cc_vbox.pack_start(midi_grid, Gtk::PACK_SHRINK);
	midi_channel_label.set_text("MIDI Channel:");
	midi_channel_label.set_hexpand(true);
	midi_grid.attach(midi_channel_label, 0, 0, 1, 1);
	midi_grid.attach(midi_channel_spinbox, 1, 0, 1, 1);

	channel_warning_image = create_image_from_icon("Warning");
	channel_warning_text.set_text("Channel Conflict!  ");
	midi_channel_spinbox.set_hexpand(true);
	midi_channel_spinbox.set_adjustment(Gtk::Adjustment::create(1, 1, 16));
	midi_channel_spinbox.get_adjustment()->set_value(midi_effect->get_midi_channel() + 1);
	midi_channel_spinbox.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_midi_channel_changed));

	midi_pitch_bend_range_label.set_text("Pitch Bend Range:");
	midi_grid.attach(midi_pitch_bend_range_label, 0, 1, 1, 1);
	midi_pitch_bend_range_label.set_hexpand(true);
	midi_grid.attach(midi_pitch_bend_range_spinbox, 1, 1, 1, 1);
	midi_pitch_bend_range_spinbox.set_hexpand(true);
	midi_pitch_bend_range_spinbox.set_adjustment(Gtk::Adjustment::create(2, 2, 24));
	midi_pitch_bend_range_spinbox.get_adjustment()->set_value(midi_effect->get_pitch_bend_range());
	midi_pitch_bend_range_spinbox.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_midi_pitch_bend_range_changed));

	midi_mono_label.set_text("Mono / Poly Mode:");

	{

		mono_list_store = Gtk::ListStore::create(mono_model_columns);
		midi_mono_combo.set_model(mono_list_store);

		const char *beat_mono_text[3] = {
			"Disabled",
			"Force Mono",
			"Force Poly",
		};

		for (int i = 0; i < 3; i++) {
			Gtk::TreeModel::Row row = *(mono_list_store->append());
			row[mono_model_columns.name] = beat_mono_text[i];
			row[mono_model_columns.index] = i;

			if (p_effect->get_mono_mode()==i) {
				midi_mono_combo.set_active(row);
			}
		}

		midi_mono_combo.pack_start(mono_model_columns.name);
		midi_mono_combo.signal_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_mono_changed));
	}

	midi_grid.attach(midi_mono_label, 0, 2, 1, 1);
	midi_mono_label.set_hexpand(true);
	midi_grid.attach(midi_mono_combo, 1, 2, 1, 1);

	curve_exponent_label.set_text("Velocity Curve Map:");
	midi_grid.attach(curve_exponent_label, 0, 3, 1, 1);
	midi_grid.attach(curve_exponent_spinbox, 1, 3, 1, 1);
	curve_exponent_spinbox.set_hexpand(true);
	curve_exponent_spinbox.set_adjustment(Gtk::Adjustment::create(0, 0.5, 2.5,0.1));
	curve_exponent_spinbox.get_adjustment()->set_value(midi_effect->get_curve_exponent());
	curve_exponent_spinbox.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_curve_exponent_changed));
	curve_exponent_spinbox.set_digits(1);

	cc_vbox.pack_start(cc_separator, Gtk::PACK_SHRINK);
	cc_vbox.pack_start(cc_scroll, Gtk::PACK_EXPAND_WIDGET);
	cc_scroll.add(cc_tree);

	cc_list_store = Gtk::ListStore::create(cc_model_columns);
	cc_tree_selection = cc_tree.get_selection();

	cc_column.set_title("Visible Controllers to Automate");
	cc_column.pack_start(cc_enabled_check, false);
	cc_column.pack_start(cc_enabled_text, true);
	cc_enabled_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_cc_toggled));
	cc_column.add_attribute(cc_enabled_check.property_active(), cc_model_columns.visible);
	cc_column.add_attribute(cc_enabled_text.property_text(), cc_model_columns.name);

	cc_defval_column.set_title("Use Default Value");
	cc_defval_column.pack_start(cc_defval_enabled_check, false);
	cc_defval_column.pack_start(cc_defval_edit_check, false);
	cc_defval_edit_check.property_editable() = true;
	cc_defval_edit_check.property_adjustment() = Gtk::Adjustment::create(0, 0, 127);
	cc_defval_enabled_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_cc_defval_toggled));
	cc_defval_column.add_attribute(cc_defval_enabled_check.property_active(), cc_model_columns.use_default_value);
	cc_defval_edit_check.signal_edited().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_cc_defval_edited));
	cc_defval_column.add_attribute(cc_defval_edit_check.property_text(), cc_model_columns.default_value);

	cc_tree.set_model(cc_list_store);
	cc_tree.append_column(cc_column);
	cc_tree.append_column(cc_defval_column);
	cc_tree.get_column(0)->set_expand(true);
	//cc_tree.get_column(1)->set_expand(true);

	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {

		Gtk::TreeModel::iterator iter = cc_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[cc_model_columns.name] = MIDIEvent::cc_names[i];
		row[cc_model_columns.visible] = midi_effect->is_cc_visible(MIDIEvent::CC(i));
		row[cc_model_columns.index] = i;

		row[cc_model_columns.use_default_value] = p_effect->get_cc_use_default_value(MIDIEvent::CC(i));
		row[cc_model_columns.default_value] = p_effect->get_cc_default_value(MIDIEvent::CC(i));

	}

	append_page(nrpn_vbox, "MIDI NRPNs");

	nrpn_vbox.pack_start(nrpn_scroll, Gtk::PACK_EXPAND_WIDGET);
	nrpn_scroll.add(nrpn_tree);

	nrpn_list_store = Gtk::ListStore::create(nrpn_model_columns);
	nrpn_tree_selection = nrpn_tree.get_selection();


	nrpn_column.set_title("Available NRPNs to Automate");
	nrpn_column.pack_start(nrpn_enabled_check, false);
	nrpn_column.pack_start(nrpn_enabled_text, true);
	nrpn_enabled_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_nrpn_toggled));
	nrpn_column.add_attribute(nrpn_enabled_check.property_active(), nrpn_model_columns.enable);
	nrpn_column.add_attribute(nrpn_enabled_text.property_text(), nrpn_model_columns.name);

	nrpn_defval_column.set_title("Use Default Value");
	nrpn_defval_column.pack_start(nrpn_defval_edit_check, false);
	nrpn_defval_edit_check.property_editable() = true;
	nrpn_defval_edit_check.property_adjustment() = Gtk::Adjustment::create(0, 0, 127);
	nrpn_defval_edit_check.signal_edited().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_nrpn_defval_edited));
	nrpn_defval_column.add_attribute(nrpn_defval_edit_check.property_text(), nrpn_model_columns.default_value);

	//
	/*
	nrpn_tree.append_column("Index", nrpn_model_columns.label);
	nrpn_column.set_title("NRPN");
	nrpn_column.pack_start(nrpn_column_text, true);
	nrpn_column_text.property_editable() = true;
	nrpn_column_text.signal_edited().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_nrpn_edited));
	nrpn_column.add_attribute(nrpn_column_text.property_text(), nrpn_model_columns.text);
	nrpn_tree.append_column(nrpn_column);
*/
	nrpn_tree.set_model(nrpn_list_store);
	nrpn_tree.append_column(nrpn_column);
	nrpn_tree.append_column(nrpn_defval_column);
	nrpn_tree.get_column(0)->set_expand(true);

	AudioEffectMIDIDevice *md = dynamic_cast<AudioEffectMIDIDevice*>(midi_effect);
	if (md) {
		update_channel_warning();
	}

}
