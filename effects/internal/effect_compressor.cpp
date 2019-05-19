#include "effect_compressor.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectCompressor::has_secondary_input() const {
	return sidechain;
}

void AudioEffectCompressor::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {
	process_with_secondary(p_events, p_event_count, p_in, NULL, p_out, p_prev_active);
}

void AudioEffectCompressor::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {

	float threshold = db2linear(control_ports[CONTROL_PORT_THRESHOLD_DB].value);
	float sample_rate = sampling_rate;

	float ratatcoef = exp(-1 / (0.00001f * sample_rate));
	float ratrelcoef = exp(-1 / (0.5f * sample_rate));
	float attime = control_ports[CONTROL_PORT_ATTACK_MS].value / 1000.0;
	float reltime = control_ports[CONTROL_PORT_RELEASE_MS].value / 1000.0;
	float atcoef = exp(-1 / (attime * sample_rate));
	float relcoef = exp(-1 / (reltime * sample_rate));

	float pre = db2linear(control_ports[CONTROL_PORT_PRE_GAIN_DB].value);
	float makeup = db2linear(control_ports[CONTROL_PORT_POST_GAIN_DB].value);

	float mix = control_ports[CONTROL_PORT_MIX].value;
	float gr_meter_decay = exp(1 / (1 * sample_rate));

	float ratio = control_ports[CONTROL_PORT_RATIO].value;

	for (int i = 0; i < block_size; i++) {

		AudioFrame s = p_in[i];
		s *= pre;

		//convert to positive
		s.l = abs(s.l);
		s.r = abs(s.r);

		float peak = MAX(s.l, s.r);

		float overdb = 2.08136898f * linear2db(peak / threshold);

		if (overdb < 0.0) //we only care about what goes over to compress
			overdb = 0.0;

		if (overdb - rundb > 5) // diffeence is too large
			averatio = 4;

		if (overdb > rundb) {
			rundb = overdb + atcoef * (rundb - overdb);
			runratio = averatio + ratatcoef * (runratio - averatio);
		} else {
			rundb = overdb + relcoef * (rundb - overdb);
			runratio = averatio + ratrelcoef * (runratio - averatio);
		}

		overdb = rundb;
		averatio = runratio;

		float cratio;

		if (false) { //rato all-in
			cratio = 12 + averatio;
		} else {
			cratio = ratio;
		}

		float gr = -overdb * (cratio - 1) / cratio;
		float grv = db2linear(gr);

		runmax = maxover + relcoef * (runmax - maxover); // highest peak for setting att/rel decays in reltime
		maxover = runmax;

		if (grv < gr_meter) {
			gr_meter = grv;
		} else {
			gr_meter *= gr_meter_decay;
			if (gr_meter > 1)
				gr_meter = 1;
		}

		if (p_secondary) {
			p_out[i] = p_secondary[i] * grv * makeup * mix + p_secondary[i] * (1.0 - mix);
			p_out[i] += p_in[i];
		} else {
			p_out[i] = p_in[i] * grv * makeup * mix + p_in[i] * (1.0 - mix);
		}
	}
}

void AudioEffectCompressor::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectCompressor::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectCompressor::get_name() const {
	return sidechain ? "Compressor (SideC)" : "Compressor";
}
String AudioEffectCompressor::get_unique_id() const {
	return sidechain ? "sc_compressor" : "compressor";
}
String AudioEffectCompressor::get_provider_id() const {
	return "internal";
}

int AudioEffectCompressor::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectCompressor::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectCompressor::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectCompressor::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectCompressor::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectCompressor::_update_buffers() {

	rundb = 0;
	runratio = 0;
	averatio = 0;
	runmax = 0;
	maxover = 0;
	gr_meter = 1.0;
	current_channel = -1;
}

AudioEffectCompressor::AudioEffectCompressor(bool p_sidechain) {

	sidechain = p_sidechain;
	control_ports[CONTROL_PORT_PRE_GAIN_DB].name = "Pre Gain (db)";
	control_ports[CONTROL_PORT_PRE_GAIN_DB].identifier = "pre_gain";
	control_ports[CONTROL_PORT_PRE_GAIN_DB].min = -24;
	control_ports[CONTROL_PORT_PRE_GAIN_DB].max = 24;
	control_ports[CONTROL_PORT_PRE_GAIN_DB].step = 0.1;
	control_ports[CONTROL_PORT_PRE_GAIN_DB].value = 0;

	control_ports[CONTROL_PORT_THRESHOLD_DB].name = "Threshold (db)";
	control_ports[CONTROL_PORT_THRESHOLD_DB].identifier = "threshold";
	control_ports[CONTROL_PORT_THRESHOLD_DB].min = -60;
	control_ports[CONTROL_PORT_THRESHOLD_DB].max = 0;
	control_ports[CONTROL_PORT_THRESHOLD_DB].step = 0.1;
	control_ports[CONTROL_PORT_THRESHOLD_DB].value = -6;

	control_ports[CONTROL_PORT_RATIO].name = "Ratio";
	control_ports[CONTROL_PORT_RATIO].identifier = "ratio";
	control_ports[CONTROL_PORT_RATIO].max = 48;
	control_ports[CONTROL_PORT_RATIO].step = 0.1;
	control_ports[CONTROL_PORT_RATIO].value = 4;

	control_ports[CONTROL_PORT_ATTACK_MS].name = "Attack (ms)";
	control_ports[CONTROL_PORT_ATTACK_MS].identifier = "attack";
	control_ports[CONTROL_PORT_ATTACK_MS].min = 0.01;
	control_ports[CONTROL_PORT_ATTACK_MS].max = 20;
	control_ports[CONTROL_PORT_ATTACK_MS].step = 0.01;
	control_ports[CONTROL_PORT_ATTACK_MS].value = 0.2;

	control_ports[CONTROL_PORT_RELEASE_MS].name = "Release (ms)";
	control_ports[CONTROL_PORT_RELEASE_MS].identifier = "release";
	control_ports[CONTROL_PORT_RELEASE_MS].min = 20;
	control_ports[CONTROL_PORT_RELEASE_MS].max = 2000;
	control_ports[CONTROL_PORT_RELEASE_MS].step = 0.1;
	control_ports[CONTROL_PORT_RELEASE_MS].value = 250;

	control_ports[CONTROL_PORT_POST_GAIN_DB].name = "Post Gain (db)";
	control_ports[CONTROL_PORT_POST_GAIN_DB].identifier = "post_gain";
	control_ports[CONTROL_PORT_POST_GAIN_DB].min = -60;
	control_ports[CONTROL_PORT_POST_GAIN_DB].max = 24;
	control_ports[CONTROL_PORT_POST_GAIN_DB].step = 0.1;
	control_ports[CONTROL_PORT_POST_GAIN_DB].value = 0;

	control_ports[CONTROL_PORT_MIX].name = "Mix";
	control_ports[CONTROL_PORT_MIX].identifier = "mix";
	control_ports[CONTROL_PORT_MIX].max = 1;
	control_ports[CONTROL_PORT_MIX].step = 0.01;
	control_ports[CONTROL_PORT_MIX].value = 1;

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectCompressor::~AudioEffectCompressor() {
}
