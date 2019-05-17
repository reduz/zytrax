#include "audio_effect.h"

String AudioEffectProvider::scan_paths[AudioEffectProvider::MAX_SCAN_PATHS];

void AudioEffectProvider::set_scan_path(int p_index, const String &p_scan) {
	ERR_FAIL_INDEX(p_index, MAX_SCAN_PATHS);
	scan_paths[p_index] = p_scan;
}
String AudioEffectProvider::get_scan_path(int p_index) {
	ERR_FAIL_INDEX_V(p_index, MAX_SCAN_PATHS, String());
	return scan_paths[p_index];
}

void ControlPort::set_normalized(float p_val) {

	p_val *= get_max() - get_min();
	p_val += get_min();
	set(p_val);
}

float ControlPort::get_normalized() const {

	float v = get();
	v -= get_min();
	v /= get_max() - get_min();
	return v;
}

String ControlPort::get_value_as_text() const {

	return String::num(get());
}

ControlPort::Hint ControlPort::get_hint() const {

	return HINT_RANGE;
}

void ControlPort::ui_changed_notify() {
	if (changed_callback) {
		changed_callback(changed_userdata);
	}
}
void ControlPort::set_ui_changed_callback(UIChangedCallback p_callback, void *p_userdata) {
	changed_callback = p_callback;
	changed_userdata = p_userdata;
}
ControlPort::ControlPort() {
	changed_callback = NULL;
	changed_userdata = NULL;
	command = 0;
}
ControlPort::~ControlPort() {
}

void ControlPort::set_command(char p_command) {
	command = p_command;
}
char ControlPort::get_command() const {
	return command;
}

void AudioEffect::set_skip(bool p_skip) {
	skip = p_skip;
}
bool AudioEffect::is_skipped() const {
	return skip;
}

AudioEffect::AudioEffect() {
	skip = false;
}

AudioEffect::~AudioEffect() {
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

AudioEffect *AudioEffectFactory::instantiate_effect(int p_idx) {

	ERR_FAIL_INDEX_V(p_idx, audio_effects.size(), NULL);

	for (int i = 0; i < providers.size(); i++) {
		if (providers[i]->get_id() == audio_effects[p_idx].provider_id) {
			return providers[i]->instantiate_effect(&audio_effects[p_idx]);
		}
	}

	return NULL;
}

void AudioEffectFactory::add_provider(AudioEffectProvider *p_provider) {
	providers.push_back(p_provider);
}

void AudioEffectFactory::rescan_effects(AudioEffectProvider::ScanCallback p_callback, void *p_userdata) {

	audio_effects.clear();
	for (int i = 0; i < providers.size(); i++) {
		providers[i]->scan_effects(this, p_callback, p_userdata);
	}
}
