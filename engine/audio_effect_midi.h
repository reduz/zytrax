#ifndef AUDIO_EFFECT_MIDI_H
#define AUDIO_EFFECT_MIDI_H

#include "audio_effect.h"

//base for effects that use MIDI, provides parameters for common MIDI stuff
//and some helpers

class AudioEffectMIDI : public AudioEffect {
public:
	enum CustomMIDIPorts {
		CUSTOM_MIDI_PITCH_BEND,
		CUSTOM_MIDI_PITCH_BEND_UP,
		CUSTOM_MIDI_PITCH_BEND_DOWN,
		CUSTOM_MIDI_SMART_PORTAMENTO,
		CUSTOM_MIDI_PITCH_BEND_PORTAMENTO,
		CUSTOM_MIDI_AFTERTOUCH,
		CUSTOM_MIDI_MAX,
	};

	enum {
		CUSTOM_MIDI_MACRO_MAX = 100,
		TOTAL_INTERNAL_PORTS = MIDIEvent::CC_MAX + CUSTOM_MIDI_MAX
	};

	enum MonoMode {
		MONO_MODE_DEFAULT,
		MONO_MODE_ENABLE,
		MONO_MODE_DISABLE
	};

	struct NRPNInfo {
		int msb =0;
		int lsb = 0;
		int default_value = 0;
		String descriptor;
	};

private:
	enum {
		INTERNAL_MIDI_EVENT_BUFFER_SIZE = 16384,
		PITCH_BEND_BASE = 8192,
		PITCH_BEND_MAX = 8191,
		PITCH_BEND_MIN = -8192
	};

	ControlPortDefault cc_ports[MIDIEvent::CC_MAX];
	bool cc_use_default_values[MIDIEvent::CC_MAX] = {};
	int cc_default_values[MIDIEvent::CC_MAX] = {};
	ControlPortDefault custom_ports[CUSTOM_MIDI_MAX];

	Vector<NRPNInfo> nrpn;

	MIDIEventStamped process_events[INTERNAL_MIDI_EVENT_BUFFER_SIZE];

	int midi_channel;
	int pitch_bend_range; //in semitones
	MonoMode mono_mode;
	float curve_exponent = 1.8;

	bool smart_porta_in_use = false;
	bool bend_porta_in_use = false;
	bool reset_bend = false;

	bool reset_pending;
	float current_pitch_bend;
	int extra_pitch_bend;	
	int target_pitch_bend = 0;

	int last_note = -1;

	bool update_pitch_bend_range;


protected:
	void _reset_midi();
	virtual void _get_bank_and_patch(int &r_bank_lsb, int &r_bank_msb,int &r_patch);
	MIDIEventStamped *_process_midi_events(const Event *p_events, int p_event_count, float p_time, int &r_stamped_event_count);
	virtual int _get_internal_control_port_count() const = 0;
	virtual ControlPort *_get_internal_control_port(int p_index) = 0;

	virtual JSON::Node _internal_to_json() const = 0;
	virtual Error _internal_from_json(const JSON::Node &node) = 0;

public:
	void set_cc_visible(MIDIEvent::CC p_cc, bool p_visible);
	bool is_cc_visible(MIDIEvent::CC p_cc) const;
	void set_cc_use_default_value(MIDIEvent::CC p_cc, bool p_enable);
	bool get_cc_use_default_value(MIDIEvent::CC p_cc) const;
	void set_cc_default_value(MIDIEvent::CC p_cc, int p_value);
	int get_cc_default_value(MIDIEvent::CC p_cc) const;
	void set_midi_channel(int p_channel);
	int get_midi_channel() const;

	void set_curve_exponent(float p_exponent);
	float get_curve_exponent() const;

	void set_mono_mode(MonoMode p_mode);
	MonoMode get_mono_mode() const;

	void set_nrpn(int p_msb, int p_lsb, int p_default_value, const String& p_description);
	bool has_nrpn(int p_msb, int p_lsb) const;
	void clear_nrpn(int p_msb, int p_lsb);
	Vector<NRPNInfo> get_nrpns() const;


	virtual int get_control_port_count() const;
	virtual ControlPort *get_control_port(int p_port);

	virtual JSON::Node to_json() const;
	virtual Error from_json(const JSON::Node &node);

	void set_pitch_bend_range(int p_semitones);
	int get_pitch_bend_range() const;
	AudioEffectMIDI();
	~AudioEffectMIDI();
};

#endif // AUDIO_EFFECT_MIDI_H
