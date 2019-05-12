#ifndef FACTORY_WRAPPER_VST2_H
#define FACTORY_WRAPPER_VST2_H

#include "engine/song.h"
#include "gui/effect_editor.h"

AudioEffectProvider *create_vst2_provider();
EffectEditorPluginFunc get_vst2_editor_function();

#endif // FACTORY_WRAPPER_VST2_H
