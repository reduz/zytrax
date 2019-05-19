#include "effect_stereo_enhancer.h"

//process
bool AudioEffectStereoEnhancer::has_secondary_input() const {
	return false;
}

void AudioEffectStereoEnhancer::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	float intensity = control_ports[CONTROL_PORT_PAN_PULLOUT].value;
	bool surround_mode = control_ports[CONTROL_PORT_SURROUND].value > 0;
	float surround_amount = control_ports[CONTROL_PORT_SURROUND].value;
	unsigned int delay_frames = (control_ports[CONTROL_PORT_SURROUND].value / 1000.0) * sampling_rate;

	for (int i = 0; i < block_size; i++) {

		float l = p_in[i].l;
		float r = p_in[i].r;

		float center = (l + r) / 2.0f;

		l = (center + (l - center) * intensity);
		r = (center + (r - center) * intensity);

		if (surround_mode) {

			float val = (l + r) / 2.0;

			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;

			float out = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask] * surround_amount;

			l += out;
			r += -out;
		} else {

			float val = r;

			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;

			//r is delayed
			r = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask];
			;
		}

		p_out[i].l = l;
		p_out[i].r = r;
		ringbuff_pos++;
	}
}

void AudioEffectStereoEnhancer::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectStereoEnhancer::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectStereoEnhancer::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectStereoEnhancer::get_name() const {
	return "StereoEnh";
}
String AudioEffectStereoEnhancer::get_unique_id() const {
	return "stereo_enhancer";
}
String AudioEffectStereoEnhancer::get_provider_id() const {
	return "internal";
}

int AudioEffectStereoEnhancer::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectStereoEnhancer::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectStereoEnhancer::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectStereoEnhancer::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectStereoEnhancer::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectStereoEnhancer::_update_buffers() {

	float ring_buffer_max_size = MAX_DELAY_MS + 8; //pad
	ring_buffer_max_size /= 1000.0; //convert to seconds
	ring_buffer_max_size *= sampling_rate;

	int ringbuff_size = (int)ring_buffer_max_size;

	int bits = 0;

	while (ringbuff_size > 0) {
		bits++;
		ringbuff_size /= 2;
	}

	ringbuff_size = 1 << bits;
	ringbuff_mask = ringbuff_size - 1;
	ringbuff_pos = 0;

	if (delay_ringbuff) {
		delete[] delay_ringbuff;
	}
	delay_ringbuff = new float[ringbuff_size];
	for (int i = 0; i < ringbuff_size; i++) {
		delay_ringbuff[i] = 0;
	}
}

AudioEffectStereoEnhancer::AudioEffectStereoEnhancer() {

	control_ports[CONTROL_PORT_PAN_PULLOUT].name = "Pan Pullout";
	control_ports[CONTROL_PORT_PAN_PULLOUT].identifier = "pan_pullout";
	control_ports[CONTROL_PORT_PAN_PULLOUT].min = 0;
	control_ports[CONTROL_PORT_PAN_PULLOUT].max = 4;
	control_ports[CONTROL_PORT_PAN_PULLOUT].step = 0.01;
	control_ports[CONTROL_PORT_PAN_PULLOUT].value = 1;

	control_ports[CONTROL_PORT_TIME_PULLOUT].name = "Time Pullout";
	control_ports[CONTROL_PORT_TIME_PULLOUT].identifier = "time_pullout";
	control_ports[CONTROL_PORT_TIME_PULLOUT].min = 0;
	control_ports[CONTROL_PORT_TIME_PULLOUT].max = MAX_DELAY_MS;
	control_ports[CONTROL_PORT_TIME_PULLOUT].step = 0.1;
	control_ports[CONTROL_PORT_TIME_PULLOUT].value = 0;

	control_ports[CONTROL_PORT_SURROUND].name = "Surround";
	control_ports[CONTROL_PORT_SURROUND].identifier = "surround";
	control_ports[CONTROL_PORT_SURROUND].min = 0;
	control_ports[CONTROL_PORT_SURROUND].max = 1;
	control_ports[CONTROL_PORT_SURROUND].step = 0.01;
	control_ports[CONTROL_PORT_SURROUND].value = 0;

	delay_ringbuff = NULL;
	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectStereoEnhancer::~AudioEffectStereoEnhancer() {

	if (delay_ringbuff) {
		delete[] delay_ringbuff;
	}
}
