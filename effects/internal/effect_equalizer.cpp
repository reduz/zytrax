#include "effect_equalizer.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectEqualizer::has_secondary_input() const {
	return false;
}

void AudioEffectEqualizer::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	int band_count = bands[0].size();
	EQ::BandProcess *proc_l = &bands[0][0];
	EQ::BandProcess *proc_r = &bands[1][0];

	float *bgain = &gains[0];
	for (int i = 0; i < band_count; i++) {
		bgain[i] = db2linear(control_ports[i].value);
	}

	for (int i = 0; i < block_size; i++) {

		AudioFrame src = p_in[i];
		AudioFrame dst = AudioFrame(0, 0);

		for (int j = 0; j < band_count; j++) {

			float l = src.l;
			float r = src.r;

			proc_l[j].process_one(l);
			proc_r[j].process_one(r);

			dst.l += l * bgain[j];
			dst.r += r * bgain[j];
		}

		p_out[i] = dst;
	}
}

void AudioEffectEqualizer::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectEqualizer::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectEqualizer::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectEqualizer::get_name() const {
	return "EQ (" + String::num(eq.get_band_count()) + " Band)";
}
String AudioEffectEqualizer::get_unique_id() const {
	return "eq_" + String::num(eq.get_band_count());
}
String AudioEffectEqualizer::get_provider_id() const {
	return "internal";
}

int AudioEffectEqualizer::get_control_port_count() const {
	return control_ports.size();
}
ControlPort *AudioEffectEqualizer::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, control_ports.size(), NULL);
	return &control_ports[p_port];
}

void AudioEffectEqualizer::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectEqualizer::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < control_ports.size(); i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectEqualizer::from_json(const JSON::Node &node) {

	for (int i = 0; i < control_ports.size(); i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectEqualizer::_update_buffers() {

	eq.set_mix_rate(sampling_rate);
	for (int i = 0; i < 2; i++) {
		bands[i].clear();
		for (int j = 0; j < eq.get_band_count(); j++) {
			bands[i].push_back(eq.get_band_processor(j));
		}
	}
	gains.resize(eq.get_band_count());
}
AudioEffectEqualizer::AudioEffectEqualizer(EQ::Preset p_preset) {

	eq.set_preset_band_mode(p_preset);

	for (int i = 0; i < eq.get_band_count(); i++) {

		ControlPortDefault port;
		port.name = String::num(eq.get_band_frequency(i), 0) + " hz (db)";
		port.identifier = "band_" + String::num(i);
		port.min = -48;
		port.max = 12;
		port.step = 0.1;
		port.value = 0;
		control_ports.push_back(port);
	}
	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectEqualizer::~AudioEffectEqualizer() {
}
