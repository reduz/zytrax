#ifndef MASTER_VU_H
#define MASTER_VU_H
#include "engine/song.h"
#include "engine/undo_redo.h"
#include <gtkmm/cssprovider.h>
#include <gtkmm/styleproperty.h>
#include <gtkmm/widget.h>

#include "gui/color_theme.h"
#include "gui/key_bindings.h"

class MasterVU : public Gtk::Widget {
protected:
	enum {
		TRACK_MAX_DB = 12,
		TRACK_MIN_DB = -60
	};
	int min_width;
	int min_height;
	int char_width;
	int font_height;
	int font_ascent;

	int vu_x, vu_y;
	int vu_w, vu_h;

	int grabber_x, grabber_y;
	int grabber_w, grabber_h;

	bool grabbing;
	float grabbing_db;
	int grabbing_x;

	// Overrides:
	Gtk::SizeRequestMode get_request_mode_vfunc() const override;
	void get_preferred_width_vfunc(int &minimum_width,
			int &natural_width) const override;
	void get_preferred_height_for_width_vfunc(int width, int &minimum_height,
			int &natural_height) const override;
	void get_preferred_height_vfunc(int &minimum_height,
			int &natural_height) const override;
	void get_preferred_width_for_height_vfunc(int height, int &minimum_width,
			int &natural_width) const override;
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_map() override;
	void on_unmap() override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	// Signal handler:
	void on_parsing_error(const Glib::RefPtr<const Gtk::CssSection> &section,
			const Glib::Error &error);

	void _mouse_button_event(GdkEventButton *event, bool p_press);

	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
	bool on_motion_notify_event(GdkEventMotion *motion_event);
	bool on_key_press_event(GdkEventKey *key_event);
	bool on_key_release_event(GdkEventKey *key_event);

	Glib::RefPtr<Gdk::Window> m_refGdkWindow;
	UndoRedo *undo_redo;

	Theme *theme;

	Song *song;

	void _draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, const String &p_text,
			const Gdk::RGBA &p_color, bool p_down);
	int _get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const;
	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
			int x, int y, int w, int h,
			const Gdk::RGBA &p_color);
	void _draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, int w, int h, const Gdk::RGBA &p_color);

	void _draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, int w, int h, const Gdk::RGBA &p_color);

	uint64_t last_time;
	float peak_db_l;
	float peak_db_r;

public:
	sigc::signal1<void, float> main_volume_db_changed;

	void update_peak();

	MasterVU(Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme);
	~MasterVU();
};

#endif // MASTER_VU_H
