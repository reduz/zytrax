#ifndef TRACK_EDITOR_H
#define TRACK_EDITOR_H

#include "engine/song.h"
#include "engine/undo_redo.h"
#include <gtkmm/cssprovider.h>
#include <gtkmm/styleproperty.h>
#include <gtkmm/widget.h>

#include "gui/color_theme.h"
#include "gui/key_bindings.h"

class TrackRackVolume : public Gtk::Widget {
protected:
	int min_width;
	int min_height;
	int min_width_chars;
	int min_height_lines;
	int char_width;
	int font_height;
	int font_ascent;
	int separator;

	int vu_x, vu_y;
	int vu_w, vu_h;

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
	KeyBindings *key_bindings;

	int track_idx;

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

	bool selected;

public:
	void set_selected(bool p_selected) {
		selected = p_selected;
		queue_draw();
	}

	TrackRackVolume(int p_track, Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme,
			KeyBindings *p_bindings);
	~TrackRackVolume();
};

class TrackRackEditor : public Gtk::Widget {
protected:
	int min_width;
	int min_height;
	int min_width_chars;
	int min_height_lines;
	int char_width;
	int font_height;
	int font_ascent;
	int separator;

	struct Area {
		bool is_fx;
		int which;
		int index;
		int y;
		int h;
		bool insert;
	};

	Vector<Area> areas;
	int mouse_over_area;
	int pressing_area;
	int menu_at_index;
	int press_y;
	bool dragging;
	bool drag_fx;
	int dragging_y;

	int v_offset;

	// Overrides:
	Gtk::SizeRequestMode
	get_request_mode_vfunc() const override;
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
	bool on_leave_notify_event(GdkEventCrossing *crossing_event);
	bool on_motion_notify_event(GdkEventMotion *motion_event);
	bool on_key_press_event(GdkEventKey *key_event);
	bool on_key_release_event(GdkEventKey *key_event);

	Glib::RefPtr<Gdk::Window> m_refGdkWindow;
	UndoRedo *undo_redo;

	Theme *theme;
	KeyBindings *key_bindings;

	int track_idx;

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

	bool selected;

	Gtk::VScrollbar *v_scroll;
	Track *track;

	void _update_menu(bool p_muted, bool p_is_fx);

	Gtk::Menu *menu;
	Gtk::CheckMenuItem menu_item_mute;
	Gtk::SeparatorMenuItem menu_item_separator;
	Gtk::MenuItem menu_item_remove;

	Gtk::Menu *send_menu;
	Vector<Gtk::MenuItem *> available_tracks;

	void _insert_send_to_track(int p_idx);
	void _item_toggle_mute();
	void _item_removed();
	void _send_amount_changed();

	Gtk::Popover send_popover;
	Gtk::HScale send_amount;
	int send_popover_index;

public:
	sigc::signal1<void, int> add_effect;
	sigc::signal2<void, int, int> toggle_effect_skip;
	sigc::signal2<void, int, int> toggle_send_mute;
	sigc::signal2<void, int, int> remove_effect;
	sigc::signal2<void, int, int> remove_send;
	sigc::signal2<void, int, int> insert_send_to_track;
	sigc::signal3<void, int, int, float> send_amount_changed;
	sigc::signal3<void, int, int, int> track_swap_effects;
	sigc::signal3<void, int, int, int> track_swap_sends;
	sigc::signal2<void, int, int> effect_request_editor;

	void set_selected(bool p_selected) {
		selected = p_selected;
		queue_draw();
	}

	int set_v_offset(int p_offset);
	int get_v_offset() const;
	Track *get_track() const;

	TrackRackEditor(int p_track, Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme,
			KeyBindings *p_bindings, Gtk::VScrollbar *p_v_scroll);
	~TrackRackEditor();
};

class TrackRackFiller : public Gtk::Widget {

	Theme *theme;
	Glib::RefPtr<Gdk::Window> m_refGdkWindow;

public:
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	TrackRackFiller(Theme *p_theme);
};

#endif // TRACK_EDITOR_H
