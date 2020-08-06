#ifndef AUDIO_EFFECT_PROVIDER_LV2_H
#define AUDIO_EFFECT_PROVIDER_LV2_H

#include "lilv/lilv.h"
#include "suil/suil.h"

#include "drivers/lv2/audio_effect_lv2.h"

// Comment about LV2
// -----------------
// For the past decades, have worked with hundreds of APIs and API designs.
// LV2 is by orders of magnitude the worst-designed API I've ever seen
// in my entire life. It's just ridiculous how bloated it is.
//
// When LV2 was created, VST2 already existed, which
// may not be brilliant but managed to have every single thing audio
// developers want within a single header. It also shown
// how easy it was to extend it. The amount of plugins created for it
// is a testament about the superiority of simple designs.
//
// In contrast, LV2 is a pure abstract API using atoms and URIs,
// which are literally impossible to understand by reading the
// specification or the source headers. Information is scattered all
// over the place. To use the LV2, you also need half a dozen support
// libraries.
//
// There is zero documentation on creating hosts, and the only example
// (JALV) is thousands of lines of code long (compare to VST, an entire
// host is done in just a dozen lines of code).
//
// It will remain a challenge for software archeologists to figure out
// how the Linux Audio community screwed up so bad on this, and
// how did they allow it to happen.

class AudioEffectProviderLV2 : public AudioEffectProvider {
public:
	struct URIS {

		LilvNode *atom_port;
		LilvNode *audio_port;
		LilvNode *control_port;
		LilvNode *cv_port;
		LilvNode *event_port;
		LilvNode *input_port;
		LilvNode *midi_event;
		LilvNode *output_port;
		LilvNode *port;

		LilvNode *port_minimum_size;
		LilvNode *atom_chunk;
		LilvNode *atom_sequence;

		LilvNode *lv2_AudioPort;
		LilvNode *lv2_CVPort;
		LilvNode *lv2_ControlPort;
		LilvNode *lv2_InputPort;
		LilvNode *lv2_OutputPort;
		LilvNode *lv2_connectionOptional;
		LilvNode *lv2_control;
		LilvNode *lv2_default;
		LilvNode *lv2_enumeration;
		LilvNode *lv2_integer;
		LilvNode *lv2_maximum;
		LilvNode *lv2_minimum;
		LilvNode *lv2_name;
		LilvNode *lv2_reportsLatency;
		LilvNode *lv2_sampleRate;
		LilvNode *lv2_symbol;
		LilvNode *lv2_toggled;

		LilvNode *work_interface;
	};

private:
#if 0
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
	} urid;

	struct {
		LilvNode* atom_AtomPort;
		LilvNode* atom_Chunk;
		LilvNode* atom_Float;
		LilvNode* atom_Path;
		LilvNode* atom_Sequence;
		LilvNode* ev_EventPort;
		LilvNode* lv2_AudioPort;
		LilvNode* lv2_CVPort;
		LilvNode* lv2_ControlPort;
		LilvNode* lv2_InputPort;
		LilvNode* lv2_OutputPort;
		LilvNode* lv2_connectionOptional;
		LilvNode* lv2_control;
		LilvNode* lv2_default;
		LilvNode* lv2_enumeration;
		LilvNode* lv2_integer;
		LilvNode* lv2_maximum;
		LilvNode* lv2_minimum;
		LilvNode* lv2_name;
		LilvNode* lv2_reportsLatency;
		LilvNode* lv2_sampleRate;
		LilvNode* lv2_symbol;
		LilvNode* lv2_toggled;
		LilvNode* midi_MidiEvent;
		LilvNode* pg_group;
		LilvNode* pprops_logarithmic;
		LilvNode* pprops_notOnGUI;
		LilvNode* pprops_rangeSteps;
		LilvNode* pset_Preset;
		LilvNode* pset_bank;
		LilvNode* rdfs_comment;
		LilvNode* rdfs_label;
		LilvNode* rdfs_range;
		LilvNode* rsz_minimumSize;
		LilvNode* work_interface;
		LilvNode* work_schedule;
		LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
	} nodes;

	Symap *symap;

	LV2_Atom_Forge forge;
	LV2_URID_Map map;
	SerdEnv env;
	Sratom *sratom; ///< Atom serialiser
	Sratom *ui_sratom; ///< Atom serialiser for UI thread
	uint32_t midi_event_id; ///< MIDI event class ID in event context


	String temp_dir;
#endif

	static URIS uris;
	LilvWorld *world;

	AudioEffect *create_effect(const AudioEffectInfo *p_info);
	static AudioEffect *create_effects(const AudioEffectInfo *p_info);
	//static uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data, const char *map, const char *uri);
	friend class AudioEffectLV2;
	static AudioEffectProviderLV2 *singleton;

	const LilvPlugins *plugins;

public:
	static const URIS &get_uris() { return uris; }
	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info);
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata);
	virtual String get_id() const;
	virtual String get_name() const;

	AudioEffectProviderLV2(int *argc, char ***argv);
};

#endif // AUDIO_EFFECT_FACTORY_LV2_H
