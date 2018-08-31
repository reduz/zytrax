#ifndef AUDIO_EFFECT_FACTORY_LV2_H
#define AUDIO_EFFECT_FACTORY_LV2_H

#include "lilv/lilv.h"
#include "suil/suil.h"

#include "engine/audio_effect.h"

class AudioEffectLV2 : public AudioEffect {

	Vector<float> in_buff_left;
	Vector<float> in_buff_right;

	Vector<float> out_buff_left;
	Vector<float> out_buff_right;

	LV2_Evbuf* evbuf;

	int buff_size;

	int mix_rate;
	AudioEffectInfo info;
	bool active;

	LilvInstance *instance;
	LilvInstance *instance_mono_pair; //for mono plugins, need a pair

	Vector<ControlPortDefault> controls;
	Vector<float> control_outs; //outs are unused
public:
	//process
	virtual bool process(const AudioFrame2 *p_in, AudioFrame2 *p_out, const Event *p_events, bool p_prev_active);

	//info
	virtual bool has_synth() const;

	virtual const AudioEffectInfo *get_info() const;

	virtual int get_control_port_count() const;
	virtual ControlPort *get_control_port(int p_port);
	const ControlPort *get_control_port(int p_port) const;

	virtual void reset();

	/* Load/Save */

	virtual Error save(TreeSaver *p_tree);
	virtual Error load(TreeLoader *p_tree);

	AudioEffectLV2();
	virtual ~AudioEffectLV2();
};

class AudioEffectProviderLV2 : public AudioEffectProvider {

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

	struct {

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
	} uris;
	LilvWorld *world;

	AudioEffect *create_effect(const AudioEffectInfo *p_info);
	static AudioEffect *create_effects(const AudioEffectInfo *p_info);
	//static uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data, const char *map, const char *uri);
friend class AudioEffectLV2;
	static AudioEffectProviderLV2 *singleton;
public:
	virtual void scan_effects(AudioEffectFactory *p_factory);
	virtual String get_name() const;
	AudioEffectProviderLV2(int *argc, char ***argv);
};

#endif // AUDIO_EFFECT_FACTORY_LV2_H
