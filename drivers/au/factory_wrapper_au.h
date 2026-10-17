#ifndef FACTORY_WRAPPER_AU_H
#define FACTORY_WRAPPER_AU_H

#ifdef OSX_ENABLED

#include "engine/audio_effect.h"
#include "gui/effect_editor.h"

AudioEffectProvider   *create_au_provider();
EffectEditorPluginFunc get_au_editor_function();

#endif // OSX_ENABLED
#endif // FACTORY_WRAPPER_AU_H
