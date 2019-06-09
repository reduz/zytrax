#ifndef EFFECT_EDITOR_X11_H
#define EFFECT_EDITOR_X11_H

#ifdef FREEDESKTOP_ENABLED
#include "drivers/vst2/effect_editor_vst2.h"

int vstfx_init();
void vstfx_exit();

int vstfx_run_editor(AudioEffectVST2 *p_effect, EffectEditorVST2 *p_editor);
void vstfx_get_window_size(AudioEffectVST2 *p_effect, int *w, int *h);
void vstfx_destroy_editor(AudioEffectVST2 *p_effect);

#endif

#endif // EFFECT_EDITOR_X11_H
