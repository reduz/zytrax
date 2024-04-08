#include "audio_effect_midi.h"
#include "globals/base64.h"
#include <math.h>

//semitones/sec
#define BEND_BASE_SPEED 100
#define BEND_VIBRATO_MAX_RATE_HZ 10.0
#define BEND_VIBRATO_MAX_DEPTH_SEMITONES 1.0
#define SLIDE_BASE_SPEED 100


void AudioEffectMIDI::_get_bank_and_patch(int &r_bank_lsb, int &r_bank_msb,int &r_patch) {
	r_bank_lsb=0;
	r_bank_msb=0;
	r_patch=0;
}

MIDIEventStamped *AudioEffectMIDI::_process_midi_events(const Event *p_events, int p_event_count, float p_time, int &r_stamped_event_count) {

	int idx = 0;

	//smart parameters

	int bank_lsb, bank_msb, patch;
	_get_bank_and_patch(bank_lsb, bank_msb, patch);

	if (reset_pending) {

		{ // Turn off all sound
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 123; // All notes off
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 120; // All sound off
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 121; // Reset all controllers
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

		}

		for(int i=0;i<MIDIEvent::CC_MAX;i++) {
			if (cc_use_default_values[i]) {
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = MIDIEvent::cc_indices[i]; // Reset all controllers
				process_events[idx].event.control.parameter = cc_default_values[i];
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
			}
		}

		if (custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].visible || custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get_command()) {

			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 65;
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			smart_porta_in_use = false;
			idx++;
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 68;
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

		}

		update_pitch_bend_range = true;
		{
			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			process_events[idx].event.pitch_bend.bend = PITCH_BEND_MAX;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
		}

		{ // bank and patch
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 0; // Bank MSB
			process_events[idx].event.control.parameter = bank_msb;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 32; // Bank HSB
			process_events[idx].event.control.parameter = bank_lsb;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

			process_events[idx].event.type = MIDIEvent::MIDI_PATCH_SELECT;
			process_events[idx].event.patch.index = patch;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

		}


		current_pitch_bend = 0;
		extra_pitch_bend = 0;
		reset_pending = false;
	}

	if (update_pitch_bend_range) {
		//pitch bend range
		process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
		process_events[idx].event.control.index = 101;
		process_events[idx].event.control.parameter = 0;
		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = 0;
		idx++;
		process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
		process_events[idx].event.control.index = 100;
		process_events[idx].event.control.parameter = 0;
		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = 0;
		idx++;
		process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
		process_events[idx].event.control.index = 6;
		process_events[idx].event.control.parameter = pitch_bend_range;
		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = 0;
		idx++;
		process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
		process_events[idx].event.control.index = 38;
		process_events[idx].event.control.parameter = 0;
		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = 0;
		idx++;

		if (mono_mode==MONO_MODE_ENABLE) {
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 126;
			process_events[idx].event.control.parameter = 1;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
		} if (mono_mode==MONO_MODE_DISABLE) {
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 127;
			process_events[idx].event.control.parameter = 0;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
		}

		update_pitch_bend_range = false;
	}


	if (true) {
		if ((custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].visible || custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get_command()) && custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].was_set) {
			float value = custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get();
			if (value == 0) {
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 65;
				process_events[idx].event.control.parameter = 0;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 68;
				process_events[idx].event.control.parameter = 0;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				smart_porta_in_use=false;
			} else {
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 65;
				process_events[idx].event.control.parameter = 127;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 5;
				process_events[idx].event.control.parameter = 127 - CLAMP(int(value * 127), 0, 127);
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 68;
				process_events[idx].event.control.parameter = 127;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				smart_porta_in_use=false;
			}
			custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].was_set = false;

		}

		if ((custom_ports[CUSTOM_MIDI_PITCH_BEND].visible || custom_ports[CUSTOM_MIDI_PITCH_BEND].get_command()) && custom_ports[CUSTOM_MIDI_PITCH_BEND].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			current_pitch_bend = int(custom_ports[CUSTOM_MIDI_PITCH_BEND].value);
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			custom_ports[CUSTOM_MIDI_PITCH_BEND].was_set = false;
		}

		if ((custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].visible || custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].get_command()) && custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			current_pitch_bend = int(custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].value);
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].was_set = false;
		}

		if ((custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].visible || custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].get_command()) && custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			current_pitch_bend = -int(custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].value);
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].was_set = false;
		}

		if ((custom_ports[CUSTOM_MIDI_AFTERTOUCH].visible || custom_ports[CUSTOM_MIDI_AFTERTOUCH].get_command()) && custom_ports[CUSTOM_MIDI_AFTERTOUCH].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_AFTERTOUCH;
			process_events[idx].event.aftertouch.pressure = CLAMP(int(custom_ports[CUSTOM_MIDI_AFTERTOUCH].value), 0, 127);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			custom_ports[CUSTOM_MIDI_AFTERTOUCH].was_set = false;
		}
	}

	//midi macros
	//TODO

	//CCs
	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {
		if (idx == INTERNAL_MIDI_EVENT_BUFFER_SIZE) {
			break;
		}
		if (!cc_ports[i].visible && !cc_ports[i].get_command()) {
			continue; //not enabled, continue
		}
		if (cc_ports[i].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = MIDIEvent::cc_indices[i];
			process_events[idx].event.control.parameter = uint8_t(CLAMP(cc_ports[i].value, 0.0, 127.0));
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			cc_ports[i].was_set = false;
			idx++;
		}
	}

	//events
	const int max_delayed_note_offs = 32;
	int delayed_note_offs[max_delayed_note_offs];
	int delayed_note_off_count = 0;

	for (int i = 0; i < p_event_count; i++) {
		if (idx == INTERNAL_MIDI_EVENT_BUFFER_SIZE) {
			break;
		}
		switch (p_events[i].type) {
			case Event::TYPE_NOTE: {

				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_ON;
				process_events[idx].event.note.note = p_events[i].param8;

				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));

			} break;
			case Event::TYPE_NOTE_OFF: {
				if (smart_porta_in_use) {
					bool off_on=false;
					for(int j=i+1;j<p_event_count;j++) {
						if (p_events[j].type == Event::TYPE_NOTE && p_events[j].offset==p_events[i].offset && p_events[j].param8 != p_events[i].param8) {
							off_on=true;
							break;
						}
					}
					if (off_on) {
						if (delayed_note_off_count < max_delayed_note_offs) {
							delayed_note_offs[delayed_note_off_count++]=i;
							continue;
						}

					}
				}
				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_OFF;
				process_events[idx].event.note.note = p_events[i].param8;
				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));
			} break;
			case Event::TYPE_AFTERTOUCH: {
				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_PRESSURE;
				process_events[idx].event.note.note = p_events[i].param8;
				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));
			} break;
			case Event::TYPE_BPM: {
				//process_events[idx].event.type = MIDIEvent::SEQ_TEMPO;
				//process_events[idx].event.tempo.tempo = p_events[i].param8;
				continue;
			} break;
		};

		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = p_events[i].offset;
		idx++;
	}

	for(int i=0;i<delayed_note_off_count;i++ ){
		if (idx == INTERNAL_MIDI_EVENT_BUFFER_SIZE) {
			break;
		}

		int noff_i = delayed_note_offs[i];
		process_events[idx].event.type = MIDIEvent::MIDI_NOTE_OFF;
		process_events[idx].event.note.note = p_events[noff_i].param8;
		process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[noff_i].paramf * 127.0, 0.0, 127.0));

		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = p_events[noff_i].offset + 1; // Has to be immediately after note.
		idx++;
	}

	r_stamped_event_count = idx;
	return process_events;
}

