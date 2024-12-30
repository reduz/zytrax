#include "effect_distortion.h"
#include "dsp/db.h"
#include <math.h>

//process
bool AudioEffectDistortion::has_secondary_input() const {
	return false;
}


void AudioEffectDistortion::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	const float *src = (const float *)p_in;
	float *dst = (float *)p_out;

	float lpf_c = expf(-M_PI * 2.0 * control_ports[CONTROL_PORT_KEEP_HF].value / mix_rate);
	float lpf_ic = 1.0 - lpf_c;

	float drive_f = control_ports[CONTROL_PORT_DRIVE].value;
	float pregain_f = db2linear(control_ports[CONTROL_PORT_PRE_GAIN].value);
	float postgain_f = db2linear(control_ports[CONTROL_PORT_POST_GAIN].value);

	float atan_mult = powf(10, drive_f * drive_f * 3.0) - 1.0 + 0.001;
	float atan_div = 1.0 / (atanf(atan_mult) * (1.0 + drive_f * 8));

	float lofi_mult = powf(2.0, 2.0 + (1.0 - drive_f) * 14); //goes from 16 to 2 bits

	int mode = control_ports[CONTROL_PORT_MODE].value;

	for (int i = 0; i < block_size * 2; i++) {

		float out = undenormalize(src[i] * lpf_ic + lpf_c * h[i & 1]);
		h[i & 1] = out;
		float a = out;
		float ha = src[i] - out; //high freqs
		a *= pregain_f;

		switch (mode) {
			case MODE_CLIP: {
				float a_sign = a < 0 ? -1.0f : 1.0f;
				a = powf(abs(a), 1.0001 - drive_f) * a_sign;
				if (a > 1.0) {
					a = 1.0;
				} else if (a < (-1.0)) {
					a = -1.0;
				}

			} break;
			case MODE_ATAN: {
				a = atanf(a * atan_mult) * atan_div;

			} break;
			case MODE_LOFI: {
				a = floorf(a * lofi_mult + 0.5) / lofi_mult;

			} break;
			case MODE_OVERDRIVE: {
				const double x = a * 0.686306;
				const double z = 1 + exp(sqrt(fabs(x)) * -0.75);
				a = (expf(x) - expf(-x * z)) / (expf(x) + expf(-x));
			} break;
			case MODE_WAVESHAPE: {
				float x = a;
				float k = 2 * drive_f / (1.00001 - drive_f);

				a = (1.0 + k) * x / (1.0 + k * fabsf(x));

			} break;
		}

		dst[i] = a * postgain_f + ha;
	}
}

void AudioEffectDistortion::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}

void AudioEffectDistortion::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectDistortion::set_sampling_rate(int p_hz) {
	mix_rate = p_hz;
}
//info
String AudioEffectDistortion::get_name() const {
	return "Distortion";
}
String AudioEffectDistortion::get_unique_id() const {
	return "distortion";
}
String AudioEffectDistortion::get_provider_id() const {
	return "internal";
}

int AudioEffectDistortion::get_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *AudioEffectDistortion::get_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

void AudioEffectDistortion::reset() {
}

/* Load/Save */

JSON::Node AudioEffectDistortion::to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error AudioEffectDistortion::from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}

AudioEffectDistortion::AudioEffectDistortion() {

	control_ports[CONTROL_PORT_MODE].name = "Mode";
	control_ports[CONTROL_PORT_MODE].identifier = "mode";
	control_ports[CONTROL_PORT_MODE].hint = ControlPort::HINT_ENUM;
	control_ports[CONTROL_PORT_MODE].max = MODE_MAX -1;
	control_ports[CONTROL_PORT_MODE].step = 1;
	control_ports[CONTROL_PORT_MODE].value = 1;
	control_ports[CONTROL_PORT_MODE].enum_values.push_back("Clip");
	control_ports[CONTROL_PORT_MODE].enum_values.push_back("ATan");
	control_ports[CONTROL_PORT_MODE].enum_values.push_back("LoFi");
	control_ports[CONTROL_PORT_MODE].enum_values.push_back("Overdrive");
	control_ports[CONTROL_PORT_MODE].enum_values.push_back("WaveShape");

	control_ports[CONTROL_PORT_PRE_GAIN].name = "Pre Gain (db)";
	control_ports[CONTROL_PORT_PRE_GAIN].identifier = "pre_gain";
	control_ports[CONTROL_PORT_PRE_GAIN].min = -60;
	control_ports[CONTROL_PORT_PRE_GAIN].max = 60;
	control_ports[CONTROL_PORT_PRE_GAIN].step = 0.1;
	control_ports[CONTROL_PORT_PRE_GAIN].value = 0;

	control_ports[CONTROL_PORT_KEEP_HF].name = "Keep HF (hz)";
	control_ports[CONTROL_PORT_KEEP_HF].identifier = "keep_hf";
	control_ports[CONTROL_PORT_KEEP_HF].min = 1;
	control_ports[CONTROL_PORT_KEEP_HF].max = 20500;
	control_ports[CONTROL_PORT_KEEP_HF].step = 1;
	control_ports[CONTROL_PORT_KEEP_HF].value = 16000;

	control_ports[CONTROL_PORT_DRIVE].name = "Drive";
	control_ports[CONTROL_PORT_DRIVE].identifier = "drive";
	control_ports[CONTROL_PORT_DRIVE].min = 0;
	control_ports[CONTROL_PORT_DRIVE].max = 1;
	control_ports[CONTROL_PORT_DRIVE].step = 0.01;
	control_ports[CONTROL_PORT_DRIVE].value =0;

	control_ports[CONTROL_PORT_POST_GAIN].name = "Post Gain (db)";
	control_ports[CONTROL_PORT_POST_GAIN].identifier = "post_gain";
	control_ports[CONTROL_PORT_POST_GAIN].min = -60;
	control_ports[CONTROL_PORT_POST_GAIN].max = 24;
	control_ports[CONTROL_PORT_POST_GAIN].step = 0.1;
	control_ports[CONTROL_PORT_POST_GAIN].value = 0;

	block_size = 128;
}
AudioEffectDistortion::~AudioEffectDistortion() {
}
