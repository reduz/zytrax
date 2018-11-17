#ifndef AUDIOEFFECTFACTORYVST_H
#define AUDIOEFFECTFACTORYVST_H

#include "engine/audio_effect.h"
#include "vst/aeffectx.h"
#include "globals/map.h"
class AudioEffectProviderVST2 : public AudioEffectProvider {

	struct VST_Struct {

		String path;
		String dir;
		bool write_only;
	};

	Map<String,VST_Struct> plugins;

	String paths;
	static VstIntPtr VSTCALLBACK host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static AudioEffect *creation_func(const AudioEffectInfo *); ///< creation function for instancing this node

public:
	void set_paths(const String& p_paths);
	String get_paths() const;

	virtual void scan_effects(AudioEffectFactory *p_factory) ;
	virtual String get_name() const;

	AudioEffectProviderVST2();
	~AudioEffectProviderVST2();
};

#endif // AUDIOEFFECTFACTORYVST_H
