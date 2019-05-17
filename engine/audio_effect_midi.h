#ifndef AUDIO_EFFECT_MIDI_H
#define AUDIO_EFFECT_MIDI_H

#include "audio_effect.h"

//base for effects that use MIDI, provides parameters for common MIDI stuff
//and some helpers

class AudioEffectMIDI : public AudioEffect {
public:
	enum CustomMIDIPorts {
		CUSTOM_MIDI_BEND_PORTAMENTO,
		CUSTOM_MIDI_BEND_VIBRATO,
		CUSTOM_MIDI_BEND_SLIDE_UP,
		CUSTOM_MIDI_BEND_SLIDE_DOWN,
		CUSTOM_MIDI_PITCH_BEND,
		CUSTOM_MIDI_PITCH_BEND_UP,
		CUSTOM_MIDI_PITCH_BEND_DOWN,
		CUSTOM_MIDI_SMART_PORTAMENTO,
		CUSTOM_MIDI_AFTERTOUCH,
		CUSTOM_MIDI_CHANGE_PROGRAM,
		CUSTOM_MIDI_MACRO,
		CUSTOM_MIDI_MAX,
	};
	enum {
		CUSTOM_MIDI_MACRO_MAX = 100,
		TOTAL_INTERNAL_PORTS = MIDIEvent::CC_MAX + CUSTOM_MIDI_MAX
	};

	struct MIDIEventStamped {
		uint32_t frame;
		MIDIEvent event;
	};

private:
	enum {
		INTERNAL_MIDI_EVENT_BUFFER_SIZE = 4096,
		PITCH_BEND_MAX = 8192,
		PITCH_BEND_MIN = -8192
	};

	ControlPortDefault cc_ports[MIDIEvent::CC_MAX];
	ControlPortDefault custom_ports[CUSTOM_MIDI_MAX];
	Vector<uint8_t> midi_macro[CUSTOM_MIDI_MACRO_MAX];

	MIDIEventStamped process_events[INTERNAL_MIDI_EVENT_BUFFER_SIZE];

	int midi_channel;
	int pitch_bend_range; //in semitones

	bool reset_pending;
	int current_pitch_bend;
	int extra_pitch_bend;

	/* bend vibrato stuff */
	int prev_bend_vibrato;
	float bend_vibrato_time;
	/* bend portamento stuff */
	bool prev_bend_portamento;
	bool prev_bend_slide;
	int bend_portamento_last_note;
	int bend_portamento_target_note;
	int bp_remap_note_off_from;
	int bp_remap_note_off_to;

	bool update_pitch_bend_range;

protected:
	void _reset_midi();
	MIDIEventStamped *_process_midi_events(const Event *p_events, int p_event_count, float p_time, int &r_stamped_event_count);
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

	void set_pitch_bend_range(int p_semitones);
	int get_pitch_bend_range() const;
	AudioEffectMIDI();
};

#endif // AUDIO_EFFECT_MIDI_H
