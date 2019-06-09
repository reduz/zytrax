#ifndef AUDIO_EFFECT_VST2_H
#define AUDIO_EFFECT_VST2_H

#include "engine/audio_effect_midi.h"
#include "globals/map.h"
#include "vestige.h"

#ifdef WINDOWS_ENABLED
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class AudioEffectVST2 : public AudioEffectMIDI {
public:
	typedef void (*ResizeCallback)(void *, int, int);

private:
	enum {
		MAX_INPUT_EVENTS = 8192
	};

	VstMidiEvent event_array[MAX_INPUT_EVENTS];
	unsigned char *event_pointer_data;
	VstEvents *event_pointers;

	Vector<float *> in_buffers;
	Vector<float *> out_buffers;

	bool has_side_input;
	String path;
	int vst_version;
	String unique_id;
	String name;
	String provider_id;
	AEffect *effect;
#ifdef WINDOWS_ENABLED
	HINSTANCE libhandle;
#else
	void *libhandle;
#endif
	static intptr_t VESTIGECALLBACK host(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt);

	ResizeCallback resize_callback;
	void *resize_userdata;

	struct ControlPortVST2 : public ControlPort {

		AEffect *effect;
		int index;
		String name;
		String identifier;
		String label;
		float value;
		bool visible;
		bool setting;
		bool editing;

		virtual Hint get_hint() const { return HINT_RANGE_NORMALIZED; }

		virtual String get_name() const { return name; }
		virtual String get_identifier() const { return identifier; }
		virtual float get_min() const { return 0; }
		virtual float get_max() const { return 1; }
		virtual float get_step() const { return 0.0001; }
		virtual bool is_visible() const { return visible; }

		virtual float get() const;
		virtual void set(float p_val);
		virtual String get_value_as_text() const;
	};

	Vector<ControlPortVST2> control_ports;
	int process_block_size;
	int sampling_rate;

	void _update_buffers();
	void _process(const Event *p_events, int p_event_count);

	bool stop_all_notes;
	uint8_t last_midi_channel;

	void _clear_buffers();

public:
	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);
	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);

	virtual void reset();

	bool has_user_interface() const;
	void get_user_interface_size(int &r_width, int &r_height);
	void resize_user_interface(int p_width, int p_height);
	void open_user_interface(void *p_window_ptr);
	void process_user_interface();
	void close_user_interface();

	/*info */
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	/* Load/Save */

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	void set_resize_callback(ResizeCallback p_callback, void *p_userdata);

	String get_path() const;
	Error open(const String &p_path, const String &p_unique_id, const String &p_name, const String &p_provider_id);
	AudioEffectVST2();
	~AudioEffectVST2();
};

#endif // AUDIO_EFFECT_VST2_H
