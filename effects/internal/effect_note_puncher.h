#ifndef EFFECT_NOTE_PUNCHER_H
#define EFFECT_NOTE_PUNCHER_H

#include "engine/audio_effect.h"

class AudioEffectNotePuncher : public AudioEffect {

	enum {
		MAX_PUNCHES = 32
	};

	struct Punch {
		bool active;
		int time;
	};

	Punch punches[MAX_PUNCHES];

	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_PUNCH_DB,
		CONTROL_PORT_ATTACK_MS,
		CONTROL_PORT_DECAY_MS,
		CONTROL_PORT_MAX
	};

	Vector<float> envelope_block;

	ControlPortDefault control_ports[CONTROL_PORT_MAX];

	void _update_buffers();

public:
	//process
	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);

	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);
	//info
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	virtual int get_control_port_count() const;
	virtual ControlPort *get_control_port(int p_port);

	virtual void reset();

	/* Load/Save */

	virtual JSON::Node to_json() const;
	virtual Error from_json(const JSON::Node &node);

	AudioEffectNotePuncher();
	~AudioEffectNotePuncher();
};

#endif // EFFECT_NOTE_PUNCHER_H
