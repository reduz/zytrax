#ifndef EFFECT_FILTER_H
#define EFFECT_FILTER_H

#include "dsp/filter.h"
#include "engine/audio_effect.h"
class AudioEffectFilter : public AudioEffect {

	Filter::Mode filter_mode;
	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_CUTOFF_HZ,
		CONTROL_PORT_RESONANCE,
		CONTROL_PORT_GAIN,
		CONTROL_PORT_DB,
		CONTROL_PORT_MAX
	};

	ControlPortDefault control_ports[CONTROL_PORT_MAX];

	void _update_buffers();

	Filter filter;
	Filter::Processor filter_process[2][4];

	template <int S>
	void _process_filter(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count);

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

	AudioEffectFilter(Filter::Mode p_filter_mode);
	~AudioEffectFilter();
};

#endif // EFFECT_FILTER_H
