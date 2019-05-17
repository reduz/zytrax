#include "audio_effect_midi.h"
#include "globals/base64.h"
#include <math.h>

//semitones/sec
#define BEND_BASE_SPEED 100
#define BEND_VIBRATO_MAX_RATE_HZ 10.0
#define BEND_VIBRATO_MAX_DEPTH_SEMITONES 1.0
#define SLIDE_BASE_SPEED 100

AudioEffectMIDI::MIDIEventStamped *AudioEffectMIDI::_process_midi_events(const Event *p_events, int p_event_count, float p_time, int &r_stamped_event_count) {

	int idx = 0;

	//smart parameters

	if (reset_pending) {

		if (custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].visible || custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get_command()) {

			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 127;
			process_events[idx].event.control.parameter = 127;
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
			process_events[idx].event.control.index = 65;
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

		current_pitch_bend = 0;
		extra_pitch_bend = 0;
		bend_portamento_last_note = -1;
		bend_portamento_target_note = -1;
		prev_bend_portamento = false;
		prev_bend_slide = false;

		bp_remap_note_off_from = -1;
		bp_remap_note_off_to = -1;
		prev_bend_vibrato = 0;

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
		update_pitch_bend_range = false;
	}

	//used by bend portamento
	bool ignore_note_off = false;
	bool ignore_note_on = false;

	if (custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].visible || custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].get_command()) {
		//use smart bend

		float bend_portamento = custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].get_normalized();

		if (bend_portamento == 0) {

			bool found_note_or_off = false;

			for (int i = 0; i < p_event_count; i++) {
				if (p_events[i].type == Event::TYPE_NOTE) {
					bend_portamento_last_note = p_events[i].param8;
					found_note_or_off = true;
				}
				if (p_events[i].type == Event::TYPE_NOTE_OFF) {
					found_note_or_off = true;
				}
			}
			bend_portamento_target_note = -1;
			if (prev_bend_portamento && found_note_or_off) {
				//send event
				current_pitch_bend = 0;
				process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
				process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + PITCH_BEND_MAX, 0, 16383);
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				//reset pitch bend
				prev_bend_portamento = false;
			}

		} else {

			//exclusive
			if (custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].was_set) {
				custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].value = 0;
				custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].value = 0;
				custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].was_set = false;
			}

			bool found_note = false;
			bool found_note_off = false;
			for (int i = 0; i < p_event_count; i++) {
				if (p_events[i].type == Event::TYPE_NOTE) {
					bend_portamento_target_note = p_events[i].param8;
					found_note = true;
				}
				if (p_events[i].type == Event::TYPE_NOTE_OFF) {
					found_note_off = true;
				}
			}

			if (found_note && found_note_off) { //combo (one note ends, another starts)
				ignore_note_off = true; //do not want
				if (bend_portamento_last_note != -1) {
					bp_remap_note_off_from = bend_portamento_target_note;
					bp_remap_note_off_to = bend_portamento_last_note;
				}
			}

			if (found_note) {
				ignore_note_on = true;
			}

			if (bend_portamento_last_note != -1 && bend_portamento_target_note != -1) {

				//semitone difference
				float note_diff = bend_portamento_target_note - bend_portamento_last_note;
				//convert pitch bend to semitones (for more same speed no matter range);
				float pitch_bend_semitones = float(current_pitch_bend) * pitch_bend_range / PITCH_BEND_MAX;

				float diff = note_diff - pitch_bend_semitones;
				if (diff != 0) {
					pitch_bend_semitones += MIN(ABS(diff) * p_time * bend_portamento * BEND_BASE_SPEED, ABS(diff)) * SIGN(diff);

					current_pitch_bend = int(pitch_bend_semitones * PITCH_BEND_MAX / pitch_bend_range);

					//send event
					process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
					process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
					process_events[idx].event.channel = midi_channel;
					process_events[idx].frame = 0;
					idx++;
				}

				prev_bend_portamento = true;
			}
		}
		custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].was_set = false;
	}

	if (custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].visible || custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].get_command()) {

		float bend_slide = custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].get_normalized();

		if (bend_slide > 0.0 || prev_bend_slide) {

			if (bend_slide > 0 && custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].was_set) {
				//they are exclusive
				custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].value = 0;
			}
			bool found_note = false;

			for (int i = 0; i < p_event_count; i++) {
				if (p_events[i].type == Event::TYPE_NOTE) {
					found_note = true;
				}
			}

			if (found_note) {
				current_pitch_bend = 0;
				prev_bend_slide = false;
			} else {

				double decrease = p_time * SLIDE_BASE_SPEED * bend_slide * double(PITCH_BEND_MAX / pitch_bend_range);
				current_pitch_bend -= int(decrease);
				if (current_pitch_bend < PITCH_BEND_MIN) {
					current_pitch_bend = PITCH_BEND_MIN;
				}

				//reset pitch bend
				prev_bend_slide = true;
			}

			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
		}

		custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].was_set = false;
	}

	if (custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].visible || custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].get_command()) {

		float bend_slide = custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].get_normalized();

		if (bend_slide > 0.0 || prev_bend_slide) {

			if (bend_slide > 0 && custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].was_set) {
				//they are exclusive
				custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].value = 0;
			}
			bool found_note = false;

			for (int i = 0; i < p_event_count; i++) {
				if (p_events[i].type == Event::TYPE_NOTE) {
					found_note = true;
				}
			}

			if (found_note) {
				current_pitch_bend = 0;
				prev_bend_slide = false;
			} else {

				double decrease = p_time * SLIDE_BASE_SPEED * bend_slide * double(PITCH_BEND_MAX / pitch_bend_range);
				current_pitch_bend += int(decrease);
				if (current_pitch_bend < PITCH_BEND_MIN) {
					current_pitch_bend = PITCH_BEND_MIN;
				}

				//reset pitch bend
				prev_bend_slide = true;
			}

			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
		}

		custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].was_set = false;
	}

	if (custom_ports[CUSTOM_MIDI_BEND_VIBRATO].visible || custom_ports[CUSTOM_MIDI_BEND_VIBRATO].get_command()) {
		int bend_vibrato = int(custom_ports[CUSTOM_MIDI_BEND_VIBRATO].get());

		if (bend_vibrato) {

			if (prev_bend_vibrato == 0) {
				//turn on
				bend_vibrato_time = 0;
			}

			float depth = (float(bend_vibrato % 10) / 9.0) * BEND_VIBRATO_MAX_DEPTH_SEMITONES;
			float rate = (float(bend_vibrato / 10) / 9.0) * BEND_VIBRATO_MAX_RATE_HZ;

			extra_pitch_bend = (sin(bend_vibrato_time) * depth) * float(PITCH_BEND_MAX) / pitch_bend_range;

			bend_vibrato_time += p_time * rate * 3.14159265359 * 2.0;

			//send event
			process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
			process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;

		} else {

			if (prev_bend_vibrato) {

				//send event
				extra_pitch_bend = 0;
				process_events[idx].event.type = MIDIEvent::MIDI_PITCH_BEND;
				process_events[idx].event.pitch_bend.bend = CLAMP(current_pitch_bend + extra_pitch_bend + PITCH_BEND_MAX, 0, 16383);
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
			}
		}

		prev_bend_vibrato = bend_vibrato;
	}

	{
		if ((custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].visible || custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get_command()) && custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].was_set) {
			float value = custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].get();
			if (value == 0) {
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 127;
				process_events[idx].event.control.parameter = 127;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 65;
				process_events[idx].event.control.parameter = 0;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
			} else {
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 126;
				process_events[idx].event.control.parameter = 127;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 65;
				process_events[idx].event.control.parameter = 127;
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
				process_events[idx].event.type = MIDIEvent::MIDI_CONTROLLER;
				process_events[idx].event.control.index = 5;
				process_events[idx].event.control.parameter = CLAMP(int(value * 127), 0, 127);
				process_events[idx].event.channel = midi_channel;
				process_events[idx].frame = 0;
				idx++;
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

		if ((custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].visible || custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].get_command()) && custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].was_set) {
			process_events[idx].event.type = MIDIEvent::MIDI_PATCH_SELECT;
			process_events[idx].event.patch.index = CLAMP(int(custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].value), 0, 127);
			process_events[idx].event.channel = midi_channel;
			process_events[idx].frame = 0;
			idx++;
			custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].was_set = false;
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
	for (int i = 0; i < p_event_count; i++) {
		if (idx == INTERNAL_MIDI_EVENT_BUFFER_SIZE) {
			break;
		}
		switch (p_events[i].type) {
			case Event::TYPE_NOTE: {
				if (ignore_note_on) {
					continue;
				}
				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_ON;
				process_events[idx].event.note.note = p_events[i].param8;

				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));

			} break;
			case Event::TYPE_NOTE_OFF: {
				if (ignore_note_off) {
					continue;
				}
				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_OFF;
				if (bp_remap_note_off_from == p_events[i].param8) {
					process_events[idx].event.note.note = bp_remap_note_off_to;
					bp_remap_note_off_from = -1;
					bp_remap_note_off_to = -1;
				} else {
					process_events[idx].event.note.note = p_events[i].param8;
				}
				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));
			} break;
			case Event::TYPE_AFTERTOUCH: {
				process_events[idx].event.type = MIDIEvent::MIDI_NOTE_PRESSURE;
				process_events[idx].event.note.note = p_events[i].param8;
				process_events[idx].event.note.velocity = uint8_t(CLAMP(p_events[i].paramf * 127.0, 0.0, 127.0));
			} break;
			case Event::TYPE_BPM: {
				process_events[idx].event.type = MIDIEvent::SEQ_TEMPO;
				process_events[idx].event.tempo.tempo = p_events[i].param8;

			} break;
		};

		process_events[idx].event.channel = midi_channel;
		process_events[idx].frame = p_events[i].offset;
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

