#ifndef AUDIO_EFFECT_AU_H
#define AUDIO_EFFECT_AU_H

#ifdef OSX_ENABLED

#include "engine/audio_effect_midi.h"
#include "globals/vector.h"

class AudioEffectAU : public AudioEffectMIDI {
public:
	typedef void (*ResizeCallback)(void *, int, int);

private:
	void *component; // AudioComponent
	void *unit;      // AudioUnit

	String name;
	String unique_id;
	String provider_id;

	int process_block_size;
	int sampling_rate;
	uint64_t sample_time;

	float *in_buf_l;
	float *in_buf_r;
	float *out_buf_l;
	float *out_buf_r;
	float *interleaved_buf; // used when AU outputs interleaved audio

	bool is_synth;
	bool has_input_bus;
	bool is_interleaved_out;
	bool accepts_midi;
	bool stop_all_notes;
	uint8_t last_midi_channel;

	void *event_listener; // AUEventListener — fires when the plugin UI changes a param

	ResizeCallback resize_callback;
	void *resize_userdata;

	struct ControlPortAU : public ControlPort {
		void *unit; // AudioUnit (void* to keep header free of CoreAudio)
		unsigned int param_id; // AudioUnitParameterID
		String name;
		String identifier;
		float min_val;
		float max_val;
		bool setting;
		bool is_bool;     // kAudioUnitParameterUnit_Boolean
		bool is_indexed;  // kAudioUnitParameterUnit_Indexed

		virtual Hint get_hint() const {
			if (is_bool) return HINT_TOGGLE;
			if (is_indexed) return HINT_ENUM;
			return HINT_RANGE;
		}
		virtual String get_name() const { return name; }
		virtual String get_identifier() const { return identifier; }
		virtual float get_min() const { return min_val; }
		virtual float get_max() const { return max_val; }
		virtual float get_step() const {
			if (is_bool || is_indexed) return 1.0f;
			return (max_val - min_val) * 0.0001f;
		}
		virtual bool is_visible() const { return true; }
		virtual float get() const;
		virtual void set(float p_val);
		virtual String get_value_as_text() const;
	};

	Vector<ControlPortAU> control_ports;

	void _alloc_buffers();
	void _free_buffers();
	void _create_event_listener();
	void _destroy_event_listener();
	void _send_midi_events(const Event *p_events, int p_event_count, float p_time);

public:
	// Called by the AU parameter-change listener to notify the host of a UI edit.
	void _notify_param_changed(unsigned int p_param_id);

private:

public:
	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual bool has_secondary_input() const { return false; }
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);
	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);
	virtual void reset();

	virtual String get_name() const { return name; }
	virtual String get_unique_id() const { return unique_id; }
	virtual String get_provider_id() const { return provider_id; }

	bool has_user_interface() const;
	void *get_user_interface_view(); // Returns NSView*
	bool get_user_interface_size(int &r_width, int &r_height);

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	void set_resize_callback(ResizeCallback p_callback, void *p_userdata);

	void _get_input_buffers(float *&l, float *&r); // used by render callback

	Error open(void *p_component, const String &p_unique_id, const String &p_name, const String &p_provider_id, bool p_is_synth);

	AudioEffectAU();
	~AudioEffectAU();
};

#endif // OSX_ENABLED
#endif // AUDIO_EFFECT_AU_H
