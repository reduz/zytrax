#ifndef FILTER_ARRAY_EDITOR_H
#define FILTER_ARRAY_EDITOR_H

#include "dsp/filter.h"
#include "gui/color_theme.h"
#include <gtkmm.h>

class FilterArrayEditor : public Gtk::Widget {

private:
	enum {
		MIN_FREQ = 20,
		MAX_FREQ = 16000,
		AMP_STEPS = 3,
		NUMBER_H=12

	};

	static constexpr float RESONANCE_MIN = 0;
	static constexpr float RESONANCE_MAX = 1;

	int size_w = 0;
	int size_h = 0;

	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
	bool on_motion_notify_event(GdkEventMotion *motion_event);


	Glib::RefPtr<Gdk::Window> m_refGdkWindow;

	Theme *theme;

	void _draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, const String &p_text,
			const Gdk::RGBA &p_color, bool p_down);
	int _get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const;

	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
			int x, int y, int w, int h,
			const Gdk::RGBA &p_color);
	void _draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, int w, int h, const Gdk::RGBA &p_color);



	struct Drag {

		int index = -1;
		int over = -1;
	} drag;

	struct FilterData {
		Filter::Mode mode = Filter::LOWPASS;
		float cutoff = MIN_FREQ;
		float resonance = 0.0;
	};


	FilterData *filter = nullptr;
	int filter_max = 0;
	int filters = 0;

	int stages = 1;

	Glib::RefPtr<Gio::Menu> new_filter_menu;
	Gtk::Menu new_filter_popup;

	void _filter_added(int p_idx);
public:

	sigc::signal0<void> filter_changed;

	void set_filter_count(int p_filters);
	int get_filter_count() const;

	void set_filter_stages(int p_filters);
	int get_filter_stages() const;

	void set_filter_mode(int p_index, Filter::Mode p_mode);
	Filter::Mode get_filter_mode(int p_index) const;

	void set_filter_cutoff(int p_index, float p_cutoff);
	float get_filter_cutoff(int p_index) const;

	void set_filter_resonance(int p_index, float p_resonance);
	float get_filter_resonance(int p_index) const;

	FilterArrayEditor(int p_filter_max, Theme *p_theme);
	~FilterArrayEditor();

};


#endif // FILTER_ARRAY_EDITOR_H
