
#include "effect_editor_vst2.h"
//for include order, do not delete this comment
#include "audio_effect_provider_vst2.h"
#include "factory_wrapper_vst2.h"

AudioEffectProvider *create_vst2_provider() {
	initialize_vst2_editor();
	return new AudioEffectProviderVST2;
}

static Gtk::Widget *create_vst2_editor(AudioEffect *p_vst, EffectEditor *p_editor) {
	if (p_vst->get_provider_id() != AudioEffectProviderVST2::singleton->get_id()) {
		return NULL;
	}
	return new EffectEditorVST2(static_cast<AudioEffectVST2 *>(p_vst), p_editor);
}

EffectEditorPluginFunc get_vst2_editor_function() {
	return &create_vst2_editor;
}