int AudioEffectMIDI::get_control_port_count() const {

	return TOTAL_INTERNAL_PORTS + _get_internal_control_port_count();
}
ControlPort *AudioEffectMIDI::get_control_port(int p_port) {
	if (p_port < CUSTOM_MIDI_MAX) {
		return &custom_ports[p_port];
	}
	p_port -= CUSTOM_MIDI_MAX;
	if (p_port < MIDIEvent::CC_MAX) {
		return &cc_ports[p_port];
	}
	p_port -= MIDIEvent::CC_MAX;
	return _get_internal_control_port(p_port);
}

void AudioEffectMIDI::set_cc_visible(MIDIEvent::CC p_cc, bool p_visible) {
	cc_ports[p_cc].visible = p_visible;
}
bool AudioEffectMIDI::is_cc_visible(MIDIEvent::CC p_cc) const {
	return cc_ports[p_cc].visible;
}

void AudioEffectMIDI::set_cc_use_default_value(MIDIEvent::CC p_cc, bool p_enable) {
	cc_use_default_values[p_cc]=p_enable;
}

bool AudioEffectMIDI::get_cc_use_default_value(MIDIEvent::CC p_cc) const{
	return cc_use_default_values[p_cc];
}

void AudioEffectMIDI::set_cc_default_value(MIDIEvent::CC p_cc, int p_value){
	cc_default_values[p_cc]=p_value;
}

