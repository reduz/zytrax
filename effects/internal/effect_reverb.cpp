
#include "effect_reverb.h"
#include <math.h>

//process
bool AudioEffectReverb::has_secondary_input() const {
	return false;
}
void AudioEffectReverb::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	if (control_ports[CONTROL_PORT_PREDELAY_MSEC].was_set) {
		reverb[0].set_predelay(control_ports[CONTROL_PORT_PREDELAY_MSEC].value);
		reverb[1].set_predelay(control_ports[CONTROL_PORT_PREDELAY_MSEC].value);
		control_ports[CONTROL_PORT_PREDELAY_MSEC].was_set = false;
	}

	if (control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].was_set) {
		reverb[0].set_predelay_feedback(control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].value);
		reverb[1].set_predelay_feedback(control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].value);
		control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].was_set = false;
	}

	if (control_ports[CONTROL_PORT_ROOM_SIZE].was_set) {
		reverb[0].set_room_size(control_ports[CONTROL_PORT_ROOM_SIZE].value);
		reverb[1].set_room_size(control_ports[CONTROL_PORT_ROOM_SIZE].value);
		control_ports[CONTROL_PORT_ROOM_SIZE].was_set = false;
	}

	if (control_ports[CONTROL_PORT_DAMPING].was_set) {
		reverb[0].set_damp(control_ports[CONTROL_PORT_DAMPING].value);
		reverb[1].set_damp(control_ports[CONTROL_PORT_DAMPING].value);
		control_ports[CONTROL_PORT_DAMPING].was_set = false;
	}

	if (control_ports[CONTROL_PORT_SPREAD].was_set) {
		reverb[0].set_extra_spread(control_ports[CONTROL_PORT_SPREAD].value);
		reverb[1].set_extra_spread(control_ports[CONTROL_PORT_SPREAD].value);
		control_ports[CONTROL_PORT_SPREAD].was_set = false;
	}

	if (control_ports[CONTROL_PORT_DRY].was_set) {
		reverb[0].set_dry(control_ports[CONTROL_PORT_DRY].value);
		reverb[1].set_dry(control_ports[CONTROL_PORT_DRY].value);
		control_ports[CONTROL_PORT_DRY].was_set = false;
	}

	if (control_ports[CONTROL_PORT_WET].was_set) {
		printf("set wet: %f\n", control_ports[CONTROL_PORT_WET].value);
		reverb[0].set_wet(control_ports[CONTROL_PORT_WET].value);
		reverb[1].set_wet(control_ports[CONTROL_PORT_WET].value);
		control_ports[CONTROL_PORT_WET].was_set = false;
	}

	if (control_ports[CONTROL_PORT_HPF].was_set) {
		reverb[0].set_highpass(control_ports[CONTROL_PORT_HPF].value);
		reverb[1].set_highpass(control_ports[CONTROL_PORT_HPF].value);
		control_ports[CONTROL_PORT_HPF].was_set = false;
	}

	float *src_buf = &tmp_frames_src[0];
	float *dst_buf = &tmp_frames_dst[0];

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < block_size; j++) {
			src_buf[j] = p_in[j][i];
		}
		reverb[i].process(src_buf, dst_buf, block_size);
		for (int j = 0; j < block_size; j++) {
			p_out[j][i] = dst_buf[j];
		}
	}
}

void AudioEffectReverb::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectReverb::set_process_block_size(int p_size) {
	block_size = p_size;
	tmp_frames_src.resize(block_size);
	tmp_frames_dst.resize(block_size);
}
void AudioEffectReverb::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectReverb::get_name() const {
	return "Reverb";
}
String AudioEffectReverb::get_unique_id() const {
	return "reverb";
}
String AudioEffectReverb::get_provider_id() const {
	return "internal";
}

