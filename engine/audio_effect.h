#ifndef AUDIO_EFFECT_H
#define AUDIO_EFFECT_H

#include "dsp/frame.h"
#include "dsp/midi_event.h"
#include "globals/json.h"
#include "globals/rstring.h"
#include "globals/vector.h"
#include <memory>

class AudioEffect;
class AudioEffectFactory;
struct AudioEffectInfo;

class AudioEffectProvider {
public:
	enum {
		MAX_SCAN_PATHS = 256
	};

private:
	static String scan_paths[MAX_SCAN_PATHS];

public:
	typedef void (*ScanCallback)(const String &, void *);

	static void set_scan_path(int p_index, const String &p_path);
	static String get_scan_path(int p_index);

	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info) = 0;
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata) = 0;
	virtual String get_id() const = 0;
	virtual String get_name() const = 0;
};

struct AudioEffectInfo {

	String caption; ///< Caption of the Node (for node browser menu)
	String description; ///< Short description of the node (for node browser / node info )
	String author; ///< plugin author
	String unique_ID; ///< Unique String ID of node (so it is reconizable when saving)
	String provider_caption;
	String category; ///< String to categorize this node (for node browser)
	String icon_string; ///< icon string (to look up for an bundled icon - internal nodes)
	String version;
	bool synth;
	bool has_ui;
	bool internal;
	String provider_id;
	String path;

	AudioEffectInfo() {
		has_ui = false;
		synth = false;
		internal = false;
	}
};

struct PortRangeHint {
	float min, max, def;
	String max_str, min_str;
};

class ControlPort {
public:
	typedef void (*UIChangedCallback)(void *);

private:
	UIChangedCallback changed_callback;
	void *changed_userdata;
	char command;

public:
	enum Hint {

		HINT_RANGE, ///< just a range, trust min and max
		HINT_RANGE_NORMALIZED, // just a range, but min and max are normalized, so ask text somewhere else
		HINT_TOGGLE, ///< valid values 0.0f , 1.0f
		HINT_ENUM, ///< asking integer values to get_Value_as_text will return them
	};

	virtual String get_name() const = 0;
	virtual String get_identifier() const = 0;

	virtual float get_min() const = 0;
	virtual float get_max() const = 0;
	virtual float get_step() const = 0;
	virtual float get() const = 0;
	virtual bool is_visible() const = 0;

	virtual void set(float p_val) = 0; //set, optionally make the value the default too
	virtual void set_normalized(float p_val); // set in range 0-1, internally converted to range
	virtual float get_normalized() const;

	virtual String get_value_as_text() const;
	virtual Hint get_hint() const;

	void ui_changed_notify();

	void set_ui_changed_callback(UIChangedCallback p_callback, void *p_userdata);
	void set_command(char p_command);
	char get_command() const;

	ControlPort();
	virtual ~ControlPort();
};

class AudioEffect {

	bool skip;

public:
	struct Event {
		enum Type {
			TYPE_NOTE,
			TYPE_NOTE_OFF,
			TYPE_AFTERTOUCH,
			TYPE_BPM,
		};

		enum {
			NOTE_MAX = 0x7F //for note
		};
		Type type;
		uint32_t param8; //for note, BPM
		float paramf; // for note volume (0-1)
		uint32_t offset; //offset in samples (for anything that supports it)
	};

	//process
	virtual bool has_secondary_input() const = 0;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) = 0;
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) = 0;

	virtual void set_process_block_size(int p_size) = 0;
	virtual void set_sampling_rate(int p_hz) = 0;

	typedef void (*MidiEventRoutedDispatchCallback)(const MIDIEventRouted&,void *); // Used for going to external devices

	virtual void set_routed_midi_event_dispatch_callback(MidiEventRoutedDispatchCallback p_callback, void *p_userdata) {}

	//info
	virtual String get_name() const = 0;
	virtual String get_unique_id() const = 0;
	virtual String get_provider_id() const = 0;

	virtual int get_control_port_count() const = 0;
	virtual ControlPort *get_control_port(int p_port) = 0;

	virtual void reset() = 0;

	virtual void mute() {}

	/* Load/Save */

	virtual String get_shared_data_key() const;
	virtual std::shared_ptr<Vector<uint8_t> > get_shared_data() const;
	virtual void set_shared_data(const std::shared_ptr<Vector<uint8_t> > &p_shared_data);

	virtual JSON::Node to_json() const = 0;
	virtual Error from_json(const JSON::Node &node) = 0;

	void set_skip(bool p_skip);
	bool is_skipped() const;

	AudioEffect();
	virtual ~AudioEffect();
};

class ControlPortDefault : public ControlPort {
public:
	String name;
	String identifier;
	float min, max, step;
	float value;
	Hint hint;
	bool visible;
	bool was_set;
	Vector<String> enum_values;

	virtual String get_name() const { return name; }
	virtual String get_identifier() const { return identifier; }

	virtual float get_min() const { return min; }
	virtual float get_max() const { return max; }
	virtual float get_step() const { return step; }
	virtual float get() const { return value; }

	virtual void set(float p_val) {
		value = p_val;
		was_set = true;
	}

	virtual Hint get_hint() const { return hint; }
	virtual bool is_visible() const { return visible; }

	virtual String get_value_as_text() const {
		if (hint == HINT_RANGE || hint == HINT_RANGE_NORMALIZED) {
			return String::num(value);
		} else if (hint == HINT_ENUM) {
			int v = int(value);
			if (v >= 0 && v < enum_values.size()) {
				return enum_values[v];
			} else {
				String::num(v);
			}
		} else {
			if (value > 0.5) {
				return "Enabled";
			} else {
				return "Disabled";
			}
		}
		return String();
	}

	ControlPortDefault() {
		value = 0;
		hint = HINT_RANGE;
		min = 0;
		max = 1;
		step = 1;
		visible = true;
		was_set = false;
	}
};

class AudioEffectFactory {

	Vector<AudioEffectInfo> audio_effects;
	Vector<AudioEffectProvider *> providers;

public:
	void add_audio_effect(AudioEffectInfo p_info);
	int get_audio_effect_count();
	const AudioEffectInfo *get_audio_effect(int p_idx);
	AudioEffect *instantiate_effect(int p_idx);

	void add_provider(AudioEffectProvider *p_provider);
	void rescan_effects(AudioEffectProvider::ScanCallback p_callback = NULL, void *p_userdata = NULL);
};

#endif // AUDIO_EFFECT_H