int AudioEffectMIDI::get_cc_default_value(MIDIEvent::CC p_cc) const{
	return cc_default_values[p_cc];
}


void AudioEffectMIDI::set_midi_channel(int p_channel) {
	ERR_FAIL_INDEX(p_channel, 16);
	midi_channel = p_channel;
	_reset_midi();
}
int AudioEffectMIDI::get_midi_channel() const {
	return midi_channel;
}

void AudioEffectMIDI::set_mono_mode(MonoMode p_mode) {
	mono_mode=p_mode;
	_reset_midi();
}
AudioEffectMIDI::MonoMode AudioEffectMIDI::get_mono_mode() const {
	return mono_mode;
}


void AudioEffectMIDI::set_midi_macro(int p_macro, const Vector<uint8_t> &p_bytes) {
	ERR_FAIL_INDEX(p_macro, CUSTOM_MIDI_MACRO_MAX);
	midi_macro[p_macro] = p_bytes;
}
Vector<uint8_t> AudioEffectMIDI::get_midi_macro(int p_macro) const {
	ERR_FAIL_INDEX_V(p_macro, CUSTOM_MIDI_MACRO_MAX, Vector<uint8_t>());
	return midi_macro[p_macro];
}

JSON::Node AudioEffectMIDI::to_json() const {

	JSON::Node node = JSON::object();
	JSON::Node enabled_ccs = JSON::array();
	JSON::Node default_val_ccs = JSON::array();
	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {
		if (cc_ports[i].visible) {
			enabled_ccs.add(MIDIEvent::cc_names[i]);
		}
		if (cc_use_default_values[i]) {
			JSON::Node defval = JSON::object();
			defval.add("cc",MIDIEvent::cc_names[i]);
			defval.add("value",cc_default_values[i]);
			default_val_ccs.add(defval);
		}
	}
	node.add("enabled_ccs", enabled_ccs);
	node.add("default_value_ccs", default_val_ccs);
	JSON::Node macros = JSON::array();
	for (int i = 0; i < CUSTOM_MIDI_MACRO_MAX; i++) {
		if (midi_macro[i].size()) {
			JSON::Node macro = JSON::object();
			macro.add("index", i);
			macro.add("macro", base64_encode(midi_macro[i]));
			macros.add(macro);
		}
	}
	if (macros.getCount()) {
		node.add("macros", macros);
	}

	node.add("midi_channel", midi_channel);
	node.add("pitch_bend_range", pitch_bend_range);
	node.add("mono_mode", mono_mode);

	JSON::Node effect_node = _internal_to_json();

	node.add("data", effect_node);

	return node;
}

