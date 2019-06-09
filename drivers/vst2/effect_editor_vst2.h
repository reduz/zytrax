#ifndef EFFECT_EDITOR_VST2_H
#define EFFECT_EDITOR_VST2_H

#include "gui/effect_editor_midi.h"
#ifdef WINDOWS_ENABLED
#include <gdk/gdkwin32.h>
#else
#endif

#ifdef FREEDESKTOP_ENABLED
#include <gtkmm/socket.h>
#endif

class AudioEffectVST2;

#ifdef WINDOWS_ENABLED
class EffectPlaceholderVST2Win32 : public Gtk::Widget {

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

	EffectPlaceholderVST2Win32(AudioEffectVST2 *p_vst_effect);
	~EffectPlaceholderVST2Win32();
};

#endif

class EffectEditorVST2 : public Gtk::VBox {

	AudioEffectVST2 *vst_effect;
	EffectEditorMIDI effect_editor_midi;
#ifdef WINDOWS_ENABLED

	EffectPlaceholderVST2Win32 vst_placeholder;
#endif

#ifdef FREEDESKTOP_ENABLED
	int xid;
	Gtk::Socket socket;
#endif

	sigc::connection init_timer;
	bool initialize();

public:
	EffectEditorVST2(AudioEffectVST2 *p_vst, EffectEditor *p_editor);
	~EffectEditorVST2();
};

void initialize_vst2_editor();
void finalize_vst2_editor();

#endif // EFFECT_EDITOR_VST2_H
