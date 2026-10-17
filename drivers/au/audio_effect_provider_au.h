#ifndef AUDIO_EFFECT_PROVIDER_AU_H
#define AUDIO_EFFECT_PROVIDER_AU_H

#ifdef OSX_ENABLED

#include "audio_effect_au.h"
#include "engine/audio_effect.h"

class AudioEffectProviderAU : public AudioEffectProvider {
public:
	static AudioEffectProviderAU *singleton;

	virtual String get_name() const;
	virtual String get_id() const;

	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info);
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata);

	AudioEffectProviderAU();
	~AudioEffectProviderAU();
};

#endif // OSX_ENABLED
#endif // AUDIO_EFFECT_PROVIDER_AU_H
