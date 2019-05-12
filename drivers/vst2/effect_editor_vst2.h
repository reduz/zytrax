#ifndef EFFECT_EDITOR_VST2_H
#define EFFECT_EDITOR_VST2_H

#include "gui/effect_editor_midi.h"
#include <gdk/gdkwin32.h>

class AudioEffectVST2;

class EffectPlaceholderVST2 : public Gtk::Widget {

	AudioEffectVST2 *vst_effect;
	Glib::RefPtr<Gdk::Window> m_refGdkWindow;
	int vst_w;
	int vst_h;
	HWND vst_window;

	sigc::connection update_timer;
	static void _vst_resize(void *self, int w, int h);

	bool _update_window_position();
	void resize_editor(int left, int top, int right, int bottom);

	int prev_x, prev_y, prev_w, prev_h;
	bool prev_visible;

public:
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
	bool on_visibility_notify_event(GdkEventVisibility *visibility_event) override;

	EffectPlaceholderVST2(AudioEffectVST2 *p_vst_effect);
	~EffectPlaceholderVST2();
};

class EffectEditorVST2 : public Gtk::VBox {

	AudioEffectVST2 *vst_effect;
	EffectEditorMIDI effect_editor_midi;
	EffectPlaceholderVST2 vst_placeholder;

public:
	EffectEditorVST2(AudioEffectVST2 *p_vst, EffectEditor *p_editor);
};

void initialize_vst2_editor();

#endif // EFFECT_EDITOR_VST2_H
