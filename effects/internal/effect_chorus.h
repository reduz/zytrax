#ifndef EFFECT_CHORUS_H
#define EFFECT_CHORUS_H

#include "engine/audio_effect.h"

class AudioEffectChorus : public AudioEffect {

	enum {

		MAX_DELAY_MS = 50,
		MAX_DEPTH_MS = 20,
		MAX_WIDTH_MS = 50,
		MAX_VOICES = 4,
		CYCLES_FRAC = 16,
		CYCLES_MASK = (1 << CYCLES_FRAC) - 1,
		MAX_CHANNELS = 4,
		MS_CUTOFF_MAX = 16000
	};

	Vector<AudioFrame> audio_buffer;
	unsigned int buffer_pos;
	unsigned int buffer_mask;

	AudioFrame filter_h[4];
	uint64_t cycles[4];

	void _process_chunk(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count);

	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_WET,
		CONTROL_PORT_DRY,
		CONTROL_PORT_VOICE1_ENABLED,
		CONTROL_PORT_VOICE1_DELAY,
		CONTROL_PORT_VOICE1_HZ,
		CONTROL_PORT_VOICE1_DEPTH_MS,
		CONTROL_PORT_VOICE1_LEVEL_DB,
		CONTROL_PORT_VOICE1_CUTOFF_HZ,
		CONTROL_PORT_VOICE1_PAN,
		CONTROL_PORT_VOICE2_ENABLED,
		CONTROL_PORT_VOICE2_DELAY,
		CONTROL_PORT_VOICE2_HZ,
		CONTROL_PORT_VOICE2_DEPTH_MS,
		CONTROL_PORT_VOICE2_LEVEL_DB,
		CONTROL_PORT_VOICE2_CUTOFF_HZ,
		CONTROL_PORT_VOICE2_PAN,
		CONTROL_PORT_VOICE3_ENABLED,
		CONTROL_PORT_VOICE3_DELAY,
		CONTROL_PORT_VOICE3_HZ,
		CONTROL_PORT_VOICE3_DEPTH_MS,
		CONTROL_PORT_VOICE3_LEVEL_DB,
		CONTROL_PORT_VOICE3_CUTOFF_HZ,
		CONTROL_PORT_VOICE3_PAN,
		CONTROL_PORT_VOICE4_ENABLED,
		CONTROL_PORT_VOICE4_DELAY,
		CONTROL_PORT_VOICE4_HZ,
		CONTROL_PORT_VOICE4_DEPTH_MS,
		CONTROL_PORT_VOICE4_LEVEL_DB,
		CONTROL_PORT_VOICE4_CUTOFF_HZ,
		CONTROL_PORT_VOICE4_PAN,
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

	AudioEffectChorus();
	~AudioEffectChorus();
};

#endif // EFFECT_CHORUS_H
