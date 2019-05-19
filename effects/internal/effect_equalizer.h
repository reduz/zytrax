#ifndef EFFECT_EQUALIZER_H
#define EFFECT_EQUALIZER_H

#include "effects/internal/eq.h"
#include "engine/audio_effect.h"

class AudioEffectEqualizer : public AudioEffect {

	int block_size;
	int sampling_rate;

	EQ eq;
	Vector<EQ::BandProcess> bands[2];
	Vector<ControlPortDefault> control_ports;
	Vector<float> gains;

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

	AudioEffectEqualizer(EQ::Preset p_preset = EQ::PRESET_6_BANDS);
	~AudioEffectEqualizer();
};

#endif // EFFECT_EQUALIZER_H
