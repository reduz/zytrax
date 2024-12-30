#ifndef SYNTH_SINEWAVE_H
#define SYNTH_SINEWAVE_H

#include "effects/internal/synth_base.h"

class SynthSinewave : public SynthBase {

	enum ControlPorts {
		CONTROL_PORT_VOLUME_DB,
		CONTROL_PORT_RELEASE_TIME,
		CONTROL_PORT_MAX
	};

	ControlPortDefault control_ports[CONTROL_PORT_MAX];

	class SineVoice : public Voice {
		friend class SynthSinewave;
		SynthSinewave *sw = nullptr;
		Status status = STATUS_DISABLED;
		double time = 0;
		double off_time = 0;
		float note = 0;
		float velocity = 1.0;

		float volume = 1.0;
		float pan = 1.0;
		float pitch_offset = 0;
		float vol_l = 1.0,vol_r = 1.0;

	public:
		virtual void note_on(int8_t p_note, float p_velocity,int p_offset);
		virtual void note_off(float p_velocity,int p_offset);
		virtual void note_aftertouch(float p_velocity, int p_offset);

		virtual void set_volume(float p_volume);
		virtual void set_pan(float p_pan); //0 - 1
		virtual void set_pitch_offset(float p_notes); // multiplier

		virtual void kill();

		virtual Status get_status() const;
		virtual int8_t get_note() const;
		virtual void add_to_mix(AudioFrame *p_out);

		virtual ~SineVoice();
	};

	virtual Voice *create_voice();
public:

	virtual void reset();

	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	/*info */
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	SynthSinewave();
};

#endif // SYNTH_SINEWAVE_H
