#include "effect_chorus.h"
#include "dsp/db.h"
#include <math.h>

//process
bool AudioEffectChorus::has_secondary_input() const {
	return false;
}

void AudioEffectChorus::_process_chunk(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {

	float base_wet = control_ports[CONTROL_PORT_WET].value;
	float base_dry = control_ports[CONTROL_PORT_DRY].value;

	//fill ringbuffer
	for (int i = 0; i < p_frame_count; i++) {
		audio_buffer[(buffer_pos + i) & buffer_mask] = p_src_frames[i];
		p_dst_frames[i] = p_src_frames[i] * base_dry;
	}

	float mix_rate = sampling_rate;

	/* process voices */
	int stride = CONTROL_PORT_VOICE2_ENABLED - CONTROL_PORT_VOICE1_ENABLED;

	for (int vc = 0; vc < 4; vc++) {
		int ofs = vc * stride;
		if (control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].value < 0.5) {
			continue; //voice disabled;
		}

		float v_rate = control_ports[CONTROL_PORT_VOICE1_HZ + ofs].value;
		float v_delay = control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].value;
		float v_depth = control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].value;
		float v_cutoff = control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].value;
		float v_pan = control_ports[CONTROL_PORT_VOICE1_PAN + ofs].value;
		float v_level = control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].value;

		double time_to_mix = (float)p_frame_count / mix_rate;
		double cycles_to_mix = time_to_mix * v_rate;

		unsigned int local_rb_pos = buffer_pos;
		AudioFrame *dst_buff = p_dst_frames;
		AudioFrame *rb_buff = &audio_buffer[0];

		double delay_msec = v_delay;
		unsigned int delay_frames = llrint((delay_msec / 1000.0) * mix_rate);
		float max_depth_frames = (v_depth / 1000.0) * mix_rate;

		uint64_t local_cycles = cycles[vc];
		uint64_t increment = llrint(cycles_to_mix / (double)p_frame_count * (double)(1 << AudioEffectChorus::CYCLES_FRAC));

		//check the LFO doesn't read ahead of the write pos
		if ((((unsigned int)max_depth_frames) + 10) > delay_frames) { //10 as some threshold to avoid precision stuff
			delay_frames += (int)max_depth_frames - delay_frames;
			delay_frames += 10; //threshold to avoid precision stuff
		}

		//low pass filter
		if (v_cutoff == 0)
			continue;
		float auxlp = expf(-2.0 * M_PI * v_cutoff / mix_rate);
		float c1 = 1.0 - auxlp;
		float c2 = auxlp;
		AudioFrame h = filter_h[vc];
		if (v_cutoff >= AudioEffectChorus::MS_CUTOFF_MAX) {
			c1 = 1.0;
			c2 = 0.0;
		}

		//vol modifier

		AudioFrame vol_modifier = AudioFrame(base_wet, base_wet) * db2linear(v_level);
		vol_modifier.l *= CLAMP(1.0 - v_pan, 0, 1);
		vol_modifier.r *= CLAMP(1.0 + v_pan, 0, 1);

		for (int i = 0; i < p_frame_count; i++) {

			/** COMPUTE WAVEFORM **/

			float phase = (float)(local_cycles & AudioEffectChorus::CYCLES_MASK) / (float)(1 << AudioEffectChorus::CYCLES_FRAC);

			float wave_delay = sinf(phase * 2.0 * M_PI) * max_depth_frames;

			int wave_delay_frames = lrint(floor(wave_delay));
			float wave_delay_frac = wave_delay - (float)wave_delay_frames;

			/** COMPUTE RINGBUFFER POS**/

			unsigned int rb_source = local_rb_pos;
			rb_source -= delay_frames;

			rb_source -= wave_delay_frames;

			/** READ FROM RINGBUFFER, LINEARLY INTERPOLATE */

			AudioFrame val = rb_buff[rb_source & buffer_mask];
			AudioFrame val_next = rb_buff[(rb_source - 1) & buffer_mask];

			val += (val_next - val) * wave_delay_frac;

			val = val * c1 + h * c2;
			h = val;

			/** MIX VALUE TO OUTPUT **/

			dst_buff[i] += val * vol_modifier;

			local_cycles += increment;
			local_rb_pos++;
		}

		filter_h[vc] = h;
		cycles[vc] += lrint(cycles_to_mix * (double)(1 << AudioEffectChorus::CYCLES_FRAC));
	}

	buffer_pos += p_frame_count;
}

