#include "audio_effect_midi.h"

AudioEffectMIDI::MIDIEventStamped *AudioEffectMIDI::_process_midi_events(const Event *p_events, int p_event_count, int &r_stamped_event_count) {

	return NULL;
}

int AudioEffectMIDI::get_control_port_count() const {

	return TOTAL_INTERNAL_PORTS + _get_internal_control_port_count();
}
ControlPort *AudioEffectMIDI::get_control_port(int p_port) {
	if (p_port < CUSTOM_MIDI_MAX) {
		return &custom_ports[p_port];
	}
	p_port -= CUSTOM_MIDI_MAX;
	if (p_port < MIDIEvent::CC_MAX) {
		return &cc_ports[p_port];
	}
	p_port -= MIDIEvent::CC_MAX;
	return _get_internal_control_port(p_port);
}

void AudioEffectMIDI::set_cc_visible(MIDIEvent::CC p_cc, bool p_visible) {
	cc_ports[p_cc].visible = p_visible;
}
bool AudioEffectMIDI::is_cc_visible(MIDIEvent::CC p_cc) const {
	return cc_ports[p_cc].visible;
}

void AudioEffectMIDI::set_midi_channel(int p_channel) {
	ERR_FAIL_INDEX(p_channel, 16);
	midi_channel = p_channel;
}
int AudioEffectMIDI::get_midi_channel() const {
	return midi_channel;
}

void AudioEffectMIDI::set_midi_macro(int p_macro, const Vector<uint8_t> &p_bytes) {
	ERR_FAIL_INDEX(p_macro, CUSTOM_MIDI_MACRO_MAX);
	midi_macro[p_macro] = p_bytes;
}
Vector<uint8_t> AudioEffectMIDI::get_midi_macro(int p_macro) const {
	ERR_FAIL_INDEX_V(p_macro, CUSTOM_MIDI_MACRO_MAX, Vector<uint8_t>());
	return midi_macro[p_macro];
}

AudioEffectMIDI::AudioEffectMIDI() {

	midi_channel = 0;
	//set built in control ports
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].name = "Smart Portamento";
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].identifier = "smart_portamento";
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].max = 1.0;
	custom_ports[CUSTOM_MIDI_SMART_PORTAMENTO].step = 0.001;

	custom_ports[CUSTOM_MIDI_SMART_VIBRATO].name = "Smart Vibrato";
	custom_ports[CUSTOM_MIDI_SMART_VIBRATO].identifier = "smart_vibrato";
	custom_ports[CUSTOM_MIDI_SMART_VIBRATO].max = 99;
	custom_ports[CUSTOM_MIDI_SMART_VIBRATO].step = 1;

	custom_ports[CUSTOM_MIDI_MACRO].name = "MIDI Macros";
	custom_ports[CUSTOM_MIDI_MACRO].identifier = "midi_macro";
	custom_ports[CUSTOM_MIDI_MACRO].max = 99;
	custom_ports[CUSTOM_MIDI_MACRO].step = 1;

	//set ports for CCs
	for (int i = 0; i < MIDIEvent::CC_MAX; i++) {
		cc_ports[i].name = String("CC: ") + MIDIEvent::cc_names[i];
		cc_ports[i].identifier = String("cc_") + MIDIEvent::cc_names[i];
		cc_ports[i].visible = false;
		cc_ports[i].max = 127;
		cc_ports[i].step = 1;
		cc_ports[i].hint = ControlPort::HINT_RANGE;
	}

	cc_ports[MIDIEvent::CC_MODULATION].visible = true;
	cc_ports[MIDIEvent::CC_BREATH].visible = true;
	cc_ports[MIDIEvent::CC_PAN].visible = true;
	cc_ports[MIDIEvent::CC_PAN].initial = 64;
	cc_ports[MIDIEvent::CC_PAN].value = 64;
	cc_ports[MIDIEvent::CC_EXPRESSION].visible = true;
	cc_ports[MIDIEvent::CC_EXPRESSION].initial = 127;
	cc_ports[MIDIEvent::CC_EXPRESSION].value = 127;
}
