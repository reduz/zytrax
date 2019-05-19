#ifndef EFFECT_PHASER_H
#define EFFECT_PHASER_H

#include "engine/audio_effect.h"

class AudioEffectPhaser : public AudioEffect {

	class AllpassDelay {
		float a, h;

	public:
		_FORCE_INLINE_ void delay(float d) {
			a = (1.f - d) / (1.f + d);
		}

		_FORCE_INLINE_ float update(float s) {
			float y = s * -a + h;
			h = y * a + s;
			return y;
		}

		AllpassDelay() {
			a = 0;
			h = 0;
		}
	};

	AllpassDelay allpass[2][6];

	float phase;
	AudioFrame h;

	int block_size;
	int sampling_rate;

	enum ControlPorts {
		CONTROL_PORT_RANGE_MIN_HZ,
		CONTROL_PORT_RANGE_MAX_HZ,
		CONTROL_PORT_RATE_HZ,
		CONTROL_PORT_FEEDBACK,
		CONTROL_PORT_DEPTH,
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

	AudioEffectPhaser();
	~AudioEffectPhaser();
};

#endif // EFFECT_PHASER_H
