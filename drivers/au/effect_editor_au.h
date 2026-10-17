#ifndef EFFECT_EDITOR_AU_H
#define EFFECT_EDITOR_AU_H

#ifdef OSX_ENABLED

#include "audio_effect_au.h"
#include "gui/effect_editor.h"
#include "gui/effect_editor_midi.h"
#include <gtkmm.h>

class EffectPlaceholderAU : public Gtk::DrawingArea {
	AudioEffectAU *au_effect;
	void *au_view;  // NSView* (+1 retain)
	void *au_panel; // NSPanel* (+1 retain), hosts the AU view as a floating child window

	int au_w, au_h;

	sigc::connection update_timer;
	bool _update_position();
	void _close_panel();

public:
	void on_realize()   override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
	void on_size_allocate(Gtk::Allocation &alloc) override;

	EffectPlaceholderAU(AudioEffectAU *p_effect);
	~EffectPlaceholderAU();
};

class EffectEditorAU : public Gtk::VBox {
	AudioEffectAU    *au_effect;
	EffectEditorMIDI  effect_editor_midi;
	EffectPlaceholderAU *placeholder;

public:
	EffectEditorAU(AudioEffectAU *p_au, EffectEditor *p_editor);
	~EffectEditorAU();
};

EffectEditorPluginFunc get_au_editor_function();

#endif // OSX_ENABLED
#endif // EFFECT_EDITOR_AU_H
