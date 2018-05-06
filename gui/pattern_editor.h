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
protected:
	enum {
		TRACK_MENU_ADD_COLUMN,
		TRACK_MENU_REMOVE_COLUMN,
		TRACK_MENU_SOLO,
		TRACK_MENU_MUTE,
		TRACK_MENU_EDIT_AUTOMATIONS,
		TRACK_MENU_RENAME,
		TRACK_MENU_MOVE_LEFT,
		TRACK_MENU_MOVE_RIGHT,
		TRACK_MENU_ADD_EFFECT,
		TRACK_MENU_REMOVE,
		AUTOMATION_MENU_VISIBLE,
		AUTOMATION_MENU_MODE_ROWS,
		AUTOMATION_MENU_MODE_SMALL,
		AUTOMATION_MENU_MODE_LARGE,
		AUTOMATION_MENU_MOVE_LEFT,
		AUTOMATION_MENU_MOVE_RIGHT,
		AUTOMATION_MENU_REMOVE,
		BASE_EFFECT = 100
	};

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
	int v_offset;
	int rows_per_beat;
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

	struct TrackButton {
		int track;
		Gdk::Rectangle r;
	};

	List<TrackButton> track_buttons;

	struct AutomationButton {
		int track;
		int automation;
		Gdk::Rectangle r;
	};

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

	List<ClickArea> click_areas;

	int current_menu_track;
	int current_menu_automation;

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

	List<AutomationButton> automation_buttons;

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
	void _redraw();

	Theme *theme;
	KeyBindings *key_bindings;

	Vector<Gtk::MenuItem *> track_menu_items;
	Gtk::Menu *track_menu;
	Vector<Gtk::MenuItem *> automation_menu_items;
	Gtk::Menu *automation_menu;

	void _menu_option(int p_option);
	void _add_option_to_menu(Gtk::Menu *menu, const char *p_text, int p_menu, Vector<Gtk::MenuItem *> &items);
	void _add_check_option_to_menu(Gtk::Menu *menu, bool p_checked, bool p_radio, const char *p_text, int p_menu, Vector<Gtk::MenuItem *> &items);
	void _add_separator_to_menu(Gtk::Menu *menu, Vector<Gtk::MenuItem *> &items, const String &p_text = String());

public:
	void set_current_pattern(int p_pattern);
	int get_current_pattern() const;

	PatternEditor(Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme, KeyBindings *p_bindings);
	~PatternEditor();
};

#endif // PATTERN_EDITOR_H
