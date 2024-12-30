#include "synth_sinewave.h"

#include "dsp/db.h"

void SynthSinewave::SineVoice::note_on(int8_t p_note, float p_velocity,int p_offset) {
	note = p_note;
	velocity = p_velocity;
	time = 0;
	off_time = 0;
	status = STATUS_ON;
	vol_l = 0;
	vol_r = 0;
}

void SynthSinewave::SineVoice::note_off(float p_velocity,int p_offset){
	status = STATUS_OFF;
}

void SynthSinewave::SineVoice::note_aftertouch(float p_velocity,int p_offset){
	velocity = p_velocity;
}

void SynthSinewave::SineVoice::set_volume(float p_volume){
	volume = p_volume;
}

void SynthSinewave::SineVoice::set_pan(float p_pan){
	pan = p_pan;
}

void SynthSinewave::SineVoice::set_pitch_offset(float p_notes) {
	pitch_offset = p_notes;
}

void SynthSinewave::SineVoice::kill() {
	status = STATUS_DISABLED;
}

SynthSinewave::SineVoice::Status SynthSinewave::SineVoice::get_status() const {
	return status;
}

int8_t SynthSinewave::SineVoice::get_note() const {
	return int8_t(note);
}

void SynthSinewave::SineVoice::add_to_mix(AudioFrame *p_out) {

	int block_size = sw->get_process_block_size();
	float sampling_rate = float(sw->get_sampling_rate());

	double freq = 440.0 * pow(2.0, (note + pitch_offset - 69.0) / 12.0);

	double increment = freq / sampling_rate;


	float final_volume = velocity * volume * db2linear(sw->control_ports[CONTROL_PORT_VOLUME_DB].value);

	if (status == STATUS_OFF) {
		double decay = sw->control_ports[CONTROL_PORT_RELEASE_TIME].value;
		double env_volume = 1.0 - off_time / decay;
		if (env_volume < 0) {
			env_volume = 0;
			status = STATUS_DISABLED;
		}

		final_volume *= env_volume;
		off_time += float(block_size) / sampling_rate;
	}

	float new_vol_l = final_volume * (1.0 - pan);
	float new_vol_r = final_volume * pan;

	float vol_adv_l = (new_vol_l - vol_l) / float(block_size);
	float vol_adv_r = (new_vol_r - vol_r) / float(block_size);

	for(int i=0;i<block_size;i++) {
		float s = sinf( time * M_PI * 2.0 );
		p_out[i].l += vol_l * s;
		p_out[i].r += vol_r * s;

		time += increment;
		vol_l += vol_adv_l;
		vol_r += vol_adv_r;
	}

}

SynthBase::Voice *SynthSinewave::create_voice() {
	SineVoice *v = new SineVoice;
	v->sw = this;
	return v;
}


int SynthSinewave::_get_internal_control_port_count() const {
	return CONTROL_PORT_MAX;
}
ControlPort *SynthSinewave::_get_internal_control_port(int p_port) {
	ERR_FAIL_INDEX_V(p_port, CONTROL_PORT_MAX, NULL);
	return &control_ports[p_port];
}

SynthSinewave::SineVoice::~SineVoice(){

}

String SynthSinewave::get_name() const {
	return "Sinewave";
}

String SynthSinewave::get_unique_id() const{
	return "synth_sinewave";
}

String SynthSinewave::get_provider_id() const{
	return "internal";
}


void SynthSinewave::reset() {
	_reset_midi();
}

JSON::Node SynthSinewave::_internal_to_json() const {
	JSON::Node node = JSON::object();

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		node.add(control_ports[i].identifier.utf8().get_data(), control_ports[i].value);
	}
	return node;
}
Error SynthSinewave::_internal_from_json(const JSON::Node &node) {

	for (int i = 0; i < CONTROL_PORT_MAX; i++) {
		std::string key = control_ports[i].identifier.utf8().get_data();
		if (node.has(key)) {
			control_ports[i].value = node.get(key).toFloat();
		}
	}
	return OK;
}
SynthSinewave::SynthSinewave() {
	control_ports[CONTROL_PORT_VOLUME_DB].name = "Volume (db)";
	control_ports[CONTROL_PORT_VOLUME_DB].identifier = "amplify";
	control_ports[CONTROL_PORT_VOLUME_DB].min = -60;
	control_ports[CONTROL_PORT_VOLUME_DB].max = 24;
	control_ports[CONTROL_PORT_VOLUME_DB].step = 0.1;
	control_ports[CONTROL_PORT_VOLUME_DB].value = -12;

	control_ports[CONTROL_PORT_RELEASE_TIME].name = "Release (time)";
	control_ports[CONTROL_PORT_RELEASE_TIME].identifier = "release";
	control_ports[CONTROL_PORT_RELEASE_TIME].min = 0;
	control_ports[CONTROL_PORT_RELEASE_TIME].max = 10;
	control_ports[CONTROL_PORT_RELEASE_TIME].step = 0.1;
	control_ports[CONTROL_PORT_RELEASE_TIME].value = 1;
	_setup();
}
