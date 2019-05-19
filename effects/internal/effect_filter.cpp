#include "effect_filter.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectFilter::has_secondary_input() const {
	return false;
}

template <int S>
void AudioEffectFilter::_process_filter(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {

	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].l;
		filter_process[0][0].process_one(f);
		if (S > 1)
			filter_process[0][1].process_one(f);
		if (S > 2)
			filter_process[0][2].process_one(f);
		if (S > 3)
			filter_process[0][3].process_one(f);

		p_dst_frames[i].l = f;
	}

	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].r;
		filter_process[1][0].process_one(f);
		if (S > 1)
			filter_process[1][1].process_one(f);
		if (S > 2)
			filter_process[1][2].process_one(f);
		if (S > 3)
			filter_process[1][3].process_one(f);

		p_dst_frames[i].r = f;
	}
}

void AudioEffectFilter::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	filter.set_cutoff(control_ports[CONTROL_PORT_CUTOFF_HZ].value);
	filter.set_gain(control_ports[CONTROL_PORT_GAIN].value);
	filter.set_resonance(control_ports[CONTROL_PORT_RESONANCE].value);
	filter.set_mode(filter_mode);
	int stages = int(control_ports[CONTROL_PORT_DB].value) + 1;
	filter.set_stages(stages);
	filter.set_sampling_rate(sampling_rate);

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].update_coeffs();
		}
	}

	if (stages == 1) {
		_process_filter<1>(p_in, p_out, block_size);
	} else if (stages == 2) {
		_process_filter<2>(p_in, p_out, block_size);
	} else if (stages == 3) {
		_process_filter<3>(p_in, p_out, block_size);
	} else if (stages == 4) {
		_process_filter<4>(p_in, p_out, block_size);
	}
}

void AudioEffectFilter::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectFilter::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectFilter::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectFilter::get_name() const {
	static const char *mode_names[] = { "BandPass", "HighPass", "LowPass", "Notch", "Peak", "BandLimit", "LowShelf", "HighShelf" };
	return mode_names[filter_mode];
}
String AudioEffectFilter::get_unique_id() const {
	static const char *mode_names[] = { "filter_band_pass", "filter_high_pass", "filter_low_pass", "filter_notch", "filter_peak", "filter_band_limit", "filter_low_shelf", "filter_high_shelf" };
	return mode_names[filter_mode];
}
String AudioEffectFilter::get_provider_id() const {
	return "internal";
}

int AudioEffectFilter::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectFilter::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectFilter::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectFilter::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectFilter::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectFilter::_update_buffers() {

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].clear();
		}
	}
}
AudioEffectFilter::AudioEffectFilter(Filter::Mode p_filter_mode) {

	filter_mode = p_filter_mode;

	control_ports[CONTROL_PORT_CUTOFF_HZ].name = "Cutoff (hz)";
	control_ports[CONTROL_PORT_CUTOFF_HZ].identifier = "cutoff";
	control_ports[CONTROL_PORT_CUTOFF_HZ].min = 10;
	control_ports[CONTROL_PORT_CUTOFF_HZ].max = 20000;
	control_ports[CONTROL_PORT_CUTOFF_HZ].step = 1;
	control_ports[CONTROL_PORT_CUTOFF_HZ].value = 2000;

	control_ports[CONTROL_PORT_RESONANCE].name = "Resonance";
	control_ports[CONTROL_PORT_RESONANCE].identifier = "resonance";
	control_ports[CONTROL_PORT_RESONANCE].min = 0;
	control_ports[CONTROL_PORT_RESONANCE].max = 1;
	control_ports[CONTROL_PORT_RESONANCE].step = 0.01;
	control_ports[CONTROL_PORT_RESONANCE].value = 0.5;

	control_ports[CONTROL_PORT_GAIN].name = "Gain";
	control_ports[CONTROL_PORT_GAIN].identifier = "gain";
	control_ports[CONTROL_PORT_GAIN].min = 0;
	control_ports[CONTROL_PORT_GAIN].max = 4;
	control_ports[CONTROL_PORT_GAIN].step = 0.01;
	control_ports[CONTROL_PORT_GAIN].value = 1.0;

	if (filter_mode == Filter::LOWPASS || filter_mode == Filter::HIGHPASS || filter_mode == Filter::BANDPASS) {
		control_ports[CONTROL_PORT_GAIN].visible = false;
	}

	control_ports[CONTROL_PORT_DB].name = "Strength";
	control_ports[CONTROL_PORT_DB].identifier = "strength";
	control_ports[CONTROL_PORT_DB].min = 0;
	control_ports[CONTROL_PORT_DB].max = 3;
	control_ports[CONTROL_PORT_DB].step = 1;
	control_ports[CONTROL_PORT_DB].value = 0;
	control_ports[CONTROL_PORT_DB].hint = ControlPort::HINT_ENUM;
	control_ports[CONTROL_PORT_DB].enum_values.push_back("6 dB");
	control_ports[CONTROL_PORT_DB].enum_values.push_back("12 dB");
	control_ports[CONTROL_PORT_DB].enum_values.push_back("18 dB");
	control_ports[CONTROL_PORT_DB].enum_values.push_back("24 dB");

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].set_filter(&filter);
		}
	}
	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectFilter::~AudioEffectFilter() {
}
