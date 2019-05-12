#ifndef ORDERLIST_EDITOR_H
#define ORDERLIST_EDITOR_H
#include "engine/song.h"
#include "engine/undo_redo.h"
#include <gtkmm/cssprovider.h>
#include <gtkmm/styleproperty.h>
#include <gtkmm/widget.h>

#include "gui/color_theme.h"
#include "gui/key_bindings.h"

class OrderlistEditor : public Gtk::Widget {
protected:
	//Overrides:

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

	struct Cursor {
		int row;
		int field;
	} cursor;

	int v_offset;

	int fw_cache;
	int fh_cache;

	bool drawing;

	int top_ofs;

	int visible_rows;

	void _adjust_cursor_to_view();

	void _validate_cursor();
	void _update_oderlist();

	void _draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, const String &p_text, const Gdk::RGBA &p_color, bool p_down = false);
	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);
	void _draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);
	void _draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int w, int h, const Gdk::RGBA &p_color);

	Theme *theme;
	KeyBindings *key_bindings;

	Glib::RefPtr<Gtk::Adjustment> v_scroll;

	void _v_scroll_changed();

public:
	void set_vscroll(Glib::RefPtr<Gtk::Adjustment> p_v_scroll);

	OrderlistEditor(Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme, KeyBindings *p_bindings);
	~OrderlistEditor();
};
#endif // ORDERLIST_EDITOR_H
