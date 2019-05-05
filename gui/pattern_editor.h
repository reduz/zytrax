#ifndef PATTERN_EDITOR_H
#define PATTERN_EDITOR_H

#include "engine/song.h"
#include "engine/undo_redo.h"
#include <gtkmm/cssprovider.h>
#include <gtkmm/styleproperty.h>
#include <gtkmm/widget.h>

#include "gui/color_theme.h"
#include "gui/key_bindings.h"

class PatternEditor : public Gtk::Widget {
public:
	enum BeatZoom {
		BEAT_ZOOM_1,
		BEAT_ZOOM_2,
		BEAT_ZOOM_3,
		BEAT_ZOOM_4,
		BEAT_ZOOM_6,
		BEAT_ZOOM_8,
		BEAT_ZOOM_12,
		BEAT_ZOOM_16,
		BEAT_ZOOM_24,
		BEAT_ZOOM_32,
		BEAT_ZOOM_48,
		BEAT_ZOOM_64,
		BEAT_ZOOM_MAX
	};

protected:
	//Overrides:
	Gtk::SizeRequestMode get_request_mode_vfunc() const override;
	void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
	void get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const override;
	void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
	void get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const override;
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_map() override;
	void on_unmap() override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	//Signal handler:
	void on_parsing_error(const Glib::RefPtr<const Gtk::CssSection> &section, const Glib::Error &error);

	void _mouse_button_event(GdkEventButton *event, bool p_press);

	bool on_scroll_event(GdkEventScroll *scroll_event);
	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
	bool on_motion_notify_event(GdkEventMotion *motion_event);
	bool on_key_press_event(GdkEventKey *key_event);
	bool on_key_release_event(GdkEventKey *key_event);

	Glib::RefPtr<Gdk::Window> m_refGdkWindow;
	UndoRedo *undo_redo;

	Song *song;

	int current_pattern;
	int current_octave;
	int cursor_advance;
	int volume_mask;
	bool volume_mask_active;
	int v_offset;
	BeatZoom beat_zoom;
	int h_offset;
	int visible_rows;

	int row_height_cache;
	int row_top_ofs;

	struct Cursor {
		int row;
		int column;
		int field;
		int skip;
	} cursor;

	struct Selection {
		int begin_column;
		Tick begin_tick;

		int end_column;
		Tick end_tick;

		Tick row_tick_size;

		bool active;

		int shift_from_column;
		int shift_from_row;
		bool shift_active;

		int mouse_drag_from_column;
		int mouse_drag_from_row;
		bool mouse_drag_active;

	} selection;

	struct Clipboard {

		List<Track::PosEvent> events;
		int columns;
		Tick ticks;
		bool active;
	} clipboard;

	void _update_shift_selection();
	bool _is_in_selection(int p_column, Tick p_tick);

	struct ClickArea {

		int column;

		struct Field {
			int x;
			int width;
		};

		Vector<Field> fields;

		Automation *automation;

		struct AutomationPoint {

			int index;
			int x, y;
			Tick tick;
		};

		List<AutomationPoint> automation_points;

		ClickArea() {
			automation = NULL;
		}
	};

	int fw_cache;
	int fh_cache;

	int get_column_offset(int p_column);

	List<ClickArea> click_areas;

	int grabbing_point;
	Tick grabbing_point_tick_from;
	uint8_t grabbing_point_value_from;
	Tick grabbing_point_tick;
	uint8_t grabbing_point_value;
	Automation *grabbing_automation;
	int grabbing_x, grabbing_width;
	int grabbing_mouse_pos_x;
	int grabbing_mouse_pos_y;

	int grabbing_mouse_prev_x;
	int grabbing_mouse_prev_y;

	int get_total_rows() const;
	int get_visible_rows() const;

	void _cursor_advance();

	void get_cursor_column_data(Track **r_track, int &r_automation, int &r_track_column);

	void _draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, const String &p_text, const Gdk::RGBA &p_color, bool p_down = false);
	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);
	void _draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);
	void _draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);

	void _field_clear();
	void _validate_cursor();
	void _validate_selection();
	void _redraw();

	Theme *theme;
	KeyBindings *key_bindings;

	int _get_rows_per_beat() const;
	void _on_action_activated(KeyBindings::KeyBind p_bind);
	void _validate_menus();
	void _notify_track_layout_changed();

	bool drawing;
	int last_amplify_value;
	float last_scale_value;

	Glib::RefPtr<Gtk::Adjustment> h_scroll;
	Glib::RefPtr<Gtk::Adjustment> v_scroll;

	void _v_scroll_changed();
	void _h_scroll_changed();

public:
	sigc::signal1<void, int> track_edited;
	sigc::signal0<void> track_layout_changed;
	sigc::signal0<void> current_track_changed;

	sigc::signal0<void> volume_mask_changed;
	sigc::signal0<void> octave_changed;
	sigc::signal0<void> step_changed;
	sigc::signal0<void> zoom_changed;
	sigc::signal0<void> pattern_changed;

	void set_current_pattern(int p_pattern);
	int get_current_pattern() const;

	void set_current_octave(int p_octave);
	int get_current_octave() const;

	void set_current_cursor_advance(int p_cursor_advance);
	int get_current_cursor_advance() const;

	void set_current_volume_mask(int p_volume_mask, bool p_active);
	int get_current_volume_mask() const;
	bool is_current_volume_mask_active() const;

	void set_beat_zoom(BeatZoom p_zoom);
	BeatZoom get_beat_zoom() const;

	int get_current_track() const;

	void set_hscroll(Glib::RefPtr<Gtk::Adjustment> p_h_scroll);
	void set_vscroll(Glib::RefPtr<Gtk::Adjustment> p_v_scroll);

	PatternEditor(Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme, KeyBindings *p_bindings);
	~PatternEditor();
};

#endif // PATTERN_EDITOR_H
