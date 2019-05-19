#ifndef EFFECT_REVERB_H
#define EFFECT_REVERB_H

#include "effects/internal/reverb.h"
#include "engine/audio_effect.h"

class AudioEffectReverb : public AudioEffect {

	Reverb reverb[2];
	Vector<float> tmp_frames_src;
	Vector<float> tmp_frames_dst;

	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_PREDELAY_MSEC,
		CONTROL_PORT_PREDELAY_FEEDBACK,
		CONTROL_PORT_ROOM_SIZE,
		CONTROL_PORT_DAMPING,
		CONTROL_PORT_SPREAD,
		CONTROL_PORT_DRY,
		CONTROL_PORT_WET,
		CONTROL_PORT_HPF,
		CONTROL_PORT_MAX
	};

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

	AudioEffectReverb();
	~AudioEffectReverb();
};

#endif // EFFECT_REVERB_H
