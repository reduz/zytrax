#include "effect_panner.h"

//process
bool AudioEffectPanner::has_secondary_input() const {
	return false;
}

void AudioEffectPanner::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {
	float lvol = CLAMP(1.0 - control_ports[CONTROL_PORT_PAN].value, 0, 1);
	float rvol = CLAMP(1.0 + control_ports[CONTROL_PORT_PAN].value, 0, 1);

	for (int i = 0; i < block_size; i++) {

		p_out[i].l = p_in[i].l * lvol + p_in[i].r * (1.0 - rvol);
		p_out[i].r = p_in[i].r * rvol + p_in[i].l * (1.0 - lvol);
	}
}

void AudioEffectPanner::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectPanner::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectPanner::set_sampling_rate(int p_hz) {
}
//info
String AudioEffectPanner::get_name() const {
	return "Panner";
}
String AudioEffectPanner::get_unique_id() const {
	return "panner";
}
String AudioEffectPanner::get_provider_id() const {
	return "internal";
}

int AudioEffectPanner::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectPanner::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectPanner::reset() {
}

/* Load/Save */

JSON::Node AudioEffectPanner::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectPanner::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

AudioEffectPanner::AudioEffectPanner() {

	control_ports[CONTROL_PORT_PAN].name = "Pan";
	control_ports[CONTROL_PORT_PAN].identifier = "pan";
	control_ports[CONTROL_PORT_PAN].min = -1;
	control_ports[CONTROL_PORT_PAN].max = 1;
	control_ports[CONTROL_PORT_PAN].step = 0.01;
	control_ports[CONTROL_PORT_PAN].value = 0;

	block_size = 128;
}
AudioEffectPanner::~AudioEffectPanner() {
}
