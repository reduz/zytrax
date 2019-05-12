#ifndef AUDIOEFFECTFACTORYVST_H
#define AUDIOEFFECTFACTORYVST_H

#include "drivers/vst2/audio_effect_vst2.h"

class AudioEffectProviderVST2 : public AudioEffectProvider {

	static VstIntPtr VSTCALLBACK host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	friend class AudioEffectVST2;

	static AEffect *open_vst_from_lib_handle(HINSTANCE libhandle, audioMasterCallback p_master_callback);

public:
	static AudioEffectProviderVST2 *singleton;

	virtual String get_name() const;
	virtual String get_id() const;

	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info);
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata);

	AudioEffectProviderVST2();
	~AudioEffectProviderVST2();
};

#endif // AUDIOEFFECTFACTORYVST_H
