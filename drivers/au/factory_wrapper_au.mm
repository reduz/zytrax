#ifdef OSX_ENABLED

#include "factory_wrapper_au.h"
#include "effect_editor_au.h"
#include "audio_effect_provider_au.h"

AudioEffectProvider *create_au_provider() {
	return new AudioEffectProviderAU();
}

// get_au_editor_function is defined in effect_editor_au.mm

#endif // OSX_ENABLED