void AudioEffectMIDI::set_midi_channel(int p_channel) {
	ERR_FAIL_INDEX(p_channel, 16);
	midi_channel = p_channel;
}
int AudioEffectMIDI::get_midi_channel() const {
	return midi_channel;
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
	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {
		if (cc_ports[i].visible) {
			enabled_ccs.add(MIDIEvent::cc_names[i]);
		}
	}
	node.add("enabled_ccs", enabled_ccs);
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
	custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].value = 0;
	custom_ports[CUSTOM_MIDI_BEND_VIBRATO].value = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].value = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_UP].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND_DOWN].value = 0;
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].value = 0;
	custom_ports[CUSTOM_MIDI_AFTERTOUCH].value = 0;
}
AudioEffectMIDI::AudioEffectMIDI() {

	midi_channel = 0;
	//set built in control ports

	custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].name = "Bend Portamento";
	custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].identifier = "bend_portamento";
	custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].max = 1.0;
	custom_ports[CUSTOM_MIDI_BEND_PORTAMENTO].step = 0.001;

	custom_ports[CUSTOM_MIDI_BEND_VIBRATO].name = "Bend Vibrato";
	custom_ports[CUSTOM_MIDI_BEND_VIBRATO].identifier = "bend_vibrato";
	custom_ports[CUSTOM_MIDI_BEND_VIBRATO].max = 99;
	custom_ports[CUSTOM_MIDI_BEND_VIBRATO].step = 1;

	custom_ports[CUSTOM_MIDI_PITCH_BEND].name = "Pitch Bend";
	custom_ports[CUSTOM_MIDI_PITCH_BEND].identifier = "pitch_bend";
	custom_ports[CUSTOM_MIDI_PITCH_BEND].value = 0;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].min = -8192;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].max = 8191;
	custom_ports[CUSTOM_MIDI_PITCH_BEND].step = 1;

	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].name = "Bend Slide Up";
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].identifier = "bend_slide_up";
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].value = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].min = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].max = 1;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_UP].step = 0.001;

	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].name = "Bend Slide Down";
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].identifier = "bend_slide_down";
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].value = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].min = 0;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].max = 1;
	custom_ports[CUSTOM_MIDI_BEND_SLIDE_DOWN].step = 0.001;

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

	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].name = "Change Program";
	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].identifier = "change_program";
	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].value = 0;
	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].min = 0;
	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].max = 99;
	custom_ports[CUSTOM_MIDI_CHANGE_PROGRAM].step = 1;

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
	bend_portamento_last_note = -1;

	prev_bend_vibrato = 0;
	reset_pending = true;
	current_pitch_bend = 0;
	extra_pitch_bend = 0;
	prev_bend_portamento = false;
	update_pitch_bend_range = true;
	prev_bend_slide = false;
}
