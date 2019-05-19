#include "effect_delay.h"

#include "dsp/db.h"
#include <math.h>
//process
bool AudioEffectDelay::has_secondary_input() const {
	return false;
}

void AudioEffectDelay::_process_chunk(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {

	float mix_rate = sampling_rate;
	float tap_level_f[MAX_TAPS];
	AudioFrame tap_pan_f[MAX_TAPS];
	unsigned int tap_delay_frames[MAX_TAPS];
	float tap_feedback_level_f[MAX_TAPS];
	float tap_feedback_stereo[MAX_TAPS];
	float lpf_c[MAX_TAPS];
	float lpf_ic[MAX_TAPS];
	AudioFrame *fb_buf[MAX_TAPS];
	bool enabled[MAX_TAPS];

	int stride = CONTROL_PORT_TAP2_ENABLED - CONTROL_PORT_TAP1_ENABLED;

	for (int i = 0; i < MAX_TAPS; i++) {
		int ofs = i * stride;

		enabled[i] = control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].value > 0.5;
		float levelf = control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].value;

		tap_level_f[i] = levelf;
		float pan = control_ports[CONTROL_PORT_TAP1_PAN + ofs].value;
		tap_pan_f[i].l = CLAMP(1.0 - pan, 0, 1);
		tap_pan_f[i].r = CLAMP(1.0 + pan, 0, 1);

		float delay_ms;
		if (bpm_sync) {
			int subdiv_idx = control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].value;
			if (subdiv_idx < 0) {
				subdiv_idx = 0;
			}
			if (subdiv_idx >= BEAT_SUBDIV_MAX) {
				subdiv_idx = BEAT_SUBDIV_MAX - 1;
			}
			static const float subdiv_values[BEAT_SUBDIV_MAX] = { 1, 2, 3, 4, 6, 8, 12, 16 };
			delay_ms = ((60000.0 / bpm) / subdiv_values[subdiv_idx]) * control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].value;
		} else {
			delay_ms = control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].value;
		}

		tap_delay_frames[i] = int((delay_ms / 1000.0) * mix_rate);
		tap_feedback_level_f[i] = control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].value;
		tap_feedback_stereo[i] = control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].value;
		lpf_c[i] = expf(-2.0 * M_PI * control_ports[CONTROL_PORT_TAP1_LPF + ofs].value / mix_rate);
		lpf_ic[i] = 1.0 - lpf_c[i];
		fb_buf[i] = &taps[i].feedback_buffer[0];
	}

	const AudioFrame *src = p_src_frames;
	AudioFrame *dst = p_dst_frames;
	AudioFrame *rb_buf = &ring_buffer[0];

	for (int i = 0; i < p_frame_count; i++) {

		rb_buf[ring_buffer_pos & ring_buffer_mask] = src[i];

		AudioFrame out = src[i];

		for (int t = 0; t < MAX_TAPS; t++) {

			if (!enabled[t]) {
				continue;
			}

			AudioFrame tap_in = rb_buf[uint32_t(ring_buffer_pos - tap_delay_frames[t]) & ring_buffer_mask] * tap_pan_f[t];
			tap_in += fb_buf[t][taps[t].feedback_buffer_pos];
			tap_in *= tap_level_f[t];
			tap_in = tap_in * lpf_ic[t] + taps[t].h * lpf_c[t];
			taps[t].h = tap_in;

			out += tap_in;

			AudioFrame fb_in = tap_in * tap_feedback_level_f[t];
			fb_in.undenormalise();

			//fb stereo
			AudioFrame fb_in_aux = fb_in;
			fb_in.l = tap_feedback_stereo[t] * fb_in_aux.r + (1.0 - tap_feedback_stereo[t]) * fb_in_aux.l;
			fb_in.r = tap_feedback_stereo[t] * fb_in_aux.l + (1.0 - tap_feedback_stereo[t]) * fb_in_aux.r;

			fb_buf[t][taps[t].feedback_buffer_pos] = fb_in;

			if ((++taps[t].feedback_buffer_pos) >= tap_delay_frames[t]) {
				taps[t].feedback_buffer_pos = 0;
			}
		}

		dst[i] = out;

		ring_buffer_pos++;
	}
}

