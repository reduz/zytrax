#ifndef SYNTH_BASE_H
#define SYNTH_BASE_H

#include "engine/audio_effect_midi.h"
#include <math.h>

class SynthBase : public AudioEffectMIDI {
protected:

	class Voice {
	public:
		virtual void note_on(int8_t p_note, float p_velocity,int p_offset) = 0;
		virtual void note_off(float p_velocity,int p_offset) = 0;
		virtual void note_aftertouch(float p_velocity,int p_offset) = 0;

		virtual void set_volume(float p_volume) = 0;
		virtual void set_pan(float p_pan) = 0; //0 - 1
		virtual void set_pitch_offset(float p_notes) = 0; // multiplier

		virtual void kill() = 0;

		enum Status {
			STATUS_DISABLED,
			STATUS_ON,
			STATUS_OFF,
		};

		virtual Status get_status() const = 0;
		virtual int8_t get_note() const = 0;
		virtual void add_to_mix(AudioFrame *p_out) = 0;
		virtual ~Voice() {}
	};

	virtual void _reset_sound() {}
	virtual void _tempo_changed(int p_tempo,int p_offset) {}
	virtual void _bar(int p_bar,int p_offset) {}
	virtual void _beat(int p_bar,int p_offset) {}
	virtual Voice *create_voice() = 0;
	virtual void _post_process(AudioFrame *p_out) {} // may not want to.

	void _setup(); // call on constructor
private:

	enum {
		MAX_VOICES = 256
	};

	uint32_t pitch_wheel = 8192;
	uint32_t midi_pan = 8192;
	uint32_t midi_volume = 16383;
	uint32_t midi_expression = 16383;
	uint32_t midi_rpn = 0xFFFF;
	uint32_t midi_data = 0xFFFF;
	float pitch_range = 2.0;
	float tuning = 0.0;

	float pitch_bend_final = 0;
	float pan_final = 0.5;
	float volume_final = 1.0;

	int active_voices = 0;
	Voice * voices[MAX_VOICES] {};

	int process_block_size = 128;
	int mix_rate = 44100;

	_FORCE_INLINE_ float _velocity_to_volume(int8_t p_velocity) const {
		return powf(float(p_velocity) / 127.0, get_curve_exponent());
	}

public:

	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);
	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);
	_FORCE_INLINE_ int get_sampling_rate() const { return mix_rate; }
	_FORCE_INLINE_ int get_process_block_size() const { return process_block_size; }

	SynthBase();
};

#endif // SYNTH_BASE_H
