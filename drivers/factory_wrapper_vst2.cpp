#include "factory_wrapper_vst2.h"
#include "audio_effect_factory_vst.h"

AudioEffectProvider *create_vst2_provider() {
	return new AudioEffectProviderVST2;
}
