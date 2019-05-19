#ifndef EFFECT_DELAY_H
#define EFFECT_DELAY_H

#include "engine/audio_effect.h"

class AudioEffectDelay : public AudioEffect {

	int block_size;
	int sampling_rate;
	bool bpm_sync;
	float bpm;

	enum {
		MAX_DELAY_UNITS = 48,
		MAX_DELAY_MS = 4000,
		MS_CUTOFF_MAX = 16000,
		MAX_TAPS = 4
	};

	Vector<AudioFrame> ring_buffer;

	unsigned int ring_buffer_pos;
	unsigned int ring_buffer_mask;

	struct Tap {
		AudioFrame h;
		Vector<AudioFrame> feedback_buffer;
		unsigned int feedback_buffer_pos;
	};

	void _process_chunk(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count);

	Tap taps[MAX_TAPS];

	enum BeatSubdiv {
		BEAT_SUBDIV_1,
		BEAT_SUBDIV_2,
		BEAT_SUBDIV_3,
		BEAT_SUBDIV_4,
		BEAT_SUBDIV_6,
		BEAT_SUBDIV_8,
		BEAT_SUBDIV_12,
		BEAT_SUBDIV_16,
		BEAT_SUBDIV_MAX
	};

	enum ControlPorts {
		CONTROL_PORT_TAP1_ENABLED,
		CONTROL_PORT_TAP1_BEAT_SUBDIV,
		CONTROL_PORT_TAP1_DELAY_UNITS,
		CONTROL_PORT_TAP1_VOLUME,
		CONTROL_PORT_TAP1_FEEDBACK,
		CONTROL_PORT_TAP1_FEEDBACK_STEREO_SWAP,
		CONTROL_PORT_TAP1_LPF,
		CONTROL_PORT_TAP1_PAN,

		CONTROL_PORT_TAP2_ENABLED,
		CONTROL_PORT_TAP2_BEAT_SUBDIV,
		CONTROL_PORT_TAP2_DELAY_UNITS,
		CONTROL_PORT_TAP2_VOLUME,
		CONTROL_PORT_TAP2_FEEDBACK,
		CONTROL_PORT_TAP2_FEEDBACK_STEREO_SWAP,
		CONTROL_PORT_TAP2_LPF,
		CONTROL_PORT_TAP2_PAN,

		CONTROL_PORT_TAP3_ENABLED,
		CONTROL_PORT_TAP3_BEAT_SUBDIV,
		CONTROL_PORT_TAP3_DELAY_UNITS,
		CONTROL_PORT_TAP3_VOLUME,
		CONTROL_PORT_TAP3_FEEDBACK,
		CONTROL_PORT_TAP3_FEEDBACK_STEREO_SWAP,
		CONTROL_PORT_TAP3_LPF,
		CONTROL_PORT_TAP3_PAN,

		CONTROL_PORT_TAP4_ENABLED,
		CONTROL_PORT_TAP4_BEAT_SUBDIV,
		CONTROL_PORT_TAP4_DELAY_UNITS,
		CONTROL_PORT_TAP4_VOLUME,
		CONTROL_PORT_TAP4_FEEDBACK,
		CONTROL_PORT_TAP4_FEEDBACK_STEREO_SWAP,
		CONTROL_PORT_TAP4_LPF,
		CONTROL_PORT_TAP4_PAN,

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

	AudioEffectDelay(bool p_bpm_sync);
	~AudioEffectDelay();
};

#endif // EFFECT_DELAY_H
