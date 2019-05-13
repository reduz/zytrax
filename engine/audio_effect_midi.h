#ifndef AUDIO_EFFECT_MIDI_H
#define AUDIO_EFFECT_MIDI_H

#include "audio_effect.h"

//base for effects that use MIDI, provides parameters for common MIDI stuff
//and some helpers

class AudioEffectMIDI : public AudioEffect {
public:
	enum CustomMIDIPorts {
		CUSTOM_MIDI_SMART_PORTAMENTO,
		CUSTOM_MIDI_SMART_VIBRATO,
		CUSTOM_MIDI_MACRO,
		CUSTOM_MIDI_MAX,
	};
	enum {
		CUSTOM_MIDI_MACRO_MAX = 100,
		TOTAL_INTERNAL_PORTS = MIDIEvent::CC_MAX + CUSTOM_MIDI_MAX
	};

private:
	enum {
		INTERNAL_MIDI_EVENT_BUFFER_SIZE = 4096
	};

	ControlPortDefault cc_ports[MIDIEvent::CC_MAX];
	ControlPortDefault custom_ports[CUSTOM_MIDI_MAX];
	Vector<uint8_t> midi_macro[CUSTOM_MIDI_MACRO_MAX];

	struct MIDIEventStamped {
		uint32_t frame;
		MIDIEvent event;
	};

	MIDIEventStamped process_events[INTERNAL_MIDI_EVENT_BUFFER_SIZE];

	int midi_channel;

protected:
	MIDIEventStamped *_process_midi_events(const Event *p_events, int p_event_count, int &r_stamped_event_count);
	virtual int _get_internal_control_port_count() const = 0;
	virtual ControlPort *_get_internal_control_port(int p_index) = 0;

	virtual JSON::Node _internal_to_json() const = 0;
	virtual Error _internal_from_json(const JSON::Node &node) = 0;

public:
	void set_cc_visible(MIDIEvent::CC p_cc, bool p_visible);
	bool is_cc_visible(MIDIEvent::CC p_cc) const;
	void set_midi_channel(int p_channel);
	int get_midi_channel() const;

	void set_midi_macro(int p_macro, const Vector<uint8_t> &p_bytes);
	Vector<uint8_t> get_midi_macro(int p_macro) const;

	virtual int get_control_port_count() const;
	virtual ControlPort *get_control_port(int p_port);

	virtual JSON::Node to_json() const;
	virtual Error from_json(const JSON::Node &node);

	AudioEffectMIDI();
};

#endif // AUDIO_EFFECT_MIDI_H
