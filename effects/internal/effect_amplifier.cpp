#include "effect_amplifier.h"
#include "dsp/db.h"
//process
bool AudioEffectAmplifier::has_secondary_input() const {
	return false;
}

void AudioEffectAmplifier::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	float amp = db2linear(control_ports[CONTROL_PORT_AMPLIFY_DB].value);
	for (int i = 0; i < block_size; i++) {

		p_out[i].l = p_in[i].l * amp;
		p_out[i].r = p_in[i].r * amp;
	}
}

void AudioEffectAmplifier::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectAmplifier::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectAmplifier::set_sampling_rate(int p_hz) {
}
//info
String AudioEffectAmplifier::get_name() const {
	return "Amplifier";
}
String AudioEffectAmplifier::get_unique_id() const {
	return "amplifier";
}
String AudioEffectAmplifier::get_provider_id() const {
	return "internal";
}

int AudioEffectAmplifier::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectAmplifier::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectAmplifier::reset() {
}

/* Load/Save */

JSON::Node AudioEffectAmplifier::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectAmplifier::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

AudioEffectAmplifier::AudioEffectAmplifier() {

	control_ports[CONTROL_PORT_AMPLIFY_DB].name = "Amplify (db)";
	control_ports[CONTROL_PORT_AMPLIFY_DB].identifier = "amplify";
	control_ports[CONTROL_PORT_AMPLIFY_DB].min = -60;
	control_ports[CONTROL_PORT_AMPLIFY_DB].max = 24;
	control_ports[CONTROL_PORT_AMPLIFY_DB].step = 0.1;
	control_ports[CONTROL_PORT_AMPLIFY_DB].value = 0;

	block_size = 128;
}
AudioEffectAmplifier::~AudioEffectAmplifier() {
}
