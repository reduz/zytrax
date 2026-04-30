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
	static URIS uris;
	LilvWorld *world;
	AudioEffect *create_effect(const AudioEffectInfo *p_info);
	static AudioEffect *create_effects(const AudioEffectInfo *p_info);
	//static uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data, const char *map, const char *uri);
	friend class AudioEffectLV2;
	const LilvPlugins *plugins;

public:
	static AudioEffectProviderLV2 *singleton;
	static const URIS &get_uris() { return uris; }
	static LilvWorld *get_world() { return singleton ? singleton->world : NULL; }
	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info);
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata);
	virtual String get_id() const;
	virtual String get_name() const;
	AudioEffectProviderLV2(int *argc, char ***argv);
	~AudioEffectProviderLV2();
};

#endif // AUDIO_EFFECT_FACTORY_LV2_H