void AudioEffectDelay::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	for (int i = 0; i < p_event_count; i++) {
		if (p_events[i].type == Event::TYPE_BPM) {
			bpm = p_events[i].param8;
		}
	}
	int todo = block_size;

	while (todo) {

		int to_mix = MIN(todo, 256); //can't mix too much

		_process_chunk(p_in, p_out, to_mix);

		p_in += to_mix;
		p_out += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectDelay::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectDelay::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectDelay::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectDelay::get_name() const {
	return bpm_sync ? "Delay (BPM)" : "Delay";
}
String AudioEffectDelay::get_unique_id() const {
	return bpm_sync ? "bpm_delay" : "delay";
}
String AudioEffectDelay::get_provider_id() const {
	return "internal";
}

int AudioEffectDelay::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectDelay::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectDelay::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectDelay::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectDelay::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectDelay::_update_buffers() {

	float ring_buffer_max_size = MAX_DELAY_MS + 100; //add 100ms of extra room, just in case
	ring_buffer_max_size /= 1000.0; //convert to seconds
	ring_buffer_max_size *= sampling_rate;

	int ringbuff_size = ring_buffer_max_size;

	int bits = 0;

	while (ringbuff_size > 0) {
		bits++;
		ringbuff_size /= 2;
	}

	ringbuff_size = 1 << bits;
	ring_buffer_mask = ringbuff_size - 1;
	ring_buffer_pos = 0;

	ring_buffer.resize(ringbuff_size);
	for (int i = 0; i < MAX_TAPS; i++) {
		taps[i].feedback_buffer.resize(ringbuff_size);
		taps[i].feedback_buffer_pos = 0;
		taps[i].h = AudioFrame(0, 0);
	}
}

AudioEffectDelay::AudioEffectDelay(bool p_bpm_sync) {

	bpm_sync = p_bpm_sync;
	int stride = CONTROL_PORT_TAP2_ENABLED - CONTROL_PORT_TAP1_ENABLED;

	for (int i = 0; i < 4; i++) {

		String pp = "Tap " + String::num(i + 1) + " ";
		String ppid = "tap_" + String::num(i + 0) + "_";

		int ofs = i * stride;
		control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].name = pp + "Enabled";
		control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].identifier = ppid + "enabled";
		control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].hint = ControlPort::HINT_TOGGLE;
		control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].max = 1;
		control_ports[CONTROL_PORT_TAP1_ENABLED + ofs].value = i == 0 ? 1 : 0;

		static const char *beat_subdiv_str[BEAT_SUBDIV_MAX] = {
			"1 Beat", "1/2 Beat", "1/3 Beat", "1/4 Beat", "1/6 Beat", "1/8 Beat", "1/12 Beat", "1/16 Beat"
		};
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].name = pp + "Unit Size";
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].identifier = ppid + "unit_size";
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].max = BEAT_SUBDIV_MAX - 1;
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].value = 1;
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].step = 1;
		control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].hint = ControlPort::HINT_ENUM;

		for (int j = 0; j < BEAT_SUBDIV_MAX; j++) {
			control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].enum_values.push_back(beat_subdiv_str[j]);
		}
		if (!bpm_sync) {
			control_ports[CONTROL_PORT_TAP1_BEAT_SUBDIV + ofs].visible = false;
		}

		if (bpm_sync) {
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].name = pp + "Delay (units)";
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].identifier = ppid + "delay";
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].min = 1;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].max = MAX_DELAY_UNITS;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].value = 1 + i;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].step = 1;
		} else {
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].name = pp + "Delay (ms)";
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].identifier = ppid + "delay";
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].min = 1;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].max = MAX_DELAY_MS;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].value = 100 + 100 * i;
			control_ports[CONTROL_PORT_TAP1_DELAY_UNITS + ofs].step = 1;
		}

		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].name = pp + "Volume";
		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].identifier = ppid + "volume";
		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].min = 0;
		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].max = 0.99;
		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].value = 0.8;
		control_ports[CONTROL_PORT_TAP1_VOLUME + ofs].step = 0.01;

		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].name = pp + "Feedback";
		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].identifier = ppid + "feedback";
		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].min = 0;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].max = 1;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].value = 0;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK + ofs].step = 0.01;

		control_ports[CONTROL_PORT_TAP1_LPF + ofs].name = pp + "Feedback LPF (hz)";
		control_ports[CONTROL_PORT_TAP1_LPF + ofs].identifier = ppid + "feedback_lpf";
		control_ports[CONTROL_PORT_TAP1_LPF + ofs].min = 1;
		control_ports[CONTROL_PORT_TAP1_LPF + ofs].max = MS_CUTOFF_MAX;
		control_ports[CONTROL_PORT_TAP1_LPF + ofs].value = 8000;
		control_ports[CONTROL_PORT_TAP1_LPF + ofs].step = 1;

		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].name = pp + "Feedback Stereo Swap";
		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].identifier = ppid + "feedback_sswap";
		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].min = 0;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].max = 1;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].value = 0;
		control_ports[CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP + ofs].step = 0.01;

		control_ports[CONTROL_PORT_TAP1_PAN + ofs].name = pp + "Pan";
		control_ports[CONTROL_PORT_TAP1_PAN + ofs].identifier = ppid + "pan";
		control_ports[CONTROL_PORT_TAP1_PAN + ofs].min = -1;
		control_ports[CONTROL_PORT_TAP1_PAN + ofs].max = 1;
		control_ports[CONTROL_PORT_TAP1_PAN + ofs].value = 0;
		control_ports[CONTROL_PORT_TAP1_PAN + ofs].step = 0.01;
	}

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
	bpm = 125;
}
AudioEffectDelay::~AudioEffectDelay() {
}
