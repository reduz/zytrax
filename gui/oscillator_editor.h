#ifndef OSCILLATOR_EDITOR_H
#define OSCILLATOR_EDITOR_H


#include <gtkmm/widget.h>
#include "gui/color_theme.h"
#include "dsp/oscillator.h"

class OscillatorEditor : public Gtk::Widget {
protected:

	OscillatorType osc_type = OSC_TYPE_SINE;
	float shape = 0.0;
	Theme *theme = nullptr;

	// Overrides:
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_map() override;
	void on_unmap() override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
			int x, int y, int w, int h,
			const Gdk::RGBA &p_color);

	void _draw_line(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, int tx, int ty, const Gdk::RGBA &p_color);

	Glib::RefPtr<Gdk::Window> m_refGdkWindow;

public:

	void set_osc_type(OscillatorType p_type);
	void set_shape(float p_shape);

	OscillatorEditor(Theme *p_theme);
	~OscillatorEditor();
};


#endif // OSCILLATOR_EDITOR_H
