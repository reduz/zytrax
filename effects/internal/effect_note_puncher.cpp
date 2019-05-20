#include "effect_note_puncher.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectNotePuncher::has_secondary_input() const {
	return false;
}

void AudioEffectNotePuncher::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {
	process_with_secondary(p_events, p_event_count, p_in, NULL, p_out, p_prev_active);
}

void AudioEffectNotePuncher::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {

	int attack_samples = (control_ports[CONTROL_PORT_ATTACK_MS].value / 1000.0) * sampling_rate;
	int decay_samples = (control_ports[CONTROL_PORT_DECAY_MS].value / 1000.0) * sampling_rate;
	float amplify_db = control_ports[CONTROL_PORT_PUNCH_DB].value;

	//check notes
	for (int i = 0; i < p_event_count; i++) {
		if (p_events[i].type == Event::TYPE_NOTE) {
			//launch an envelope
			for (int j = 0; j < MAX_PUNCHES; j++) {
				if (!punches[j].active) {
					punches[j].active = true;
					punches[j].time = 0;
					break;
				}
			}
		}
	}

	//process envelope

	float *envelope = &envelope_block[0];

	for (int i = 0; i < block_size; i++) {
		envelope[i] = -1.0; // if negative, this was unused (will convert to 1.0 later)
	}
	for (int j = 0; j < MAX_PUNCHES; j++) {
		if (!punches[j].active) {
			continue;
		}
		for (int i = 0; i < block_size; i++) {

			float amp = 1.0;
			if (punches[j].time < attack_samples) {
				amp = float(punches[j].time) / float(attack_samples);
			} else {
				int time = punches[j].time - attack_samples;
				if (time < decay_samples) {
					amp = 1.0 - float(time) / float(decay_samples);
				} else {
					punches[j].active = false;
					break;
				}
			}

			amp = db2linear(amplify_db * amp);

			//printf("%i att %i dec %i - %f\n", punches[j].time, attack_samples, decay_samples, amp);

			envelope[i] = MAX(envelope[i], amp);
			punches[j].time++;
		}
	}

	for (int i = 0; i < block_size; i++) {
		float amp = envelope[i] >= 0 ? envelope[i] : 1.0;
		p_out[i] = p_in[i] * amp;
	}
}

void AudioEffectNotePuncher::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectNotePuncher::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectNotePuncher::get_name() const {
	return "NotePuncher";
}
String AudioEffectNotePuncher::get_unique_id() const {
	return "note_puncher";
}
String AudioEffectNotePuncher::get_provider_id() const {
	return "internal";
}

int AudioEffectNotePuncher::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectNotePuncher::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectNotePuncher::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectNotePuncher::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectNotePuncher::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectNotePuncher::_update_buffers() {

	envelope_block.resize(block_size);
	for (int i = 0; i < MAX_PUNCHES; i++) {
		punches[i].active = false;
		punches[i].time = 0;
	}
}

AudioEffectNotePuncher::AudioEffectNotePuncher() {

	control_ports[CONTROL_PORT_PUNCH_DB].name = "Punch Strength (db)";
	control_ports[CONTROL_PORT_PUNCH_DB].identifier = "punch_strength";
	control_ports[CONTROL_PORT_PUNCH_DB].min = -24;
	control_ports[CONTROL_PORT_PUNCH_DB].max = 24;
	control_ports[CONTROL_PORT_PUNCH_DB].step = 0.1;
	control_ports[CONTROL_PORT_PUNCH_DB].value = 6;

	control_ports[CONTROL_PORT_ATTACK_MS].name = "Attack (ms)";
	control_ports[CONTROL_PORT_ATTACK_MS].identifier = "attack";
	control_ports[CONTROL_PORT_ATTACK_MS].min = 0.1;
	control_ports[CONTROL_PORT_ATTACK_MS].max = 20;
	control_ports[CONTROL_PORT_ATTACK_MS].step = 0.1;
	control_ports[CONTROL_PORT_ATTACK_MS].value = 5;

	control_ports[CONTROL_PORT_DECAY_MS].name = "Decay (ms)";
	control_ports[CONTROL_PORT_DECAY_MS].identifier = "decay";
	control_ports[CONTROL_PORT_DECAY_MS].min = 5;
	control_ports[CONTROL_PORT_DECAY_MS].max = 200;
	control_ports[CONTROL_PORT_DECAY_MS].step = 0.1;
	control_ports[CONTROL_PORT_DECAY_MS].value = 50;

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectNotePuncher::~AudioEffectNotePuncher() {
}
