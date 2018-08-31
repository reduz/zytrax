#include "audio_effect.h"

void ControlPort::set_normalized(float p_val, bool p_make_initial) {

	p_val *= get_max() - get_min();
	p_val += get_min();
	set(p_val, p_make_initial);
}

float ControlPort::get_normalized() const {

	float v = get();
	v -= get_min();
	v /= get_max() - get_min();
	return v;
}

String ControlPort::convert_value_to_text(float p_value) const {

	return String::num(p_value);
}

String ControlPort::get_value_as_text() const {

	return convert_value_to_text(get());
}

ControlPort::Hint ControlPort::get_hint() const {

	return HINT_RANGE;
}

ControlPort::ControlPort() {
}
ControlPort::~ControlPort() {
}

AudioEffect::AudioEffect() {
}

AudioEffect::~AudioEffect() {
}

const ControlPort *AudioEffect::get_control_port(int p_idx) const {

	return const_cast<AudioEffect *>(this)->get_control_port(p_idx);
}

void AudioEffectFactory::add_audio_effect(AudioEffectInfo p_info) {
	audio_effects.push_back(p_info);
}

int AudioEffectFactory::get_audio_effect_count() {

	return audio_effects.size();
}

const AudioEffectInfo *AudioEffectFactory::get_audio_effect(int p_idx) {

	ERR_FAIL_INDEX_V(p_idx, audio_effects.size(), NULL);
	return &audio_effects[p_idx];
}

AudioEffect *AudioEffectFactory::instance_effect(int p_idx) {

	ERR_FAIL_INDEX_V(p_idx, audio_effects.size(), NULL);

	return audio_effects[p_idx].creation_func(&audio_effects[p_idx]);
}

void AudioEffectFactory::add_provider(AudioEffectProvider *p_provider) {
	providers.push_back(p_provider);
}

void AudioEffectFactory::rescan_effects() {
	for (int i = 0; i < providers.size(); i++) {
		providers[i]->scan_effects(this);
	}
}
