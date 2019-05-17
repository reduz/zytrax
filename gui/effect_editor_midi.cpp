#include "effect_editor_midi.h"

void EffectEditorMIDI::_cc_toggled(const Glib::ustring &path) {

	Gtk::TreeIter iter = cc_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool visible = (*iter)[cc_model_columns.visible];
	(*iter)[cc_model_columns.visible] = !visible;
	midi_effect->set_cc_visible(MIDIEvent::CC(int((*iter)[cc_model_columns.index])), !visible);

	effect_editor->update_automations();
}

void EffectEditorMIDI::_midi_channel_changed() {

	int channel = midi_channel_spinbox.get_adjustment()->get_value();
	midi_effect->set_midi_channel(channel);
}

void EffectEditorMIDI::_midi_pitch_bend_range_changed() {

	int pitch_bend_range = midi_pitch_bend_range_spinbox.get_adjustment()->get_value();
	midi_effect->set_pitch_bend_range(pitch_bend_range);
}

String EffectEditorMIDI::_get_text_from_hex(const Vector<uint8_t> &p_hex) {

	String macro_text;
	for (int j = 0; j < p_hex.size(); j++) {
		const char *hex = "0123456789ABCDEF";
		uint8_t b = p_hex[j];
		macro_text += String(String::CharType(hex[b >> 4]));
		macro_text += String(String::CharType(hex[b & 0xF]));
		macro_text += " ";
	}
	return macro_text;
}
void EffectEditorMIDI::_macro_edited(const Glib::ustring &path, const Glib::ustring &text) {

	Gtk::TreeIter iter = macro_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);

	Vector<uint8_t> hex;

	uint8_t byte = 0;
	bool msb = true;
	for (int i = 0; i < text.length(); i++) {
		char c = text[i];
		uint8_t nibble;
		if (c >= 'a' && c <= 'f') {
			nibble = 10 + (c - 'a');
		} else if (c >= 'A' && c <= 'F') {
			nibble = 10 + (c - 'A');
		} else if (c >= '0' && c <= '9') {
			nibble = c - '0';
		} else {
			continue;
		}

		if (msb) {
			byte = nibble << 4;
			msb = false;
		} else {
			byte |= nibble;
			msb = true;
			hex.push_back(byte);
		}
	}

	(*iter)[macro_model_columns.text] = _get_text_from_hex(hex).ascii().get_data();
	int index = (*iter)[macro_model_columns.index];

	midi_effect->set_midi_macro(index, hex);
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
	midi_channel_spinbox.set_hexpand(true);
	midi_channel_spinbox.set_adjustment(Gtk::Adjustment::create(0, 0, 15));
	midi_channel_spinbox.get_adjustment()->set_value(midi_effect->get_midi_channel());
	midi_channel_spinbox.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_midi_channel_changed));

	midi_pitch_bend_range_label.set_text("Pitch Bend Range:");
	midi_grid.attach(midi_pitch_bend_range_label, 0, 1, 1, 1);
	midi_pitch_bend_range_label.set_hexpand(true);
	midi_grid.attach(midi_pitch_bend_range_spinbox, 1, 1, 1, 1);
	midi_pitch_bend_range_spinbox.set_hexpand(true);
	midi_pitch_bend_range_spinbox.set_adjustment(Gtk::Adjustment::create(2, 2, 24));
	midi_pitch_bend_range_spinbox.get_adjustment()->set_value(midi_effect->get_pitch_bend_range());
	midi_pitch_bend_range_spinbox.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_midi_pitch_bend_range_changed));

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
	cc_tree.set_model(cc_list_store);
	cc_tree.append_column(cc_column);
	cc_tree.get_column(0)->set_expand(true);

	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {

		Gtk::TreeModel::iterator iter = cc_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[cc_model_columns.name] = MIDIEvent::cc_names[i];
		row[cc_model_columns.visible] = midi_effect->is_cc_visible(MIDIEvent::CC(i));
		row[cc_model_columns.index] = i;
	}

	append_page(macro_vbox, "MIDI Macros");

	macro_vbox.pack_start(macro_scroll, Gtk::PACK_EXPAND_WIDGET);
	macro_scroll.add(macro_tree);

	macro_list_store = Gtk::ListStore::create(macro_model_columns);
	macro_tree_selection = macro_tree.get_selection();

	macro_tree.append_column("Index", macro_model_columns.label);
	macro_column.set_title("Macro");
	macro_column.pack_start(macro_column_text, true);
	macro_column_text.property_editable() = true;
	macro_column_text.signal_edited().connect(sigc::mem_fun(*this, &EffectEditorMIDI::_macro_edited));
	macro_column.add_attribute(macro_column_text.property_text(), macro_model_columns.text);
	macro_tree.append_column(macro_column);

	macro_tree.set_model(macro_list_store);
	macro_tree.get_column(0)->set_expand(false);
	macro_tree.get_column(1)->set_expand(true);

	for (int i = 0; i < AudioEffectMIDI::CUSTOM_MIDI_MACRO_MAX; i++) {

		if (i == 0) {
			continue;
		}
		Gtk::TreeModel::iterator iter = macro_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		Vector<uint8_t> macro = midi_effect->get_midi_macro(i);
		String macro_text = _get_text_from_hex(macro);
		row[macro_model_columns.label] = String::num(i).utf8().get_data();
		row[macro_model_columns.text] = macro_text.utf8().get_data();
		row[macro_model_columns.index] = i - 1;
	}
}
