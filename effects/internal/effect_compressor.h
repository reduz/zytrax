#ifndef EFFECT_COMPRESSOR_H
#define EFFECT_COMPRESSOR_H

#include "engine/audio_effect.h"

class AudioEffectCompressor : public AudioEffect {

	float rundb, averatio, runratio, runmax, maxover, gr_meter;
	int current_channel;

	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_PRE_GAIN_DB,
		CONTROL_PORT_THRESHOLD_DB,
		CONTROL_PORT_RATIO,
		CONTROL_PORT_ATTACK_MS,
		CONTROL_PORT_RELEASE_MS,
		CONTROL_PORT_POST_GAIN_DB,
		CONTROL_PORT_MIX,
		CONTROL_PORT_MAX
	};

	bool sidechain;

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

	AudioEffectCompressor(bool p_sidechain);
	~AudioEffectCompressor();
};

#endif // EFFECT_COMPRESSOR_H
