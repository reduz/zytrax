#include "audio_effect.h"




void ControlPort::set_normalized(float p_val,bool p_make_initial) {

	p_val*=get_max()-get_min();
	p_val+=get_min();
	set(p_val,p_make_initial);

}

float ControlPort::get_normalized() const {

	float v=get();
	v-=get_min();
	v/=get_max()-get_min();
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

const ControlPort* AudioEffect::get_control_port(int p_idx) const {

	return const_cast<AudioEffect*>(this)->get_control_port(p_idx);
}
AudioEffect::AudioEffect()
{
}

AudioEffect::~AudioEffect()
{
}