int AudioEffectReverb::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectReverb::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectReverb::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectReverb::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectReverb::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectReverb::_update_buffers() {
	reverb[0].set_mix_rate(sampling_rate);
	reverb[0].set_extra_spread_base(0);
	reverb[1].set_mix_rate(sampling_rate);
	reverb[1].set_extra_spread_base(0.000521);
}
AudioEffectReverb::AudioEffectReverb() {

	control_ports[CONTROL_PORT_PREDELAY_MSEC].name = "Predelay (msec)";
	control_ports[CONTROL_PORT_PREDELAY_MSEC].identifier = "predelay";
	control_ports[CONTROL_PORT_PREDELAY_MSEC].min = 20;
	control_ports[CONTROL_PORT_PREDELAY_MSEC].max = 500;
	control_ports[CONTROL_PORT_PREDELAY_MSEC].step = 1;
	control_ports[CONTROL_PORT_PREDELAY_MSEC].value = 150;

	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].name = "Predelay Feedback";
	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].identifier = "predelay_fbk";
	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].min = 0;
	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].max = 0.98;
	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].step = 0.01;
	control_ports[CONTROL_PORT_PREDELAY_FEEDBACK].value = 0.4;

	control_ports[CONTROL_PORT_ROOM_SIZE].name = "Room Size";
	control_ports[CONTROL_PORT_ROOM_SIZE].identifier = "room_size";
	control_ports[CONTROL_PORT_ROOM_SIZE].min = 0;
	control_ports[CONTROL_PORT_ROOM_SIZE].max = 1;
	control_ports[CONTROL_PORT_ROOM_SIZE].step = 0.01;
	control_ports[CONTROL_PORT_ROOM_SIZE].value = 0.8;

	control_ports[CONTROL_PORT_DAMPING].name = "Damping";
	control_ports[CONTROL_PORT_DAMPING].identifier = "damping";
	control_ports[CONTROL_PORT_DAMPING].min = 0;
	control_ports[CONTROL_PORT_DAMPING].max = 1;
	control_ports[CONTROL_PORT_DAMPING].step = 0.01;
	control_ports[CONTROL_PORT_DAMPING].value = 0.5;

	control_ports[CONTROL_PORT_SPREAD].name = "Spread";
	control_ports[CONTROL_PORT_SPREAD].identifier = "spread";
	control_ports[CONTROL_PORT_SPREAD].min = 0;
	control_ports[CONTROL_PORT_SPREAD].max = 1;
	control_ports[CONTROL_PORT_SPREAD].step = 0.01;
	control_ports[CONTROL_PORT_SPREAD].value = 1.0;

	control_ports[CONTROL_PORT_HPF].name = "High Pass Filter";
	control_ports[CONTROL_PORT_HPF].identifier = "hpf";
	control_ports[CONTROL_PORT_HPF].min = 0;
	control_ports[CONTROL_PORT_HPF].max = 1;
	control_ports[CONTROL_PORT_HPF].step = 0.01;
	control_ports[CONTROL_PORT_HPF].value = 0.2;

	control_ports[CONTROL_PORT_DRY].name = "Dry";
	control_ports[CONTROL_PORT_DRY].identifier = "dry";
	control_ports[CONTROL_PORT_DRY].min = 0;
	control_ports[CONTROL_PORT_DRY].max = 1;
	control_ports[CONTROL_PORT_DRY].step = 0.01;
	control_ports[CONTROL_PORT_DRY].value = 1;

	control_ports[CONTROL_PORT_WET].name = "Wet";
	control_ports[CONTROL_PORT_WET].identifier = "wet";
	control_ports[CONTROL_PORT_WET].min = 0;
	control_ports[CONTROL_PORT_WET].max = 1;
	control_ports[CONTROL_PORT_WET].step = 0.01;
	control_ports[CONTROL_PORT_WET].value = 0.5;

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		control_ports[i].was_set = true; //ensure updating
	}
}
AudioEffectReverb::~AudioEffectReverb() {
}