void AudioEffectChorus::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	int todo = block_size;

	while (todo) {

		int to_mix = MIN(todo, 256); //can't mix too much

		_process_chunk(p_in, p_out, to_mix);

		p_in += to_mix;
		p_out += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectChorus::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectChorus::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectChorus::set_sampling_rate(int p_hz) {
	if (sampling_rate != p_hz) {
		sampling_rate = p_hz;
		_update_buffers();
	}
}
//info
String AudioEffectChorus::get_name() const {
	return "Chorus";
}
String AudioEffectChorus::get_unique_id() const {
	return "chorus";
}
String AudioEffectChorus::get_provider_id() const {
	return "internal";
}

int AudioEffectChorus::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectChorus::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectChorus::reset() {
	_update_buffers();
}

/* Load/Save */

JSON::Node AudioEffectChorus::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectChorus::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

void AudioEffectChorus::_update_buffers() {

	float ring_buffer_max_size = AudioEffectChorus::MAX_DELAY_MS + AudioEffectChorus::MAX_DEPTH_MS + AudioEffectChorus::MAX_WIDTH_MS;

	ring_buffer_max_size *= 2; //just to avoid complications
	ring_buffer_max_size /= 1000.0; //convert to seconds
	ring_buffer_max_size *= sampling_rate;

	int ringbuff_size = ring_buffer_max_size;

	int bits = 0;

	while (ringbuff_size > 0) {
		bits++;
		ringbuff_size /= 2;
	}

	ringbuff_size = 1 << bits;
	buffer_mask = ringbuff_size - 1;
	buffer_pos = 0;
	audio_buffer.resize(ringbuff_size);
	for (int i = 0; i < ringbuff_size; i++) {
		audio_buffer[i] = AudioFrame(0, 0);
	}
}
AudioEffectChorus::AudioEffectChorus() {

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

	int stride = CONTROL_PORT_VOICE2_ENABLED - CONTROL_PORT_VOICE1_ENABLED;
	for (int i = 0; i < 4; i++) {

		String pp = "Voice " + String::num(i + 1) + " ";
		String ppid = "voice_" + String::num(i + 0) + "_";
		int ofs = i * stride;
		control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].name = pp + "Enabled";
		control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].identifier = ppid + "enabled";
		control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].hint = ControlPort::HINT_TOGGLE;
		control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].max = 1;
		control_ports[CONTROL_PORT_VOICE1_ENABLED + ofs].value = i < 2 ? 1 : 0;
		;

		control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].name = pp + "Delay (ms)";
		control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].identifier = ppid + "delay";
		control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].max = MAX_DELAY_MS;
		control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].value = 15 + i * 5;
		control_ports[CONTROL_PORT_VOICE1_DELAY + ofs].step = 0.1;

		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].name = pp + "Rate (hz)";
		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].identifier = ppid + "rate";
		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].min = 0.01;
		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].max = 20;
		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].value = 0.8 + i * 0.4;
		control_ports[CONTROL_PORT_VOICE1_HZ + ofs].step = 0.01;

		control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].name = pp + "Depth (ms)";
		control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].identifier = ppid + "depth";
		control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].max = MAX_DEPTH_MS;
		;
		control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].value = 2 + i;
		control_ports[CONTROL_PORT_VOICE1_DEPTH_MS + ofs].step = 0.1;

		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].name = pp + "Level (db)";
		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].identifier = ppid + "level";
		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].min = -60;
		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].max = 0;
		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].value = -2;
		control_ports[CONTROL_PORT_VOICE1_LEVEL_DB + ofs].step = 0.1;

		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].name = pp + "Cutoff (hz)";
		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].identifier = ppid + "cutoff";
		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].min = 1;
		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].max = MS_CUTOFF_MAX;
		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].value = 8000;
		control_ports[CONTROL_PORT_VOICE1_CUTOFF_HZ + ofs].step = 1;

		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].name = pp + "Pan";
		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].identifier = ppid + "pan";
		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].min = -1;
		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].max = 1;
		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].value = 0.5 * ((i & 1) ? -1.0 : 1.0);
		control_ports[CONTROL_PORT_VOICE1_PAN + ofs].step = 0.01;
	}

	block_size = 128;
	sampling_rate = 0;
	set_sampling_rate(44100);
	set_process_block_size(128);

	_update_buffers();
}
AudioEffectChorus::~AudioEffectChorus() {
}