Error AudioEffectMIDI::from_json(const JSON::Node &node) {

	JSON::Node enabled_ccs = node.get("enabled_ccs");
	for (int i = 0; i < enabled_ccs.getCount(); i++) {
		std::string ccname = enabled_ccs.get(i).toString();
		for (int j = 0; j < MIDIEvent::CC_MAX; j++) {
			if (ccname == MIDIEvent::cc_names[j]) {
				cc_ports[j].visible = true;
				break;
			}
		}
	}

	for (int j = 0; j < MIDIEvent::CC_MAX; j++) {
		cc_use_default_values[j]=false;
		cc_default_values[j]=0;
	}

	if (node.has("default_value_ccs")) {
		JSON::Node defvals = node.get("default_value_ccs");
		for (int i = 0; i < defvals.getCount(); i++) {
			JSON::Node defval = defvals.get(i);
			std::string ccname= defval.get("cc").toString();
			int ccval = defval.get("value").toInt();
			for (int j = 0; j < MIDIEvent::CC_MAX; j++) {
				if (ccname == MIDIEvent::cc_names[j]) {
					cc_use_default_values[j]=true;
					cc_default_values[j]=ccval;
					break;
				}
			}
		}
	}

	if (node.has("macros")) {
		JSON::Node macros = node.get("macros");
		for (int i = 0; i < macros.getCount(); i++) {
			JSON::Node macro = macros.get(i);
			int index = macro.get("index").toInt();
			std::string b64 = macro.get("macro").toString();
			ERR_CONTINUE(index < 0 || index >= CUSTOM_MIDI_MACRO_MAX);
			midi_macro[index] = base64_decode(b64);
		}
	}

	if (node.has("mono_mode")) {
		mono_mode = MonoMode(node.get("mono_mode").toInt());
	}

	midi_channel = node.get("midi_channel").toInt();
	pitch_bend_range = node.get("pitch_bend_range").toInt();
	if (pitch_bend_range < 2) {
		pitch_bend_range = 2;
	}

	return _internal_from_json(node.get("data"));
}

void AudioEffectMIDI::set_pitch_bend_range(int p_semitones) {
	pitch_bend_range = p_semitones;
	update_pitch_bend_range = true;
}
int AudioEffectMIDI::get_pitch_bend_range() const {
	return pitch_bend_range;
}

void AudioEffectMIDI::_reset_midi() {
	reset_pending = true;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].value = 0;
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].value = 0;
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].value = 0;
}
AudioEffectMIDI::AudioEffectMIDI() {

	midi_channel = 0;
	//set built in control ports

	custom_ports[CUSTOM_MIDI_PITCH_BEND].name = "Pitch Bend";
	custom_ports[CUSTOM_MIDI_PITCH_BEND].identifier = "pitch_bend";
	custom_ports[CUSTOM_MIDI_PITCH_BEND].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].min = -8192;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].max = 8191;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].step = 1;

	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].name = "Pitch Bend Up";
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].identifier = "pitch_bend_up";
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].min = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].max = 8191;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].step = 1;

	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].name = "Pitch Bend Down";
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].identifier = "pitch_bend_down";
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].min = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].max = 8192;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].step = 1;

	custom_ports[CUSTOM_MIDI_AFTERTOUCH].name = "Aftertouch";
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].identifier = "aftertouch";
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].value = 0;
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].min = 0;
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].max = 127;
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].step = 1;

	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].name = "Smart Portamento";
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].identifier = "smart_portamento";
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].max = 1.0;
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].step = 0.001;

	custom_ports[CUSTOM_MIDI_MACRO].name = "MIDI Macros";
	custom_ports[CUSTOM_MIDI_MACRO].identifier = "midi_macro";
	custom_ports[CUSTOM_MIDI_MACRO].max = 99;
	custom_ports[CUSTOM_MIDI_MACRO].step = 1;

	//set ports for CCs
	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {
		cc_ports[i].name = String("CC: ") + MIDIEvent::cc_names[i];
		cc_ports[i].identifier = String("cc_") + MIDIEvent::cc_names[i];
		cc_ports[i].visible = false;
		cc_ports[i].max = 127;
		cc_ports[i].step = 1;
		cc_ports[i].hint = ControlPort::HINT_RANGE;
	}

	cc_ports[MIDIEvent::CC_MODULATION].visible = true;
	cc_ports[MIDIEvent::CC_BREATH].visible = true;
	cc_ports[MIDIEvent::CC_PAN].visible = true;
	cc_ports[MIDIEvent::CC_PAN].value = 64;
	cc_ports[MIDIEvent::CC_EXPRESSION].visible = true;
	cc_ports[MIDIEvent::CC_EXPRESSION].value = 127;

	pitch_bend_range = 2;
	mono_mode = MONO_MODE_DEFAULT;

	reset_pending = true;
	current_pitch_bend = 0;
	extra_pitch_bend = 0;
	update_pitch_bend_range = true;
}
