#ifndef AUDIO_EFFECT_H
#define AUDIO_EFFECT_H

#include "dsp/frame.h"
#include "dsp/midi_event.h"
#include "globals/json.h"
#include "rstring.h"
#include "vector.h"

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
	String short_caption; ///< Short Caption of the Node (for audio graph)
	String description; ///< Short description of the node (for node browser / node info )
	String author; ///< plugin author
	String long_description; ///< Long description of the node (for node browser / node info )
	String unique_ID; ///< Unique String ID of node (so it is reconizable when saving)
	String provider_caption;
	String category; ///< String to categorize this node (for node browser)
	String icon_string; ///< icon string (to look up for an bundled icon - internal nodes)
	String version;
	bool synth;
	bool has_ui;
	String provider_id;
	String path;
};

struct PortRangeHint {
	float min, max, def;
	String max_str, min_str;
};

class ControlPort {
public:
	enum Hint {

		HINT_RANGE, ///< just a range (Default)
		HINT_TOGGLE, ///< valid values 0.0f , 1.0f
		HINT_ENUM, ///< asking integer values to get_Value_as_text will return them
	};

	virtual String get_name() const = 0;
	virtual String get_identifier() const = 0;

	virtual float get_min() const = 0;
	virtual float get_max() const = 0;
	virtual float get_step() const = 0;
	virtual float get_initial() const = 0;
	virtual float get() const = 0;
	virtual bool is_visible() const = 0;

	virtual void set(float p_val, bool p_make_initial = false) = 0; //set, optionally make the value the default too
	virtual void set_normalized(float p_val, bool p_make_initial = false); // set in range 0-1, internally converted to range
	virtual float get_normalized() const;

	virtual String get_value_as_text() const;
	virtual Hint get_hint() const;

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
		uint32_t param8; //for note
		float paramf; // for note volume or aftertouch (0-1) or bpm (float)
		uint32_t offset; //offset in samples (for anything that supports it)
	};

	//process
	virtual bool has_secondary_input() const = 0;
	virtual void process(const Event *p_events, int p_event_count, const Frame *p_in, Frame *p_out, bool p_prev_active) = 0;
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const Frame *p_in, const Frame *p_secondary, Frame *p_out, bool p_prev_active) = 0;

	//info
	virtual String get_name() const = 0;
	virtual String get_unique_id() const = 0;
	virtual String get_provider_id() const = 0;

	virtual int get_control_port_count() const = 0;
	virtual ControlPort *get_control_port(int p_port) = 0;

	virtual void reset() = 0;

	/* Load/Save */

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
	float min, max, step, initial;
	float value;
	Hint hint;
	bool visible;
	bool was_set;

	virtual String get_name() const { return name; }
	virtual String get_identifier() const { return identifier; }

	virtual float get_min() const { return min; }
	virtual float get_max() const { return max; }
	virtual float get_step() const { return step; }
	virtual float get_initial() const { return initial; }
	virtual float get() const { return value; }

	virtual void set(float p_val, bool p_make_initial = false) {
		value = p_val;
		if (p_make_initial)
			initial = value;
		was_set = true;
	}

	virtual Hint get_hint() const { return hint; }
	virtual bool is_visible() const { return visible; }

	ControlPortDefault() {
		hint = HINT_RANGE;
		min = 0;
		max = 1;
		step = 1;
		initial = 0;
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
