#include "effect_phaser.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectPhaser::has_secondary_input() const {
	return false;
}

void AudioEffectPhaser::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	float dmin = control_ports[CONTROL_PORT_RANGE_MIN_HZ].value / (sampling_rate / 2.0);
	float dmax = control_ports[CONTROL_PORT_RANGE_MAX_HZ].value / (sampling_rate / 2.0);

	float increment = 2.f * M_PI * (control_ports[CONTROL_PORT_RATE_HZ].value / sampling_rate);
	float depth = control_ports[CONTROL_PORT_DEPTH].value;
	float feedback = control_ports[CONTROL_PORT_FEEDBACK].value;

	for (int i = 0; i < block_size; i++) {

		phase += increment;

		while (phase >= M_PI * 2.f) {
			phase -= M_PI * 2.f;
		}

		float d = dmin + (dmax - dmin) * ((sin(phase) + 1.f) / 2.f);

		//update filter coeffs
		for (int j = 0; j < 6; j++) {
			allpass[0][j].delay(d);
			allpass[1][j].delay(d);
		}

		//calculate output
		float y = allpass[0][0].update(
				allpass[0][1].update(
						allpass[0][2].update(
								allpass[0][3].update(
										allpass[0][4].update(
												allpass[0][5].update(p_in[i].l + h.l * feedback))))));
		h.l = y;

		p_out[i].l = p_in[i].l + y * depth;

		y = allpass[1][0].update(
				allpass[1][1].update(
						allpass[1][2].update(
								allpass[1][3].update(
										allpass[1][4].update(
												allpass[1][5].update(p_in[i].r + h.r * feedback))))));
		h.r = y;

		p_out[i].r = p_in[i].r + y * depth;
	}
}

void AudioEffectPhaser::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectPhaser::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectPhaser::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectPhaser::get_name() const {
	return "Phaser";
}
String AudioEffectPhaser::get_unique_id() const {
	return "phaser";
}
String AudioEffectPhaser::get_provider_id() const {
	return "internal";
}

int AudioEffectPhaser::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectPhaser::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectPhaser::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectPhaser::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectPhaser::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectPhaser::_update_buffers() {

	h = AudioFrame(0, 0);
	phase = 0;
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 6; j++) {
			allpass[i][j] = AllpassDelay();
		}
	}
}
AudioEffectPhaser::AudioEffectPhaser() {

	control_ports[CONTROL_PORT_RANGE_MIN_HZ].name = "Range Min (hz)";
	control_ports[CONTROL_PORT_RANGE_MIN_HZ].identifier = "range_min";
	control_ports[CONTROL_PORT_RANGE_MIN_HZ].min = 10;
	control_ports[CONTROL_PORT_RANGE_MIN_HZ].max = 10000;
	control_ports[CONTROL_PORT_RANGE_MIN_HZ].step = 1;
	control_ports[CONTROL_PORT_RANGE_MIN_HZ].value = 440;

	control_ports[CONTROL_PORT_RANGE_MAX_HZ].name = "Range Max (hz)";
	control_ports[CONTROL_PORT_RANGE_MAX_HZ].identifier = "range_max";
	control_ports[CONTROL_PORT_RANGE_MAX_HZ].min = 10;
	control_ports[CONTROL_PORT_RANGE_MAX_HZ].max = 10000;
	control_ports[CONTROL_PORT_RANGE_MAX_HZ].step = 1;
	control_ports[CONTROL_PORT_RANGE_MAX_HZ].value = 1600;

	control_ports[CONTROL_PORT_RATE_HZ].name = "Rate (hz)";
	control_ports[CONTROL_PORT_RATE_HZ].identifier = "rate";
	control_ports[CONTROL_PORT_RATE_HZ].min = 0.01;
	control_ports[CONTROL_PORT_RATE_HZ].max = 20;
	control_ports[CONTROL_PORT_RATE_HZ].step = 0.01;
	control_ports[CONTROL_PORT_RATE_HZ].value = 0.5;

	control_ports[CONTROL_PORT_FEEDBACK].name = "Feedback";
	control_ports[CONTROL_PORT_FEEDBACK].identifier = "feedback";
	control_ports[CONTROL_PORT_FEEDBACK].min = 0.1;
	control_ports[CONTROL_PORT_FEEDBACK].max = 0.9;
	control_ports[CONTROL_PORT_FEEDBACK].step = 0.1;
	control_ports[CONTROL_PORT_FEEDBACK].value = 0.7;

	control_ports[CONTROL_PORT_DEPTH].name = "Depth";
	control_ports[CONTROL_PORT_DEPTH].identifier = "depth";
	control_ports[CONTROL_PORT_DEPTH].min = 0.1;
	control_ports[CONTROL_PORT_DEPTH].max = 4;
	control_ports[CONTROL_PORT_DEPTH].step = 0.1;
	control_ports[CONTROL_PORT_DEPTH].value = 1;

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectPhaser::~AudioEffectPhaser() {
}
