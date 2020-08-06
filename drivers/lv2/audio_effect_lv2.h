#ifndef AUDIO_EFFECT_LV2_H
#define AUDIO_EFFECT_LV2_H

#include "globals/map.h"

#include "engine/audio_effect_midi.h"
#include "lilv/lilv.h"
#include "suil/suil.h"

#include "drivers/lv2/lv2_evbuf.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
#include "lv2/lv2plug.in/ns/ext/data-access/data-access.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/parameters/parameters.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/port-groups/port-groups.h"
#include "lv2/lv2plug.in/ns/ext/port-props/port-props.h"
#include "lv2/lv2plug.in/ns/ext/presets/presets.h"
#include "lv2/lv2plug.in/ns/ext/resize-port/resize-port.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "sratom/sratom.h"
#include <atomic>

#define NS_EXT "http://lv2plug.in/ns/ext/"

class AudioEffectLV2 : public AudioEffectMIDI {

	const LilvPlugin *plugin;

	LilvInstance *instance;
	LilvInstance *instance2; //stereo pair

	LV2_Extension_Data_Feature ext_data;

	// URI BS

	struct CStringKey {
		const char *cstring;
		bool operator<(const CStringKey &p_key) const {
			return strcmp(cstring, p_key.cstring) < 0;
		}
	};

	Map<CStringKey, LV2_URID> uri_id_map;
	Map<LV2_URID, CStringKey> id_uri_map;

	std::atomic_flag locked = ATOMIC_FLAG_INIT;

	LV2_URID uri_to_idm(const char *uri);
	const char *id_to_urim(LV2_URID id);

	LV2_Atom_Forge forge;

	LV2_URID_Map map; ///< URI => Int map
	LV2_URID_Unmap unmap; ///< Int => URI map
	struct {
		LV2_URID atom_Float;
		LV2_URID atom_Int;
		LV2_URID atom_Object;
		LV2_URID atom_Path;
		LV2_URID atom_String;
		LV2_URID atom_eventTransfer;
		LV2_URID bufsz_maxBlockLength;
		LV2_URID bufsz_minBlockLength;
		LV2_URID bufsz_sequenceSize;
		LV2_URID log_Error;
		LV2_URID log_Trace;
		LV2_URID log_Warning;
		LV2_URID midi_MidiEvent;
		LV2_URID param_sampleRate;
		LV2_URID patch_Get;
		LV2_URID patch_Put;
		LV2_URID patch_Set;
		LV2_URID patch_body;
		LV2_URID patch_property;
		LV2_URID patch_value;
		LV2_URID time_Position;
		LV2_URID time_bar;
		LV2_URID time_barBeat;
		LV2_URID time_beatUnit;
		LV2_URID time_beatsPerBar;
		LV2_URID time_beatsPerMinute;
		LV2_URID time_frame;
		LV2_URID time_speed;
		LV2_URID ui_updateRate;
	} urids;

	uint32_t midi_event_id; ///< MIDI event class ID in event context

	//buffers

	LV2_Evbuf *evbuf;

	Vector<Vector<float> > in_buffers;
	Vector<float *> in_buffer_ptrs;

	Vector<Vector<float> > out_buffers;
	Vector<float *> out_buffer_ptrs;

	float samplerate_port;

	struct LV2ControlPort : public ControlPort {
		String name;
		String identifier;
		float min;
		float max;
		bool needs_sr_multiplier;
		float multiplier;
		float value;

		virtual String get_name() const { return name; }
		virtual String get_identifier() const { return identifier; }

		virtual float get_min() const { return min * multiplier; }
		virtual float get_max() const { return max * multiplier; }
		virtual float get_step() const { return 0; }
		virtual float get() const { return value * multiplier; }
		virtual bool is_visible() const { return true; }

		virtual void set(float p_val) { value /= multiplier; }
	};

	Vector<LV2ControlPort *> control_ports;

	float out_control_trash;

	int buffer_size;
	int sampling_rate;

	void _update_buffers_and_ports();

	void _plugin_init();
	void _plugin_finish();

	String name;
	String identifier;

	bool has_ui;

	void _process_events(const Event *p_events, int p_event_count);

public:
	static LV2_URID map_uri(LV2_URID_Unmap_Handle handle,
			const char *uri);
	static const char *unmap_uri(LV2_URID_Unmap_Handle handle,
			LV2_URID urid);
	static uint32_t
	uri_to_id(LV2_URI_Map_Callback_Data callback_data,
			const char *map,
			const char *uri);

	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);

	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);
	//info
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual void reset();

	/* Load/Save */

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	Error setup(const LilvPlugin *p_plugin, bool p_has_ui);

	AudioEffectLV2();
	~AudioEffectLV2();
};

#endif // AUDIO_EFFECT_LV2_H
