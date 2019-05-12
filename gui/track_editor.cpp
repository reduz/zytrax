#include "track_editor.h"

void TrackRackVolume::_mouse_button_event(GdkEventButton *event, bool p_press) {

	Gdk::Rectangle posr(event->x, event->y, 1, 1);
}

bool TrackRackVolume::on_button_press_event(GdkEventButton *event) {
	grab_focus();
	_mouse_button_event(event, true);
	return false;
}

bool TrackRackVolume::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool TrackRackVolume::on_motion_notify_event(GdkEventMotion *motion_event) {

	return false;
}

bool TrackRackVolume::on_key_press_event(GdkEventKey *key_event) {

	return true;
}

bool TrackRackVolume::on_key_release_event(GdkEventKey *key_event) {

	return false;
}

Gtk::SizeRequestMode TrackRackVolume::get_request_mode_vfunc() const {
	// Accept the default value supplied by the base class.
	return Gtk::Widget::get_request_mode_vfunc();
}

// Discover the total amount of minimum space and natural space needed by
// this widget.
// Let's make this simple example widget always need minimum 60 by 50 and
// natural 100 by 70.

void TrackRackVolume::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {

	minimum_width = min_width;
	natural_width = min_width;
}

void TrackRackVolume::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void TrackRackVolume::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void TrackRackVolume::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = min_width;
	natural_width = min_width;
}

void TrackRackVolume::on_size_allocate(Gtk::Allocation &allocation) {
	// Do something with the space that we have actually been given:
	//(We will not be given heights or widths less than we have requested, though
	// we might get more)

	// Use the offered allocation for this container:
	set_allocation(allocation);

	if (m_refGdkWindow) {
		m_refGdkWindow->move_resize(allocation.get_x(), allocation.get_y(),
				allocation.get_width(),
				allocation.get_height());
	}
}

void TrackRackVolume::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void TrackRackVolume::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void TrackRackVolume::on_realize() {
	// Do not call base class Gtk::Widget::on_realize().
	// It's intended only for widgets that set_has_window(false).

	set_realized();

	if (!m_refGdkWindow) {
		// Create the GdkWindow:

		GdkWindowAttr attributes;
		memset(&attributes, 0, sizeof(attributes));

		Gtk::Allocation allocation = get_allocation();

		// Set initial position and size of the Gdk::Window:
		attributes.x = allocation.get_x();
		attributes.y = allocation.get_y();
		attributes.width = allocation.get_width();
		attributes.height = allocation.get_height();

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK |
								Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
								Gdk::BUTTON1_MOTION_MASK | Gdk::KEY_PRESS_MASK |
								Gdk::KEY_RELEASE_MASK;
		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.wclass = GDK_INPUT_OUTPUT;

		m_refGdkWindow = Gdk::Window::create(get_parent_window(), &attributes,
				GDK_WA_X | GDK_WA_Y);
		set_window(m_refGdkWindow);

		// make the widget receive expose events
		m_refGdkWindow->set_user_data(gobj());
	}
}

void TrackRackVolume::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void TrackRackVolume::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, const String &p_text,
		const Gdk::RGBA &p_color, bool p_down) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x, y);
	if (p_down)
		cr->rotate_degrees(90);
	cr->show_text(p_text.utf8().get_data());
	if (p_down)
		cr->rotate_degrees(-90);
	cr->move_to(0, 0);
	cr->stroke();
}

int TrackRackVolume::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}
void TrackRackVolume::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void TrackRackVolume::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

void TrackRackVolume::_draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x + w / 4, y + h / 4);
	cr->line_to(x + w * 3 / 4, y + h / 4);
	cr->line_to(x + w / 2, y + h * 3 / 4);
	cr->line_to(x + w / 4, y + h / 4);
	cr->fill();
	cr->stroke();
}

bool TrackRackVolume::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	{
		//update min width
		theme->select_font_face(cr);

		Cairo::FontExtents fe;
		cr->get_font_extents(fe);

		Cairo::TextExtents te;
		cr->get_text_extents("XXX", te);
		int fw = te.width;
		cr->get_text_extents("XX", te);
		fw -= te.width;

		int new_width = fw * min_width_chars + font_height + separator * 2;
		int new_height = fe.height;

		if (new_width != min_width || new_height != min_height) {
			min_width = new_width;
			min_height = new_height;

			char_width = fw;
			font_height = fe.height;
			font_ascent = fe.ascent;
			queue_resize();
			/*Gtk::Widget *w = this;
			while (w) {
				w->queue_resize();
				w = w->get_parent();
			}*/
		}
	}

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG]);
	cr->rectangle(0, 0, font_height + separator, h);
	cr->fill();

	_draw_text(cr, separator, separator, song->get_track(track_idx)->get_name(), selected ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE], true);

	float track_max_db = 12;
	float track_min_db = -60;

	vu_x = font_height + separator;
	vu_y = font_height / 2;
	vu_w = char_width * min_width_chars;
	vu_h = h - vu_y * 2;

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_BACKGROUND]);

	Gdk::Cairo::set_source_rgba(cr, Theme::make_rgba(0, 0, 0));
	cr->rectangle(font_height + separator, 0, w - (font_height + separator), h);
	cr->fill();

	Gdk::RGBA rgba;
	rgba.set_alpha(1);

	cr->set_line_width(1);

	float peak_db = song->get_track(track_idx)->get_peak_volume_db();

	for (int i = 0; i < vu_h; i += 2) {
		float db = track_max_db - i * (track_max_db - track_min_db) / vu_h;
		float r = 0, g = 0, b = 0;
		if (db > 0) {
			r = 1.0;
			g = 1.0 - db / track_max_db;
		} else {
			r = 1.0 - db / track_min_db;
			g = 1.0;
		}

		if (db > peak_db) {
			r *= 0.3;
			g *= 0.3;
			b *= 0.3;
		}
		rgba.set_red(r);
		rgba.set_green(g);
		rgba.set_blue(b);
		Gdk::Cairo::set_source_rgba(cr, rgba);
		cr->move_to(vu_x, vu_y + i + 0.5);
		cr->line_to(vu_x + vu_w, vu_y + i + 0.5);
		cr->stroke();
	}

	//white line at 0db
	rgba.set_red(1);
	rgba.set_green(1);
	rgba.set_blue(1);
	Gdk::Cairo::set_source_rgba(cr, rgba);

	int db0 = track_max_db * vu_h / (track_max_db - track_min_db);
	cr->move_to(vu_x, vu_y + db0 + 0.5);
	cr->line_to(vu_x + vu_w, vu_y + db0 + 0.5);
	cr->move_to(vu_x, vu_y + db0 + 1.5);
	cr->line_to(vu_x + vu_w, vu_y + db0 + 1.5);
	cr->stroke();

	//draw handle

	float track_db = song->get_track(track_idx)->get_mix_volume_db();
	int db_handle = (track_max_db - track_db) * vu_h / (track_max_db - track_min_db);

	_draw_rect(cr, vu_x, db_handle, vu_w - 1, font_height, rgba);
	rgba.set_alpha(0.7);

	cr->move_to(vu_x, vu_y + db_handle + 0.5);
	cr->line_to(vu_x + vu_w, vu_y + db_handle + 0.5);
	//cr->move_to(vu_x, vu_y + db_handle + 1.5);
	//cr->line_to(vu_x + vu_w, vu_y + db_handle + 1.5);
	cr->stroke();

	/*if (selected) {
		_draw_rect(cr, 1, 0, w + 1, h, theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR]);
	}*/
}

void TrackRackVolume::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}
TrackRackVolume::TrackRackVolume(int p_track, Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme,
		KeyBindings *p_bindings) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("track_editor"),
		Gtk::Widget() {

	// This shows the GType name, which must be used in the CSS file.
	// std::cout << "GType name: " << G_OBJECT_TYPE_NAME(gobj()) << std::endl;

	// This shows that the GType still derives from GtkWidget:
	// std::cout << "Gtype is a GtkWidget?:" << GTK_IS_WIDGET(gobj()) <<
	// std::endl;

	track_idx = p_track;
	song = p_song;
	undo_redo = p_undo_redo;
	key_bindings = p_bindings;
	theme = p_theme;
	set_has_window(true);
	set_can_focus(true);
	set_focus_on_click(true);
	// Gives Exposure & Button presses to the widget.

	set_name("rack_volume");

	min_width = 1;
	min_height = 1;
	min_width_chars = 3;
	min_height_lines = 10;
	char_width = 1;
	font_height = 1;
	font_ascent = 1;
	separator = 2;
	selected = false;
}

TrackRackVolume::~TrackRackVolume() {
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void TrackRackEditor::_mouse_button_event(GdkEventButton *event, bool p_press) {

	if (event->button == 1) {

		int new_mouse_over_area = -1;

		for (int i = 0; i < areas.size(); i++) {
			if (event->y >= areas[i].y && event->y < areas[i].y + areas[i].h) {
				new_mouse_over_area = i;
				break;
			}
		}

		if (p_press) {

			if (new_mouse_over_area == -1) {
				return; //nothing to do here
			}
			if (areas[new_mouse_over_area].insert) {
				//open dialog and insert
				if (areas[new_mouse_over_area].is_fx) {
					add_effect.emit(track_idx);
				} else {
					if (!send_menu) {
						send_menu = new Gtk::Menu;
					}
					for (int i = 0; i < available_tracks.size(); i++) {
						delete available_tracks[i];
					}
					available_tracks.clear();
					for (int i = 0; i < song->get_track_count(); i++) {
						String track_name = song->get_track(i)->get_name();
						Gtk::MenuItem *item = new Gtk::MenuItem;
						item->show();
						item->set_label(track_name.utf8().get_data());
						item->set_sensitive(track_idx != i);
						item->signal_activate().connect(sigc::bind<int>(sigc::mem_fun(*this, &TrackRackEditor::_insert_send_to_track), i));
						available_tracks.push_back(item);
						send_menu->append(*item);
					}

					Gdk::Rectangle alloc;
					alloc.set_x(0);
					alloc.set_width(get_allocated_width());
					alloc.set_y(areas[new_mouse_over_area].y);
					alloc.set_height(areas[new_mouse_over_area].h);
					send_menu->popup_at_rect(get_window(), alloc, Gdk::GRAVITY_NORTH_WEST, Gdk::GRAVITY_SOUTH_WEST, (GdkEvent *)event);
				}
			} else {
				pressing_area = new_mouse_over_area;
				press_y = event->y;
				queue_draw();
			}
		} else {

			if (new_mouse_over_area < 0) {
				//none?
			} else if (pressing_area == new_mouse_over_area) {

				if (areas[pressing_area].is_fx) {

					effect_request_editor.emit(track_idx, areas[pressing_area].which);
				} else {

					Gdk::Rectangle alloc;
					alloc.set_x(0);
					alloc.set_width(get_allocated_width());
					alloc.set_y(areas[pressing_area].y);
					alloc.set_height(areas[pressing_area].h);
					send_amount.set_size_request(alloc.get_width(), 1);
					send_amount.set_value(song->get_track(track_idx)->get_send_amount(areas[pressing_area].which) * 100);
					send_popover_index = areas[pressing_area].which;
					send_popover.set_pointing_to(alloc);
					send_popover.popup();
				}
				printf("click on %i\n", pressing_area);
				//press
			}

			if (dragging && pressing_area != -1 && mouse_over_area != -1) {
				if (drag_fx == areas[mouse_over_area].is_fx) {
					int from = areas[pressing_area].which;
					int to = areas[mouse_over_area].which;
					if (from != to) {
						if (drag_fx) {
							track_swap_effects.emit(track_idx, from, to);
						} else {
							track_swap_sends.emit(track_idx, from, to);
						}
					}
				}
			}
			pressing_area = -1;
			dragging = false;
			drag_fx = false;
			dragging_y = -1;
			queue_draw();
		}
	} else if (event->button == 3 && p_press) {

		int at_idx = -1;

		for (int i = 0; i < areas.size(); i++) {
			if (event->y >= areas[i].y && event->y < areas[i].y + areas[i].h) {
				at_idx = i;
				break;
			}
		}

		if (at_idx < 0) {
			return;
		}
		if (areas[at_idx].insert) {
			return;
		}

		menu_at_index = at_idx;

		if (areas[at_idx].is_fx) {
			bool skipped = song->get_track(track_idx)->get_audio_effect(areas[at_idx].which)->is_skipped();
			_update_menu(skipped, true);
		} else {

			bool muted = song->get_track(track_idx)->is_send_muted(areas[at_idx].which);
			_update_menu(muted, false);
		}

		menu->popup_at_pointer((GdkEvent *)event);
	}
}

bool TrackRackEditor::on_button_press_event(GdkEventButton *event) {

	_mouse_button_event(event, true);
	return false;
}

bool TrackRackEditor::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool TrackRackEditor::on_leave_notify_event(GdkEventCrossing *crossing_event) {
	if (mouse_over_area != -1) {
		mouse_over_area = -1;
		queue_draw();
	}
	return false;
}
bool TrackRackEditor::on_motion_notify_event(GdkEventMotion *motion_event) {

	int new_mouse_over_area = -1;

	for (int i = 0; i < areas.size(); i++) {
		if (motion_event->y >= areas[i].y && motion_event->y < areas[i].y + areas[i].h) {
			new_mouse_over_area = i;
			break;
		}
	}

	if (mouse_over_area != new_mouse_over_area) {
		mouse_over_area = new_mouse_over_area;
		queue_draw();
	}

	if (pressing_area >= 0 && !dragging) {
		dragging = true;
		queue_draw();
	}

	if (dragging) {
		dragging_y = motion_event->y;
		drag_fx = areas[pressing_area].is_fx;
		queue_draw();
	}

	gdk_event_request_motions(motion_event);
	return false;
}

bool TrackRackEditor::on_key_press_event(GdkEventKey *key_event) {

	return true;
}

bool TrackRackEditor::on_key_release_event(GdkEventKey *key_event) {

	return false;
}

Gtk::SizeRequestMode TrackRackEditor::get_request_mode_vfunc() const {
	// Accept the default value supplied by the base class.
	return Gtk::Widget::get_request_mode_vfunc();
}

// Discover the total amount of minimum space and natural space needed by
// this widget.
// Let's make this simple example widget always need minimum 60 by 50 and
// natural 100 by 70.

void TrackRackEditor::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {

	minimum_width = min_width;
	natural_width = min_width;
}

void TrackRackEditor::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void TrackRackEditor::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void TrackRackEditor::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = min_width;
	natural_width = min_width;
}

void TrackRackEditor::on_size_allocate(Gtk::Allocation &allocation) {
	// Do something with the space that we have actually been given:
	//(We will not be given heights or widths less than we have requested, though
	// we might get more)

	// Use the offered allocation for this container:
	set_allocation(allocation);

	if (m_refGdkWindow) {
		m_refGdkWindow->move_resize(allocation.get_x(), allocation.get_y(),
				allocation.get_width(),
				allocation.get_height());
	}
}

void TrackRackEditor::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void TrackRackEditor::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void TrackRackEditor::on_realize() {
	// Do not call base class Gtk::Widget::on_realize().
	// It's intended only for widgets that set_has_window(false).

	set_realized();

	if (!m_refGdkWindow) {
		// Create the GdkWindow:

		GdkWindowAttr attributes;
		memset(&attributes, 0, sizeof(attributes));

		Gtk::Allocation allocation = get_allocation();

		// Set initial position and size of the Gdk::Window:
		attributes.x = allocation.get_x();
		attributes.y = allocation.get_y();
		attributes.width = allocation.get_width();
		attributes.height = allocation.get_height();

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
								Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
								Gdk::BUTTON1_MOTION_MASK | Gdk::KEY_PRESS_MASK |
								Gdk::KEY_RELEASE_MASK;
		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.wclass = GDK_INPUT_OUTPUT;

		m_refGdkWindow = Gdk::Window::create(get_parent_window(), &attributes,
				GDK_WA_X | GDK_WA_Y);
		set_window(m_refGdkWindow);

		// make the widget receive expose events
		m_refGdkWindow->set_user_data(gobj());
	}
}

void TrackRackEditor::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void TrackRackEditor::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, const String &p_text,
		const Gdk::RGBA &p_color, bool p_down) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x, y);
	if (p_down)
		cr->rotate_degrees(90);
	cr->show_text(p_text.utf8().get_data());
	if (p_down)
		cr->rotate_degrees(-90);
	cr->move_to(0, 0);
	cr->stroke();
}

int TrackRackEditor::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}
void TrackRackEditor::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void TrackRackEditor::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

void TrackRackEditor::_draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x + w / 4, y + h / 4);
	cr->line_to(x + w * 3 / 4, y + h / 4);
	cr->line_to(x + w / 2, y + h * 3 / 4);
	cr->line_to(x + w / 4, y + h / 4);
	cr->fill();
	cr->stroke();
}

bool TrackRackEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	{
		//update min width
		theme->select_font_face(cr);

		Cairo::FontExtents fe;
		cr->get_font_extents(fe);

		int new_width = fe.max_x_advance * min_width_chars;
		int new_height = (fe.height + 2) * min_height_lines;

		if (new_width != min_width || new_height != min_height) {
			min_width = new_width;
			min_height = new_height;
			char_width = fe.max_x_advance;
			font_height = fe.height;
			font_ascent = fe.ascent;
			queue_resize();
			/*Gtk::Widget *w = this;
			while (w) {
				w->queue_resize();
				w = w->get_parent();
			}*/
		}
	}

	Gdk::Cairo::set_source_rgba(cr, selected ? theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_RACK_SELECTED] : theme->colors[Theme::COLOR_PATTERN_EDITOR_BG]);

	cr->rectangle(0, 0, w, h);
	cr->fill();

	int idx = 0;
	int y = 0;
	int row_height = (font_height + separator);

	int max_effects = song->get_track(track_idx)->get_audio_effect_count();
	int max_sends = song->get_track(track_idx)->get_send_count();

	areas.clear();

	for (int i = 0; i <= max_effects; i++) {

		if (idx >= v_offset) {

			Area area;
			area.index = idx;
			area.which = i;
			area.h = row_height;
			area.y = y;
			area.is_fx = true;

			String text;
			Gdk::RGBA color = (pressing_area == -1 && idx == mouse_over_area) ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];

			if (i == max_effects) {
				text = "<Insert FX>";
				color.set_alpha(color.get_alpha() * 0.7);
				area.insert = true;
			} else {

				if (dragging && drag_fx && mouse_over_area == idx) {
					_draw_rect(cr, 0, y, w, row_height - 2, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_SELECTED]);
				}
				text = song->get_track(track_idx)->get_audio_effect(i)->get_name();
				_draw_rect(cr, 0, y + row_height - 1, w, 0, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
				if (song->get_track(track_idx)->get_audio_effect(i)->is_skipped()) {
					color = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
					color.set_alpha(color.get_alpha() * 0.7);
				}

				area.insert = false;
			}

			int insert_ofs = (w - _get_text_width(cr, text)) / 2;

			int draw_y = y + separator / 2 + font_ascent;
			if (dragging && pressing_area == idx) {
				draw_y += dragging_y - press_y;
			}
			_draw_text(cr, insert_ofs, draw_y, text, color, false);

			y += row_height;

			areas.push_back(area);
		}

		idx++;
	}

	//if enough room, just put sends at the end
	if ((max_sends + max_effects + 2) * row_height < h) {
		int from = h - (max_sends + 1) * row_height;
		while (y + row_height < from) {
			_draw_rect(cr, 0, y - 1, w, 0, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
			y += row_height;
		}
		y = from;
	}

	_draw_rect(cr, 0, y - 1, w, 1, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR]);

	for (int i = 0; i <= max_sends; i++) {

		if (idx >= v_offset) {

			String text;
			Area area;
			area.index = idx;
			area.which = i;
			area.h = row_height;
			area.y = y;
			area.is_fx = false;

			Gdk::RGBA color = (pressing_area == -1 && idx == mouse_over_area) ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];

			if (i == max_sends) {
				text = "<Insert Send>";
				color.set_alpha(color.get_alpha() * 0.7);
				area.insert = true;
			} else {

				if (dragging && !drag_fx && mouse_over_area == idx) {
					_draw_rect(cr, 0, y, w, row_height - 2, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_SELECTED]);
				}

				bool muted = song->get_track(track_idx)->is_send_muted(i);
				int to = song->get_track(track_idx)->get_send_track(i);
				if (to < 0 || to >= song->get_track_count()) {
					text = "Speakers: ";
				} else {
					text = song->get_track(to)->get_name() + ": ";
				}
				int amount_percent = int(song->get_track(track_idx)->get_send_amount(i) * 100);
				if (muted) {
					text += "Muted";
				} else {
					text += String::num(amount_percent) + "%";
				}

				_draw_rect(cr, 0, y + row_height - 1, w, 0, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
				area.insert = false;

				if (muted) {
					color = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
					color.set_alpha(color.get_alpha() * 0.7);
				}
			}

			int insert_ofs = (w - _get_text_width(cr, text)) / 2;
			int draw_y = y + separator / 2 + font_ascent;
			if (dragging && pressing_area == idx) {
				draw_y += dragging_y - press_y;
			}
			_draw_text(cr, insert_ofs, draw_y, text, color, false);
			y += row_height;

			areas.push_back(area);
		}

		idx++;
	}

	/*if (selected) {
		_draw_rect(cr, -1, 0, w + 1, h, theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR]);
	}*/
}

int TrackRackEditor::set_v_offset(int p_offset) {
	v_offset = p_offset;
}
int TrackRackEditor::get_v_offset() const {
	return v_offset;
}

Track *TrackRackEditor::get_track() const {
	return track;
}

void TrackRackEditor::_update_menu(bool p_muted, bool p_is_fx) {

	if (!menu) {
		menu = new (Gtk::Menu);
		menu->append(menu_item_mute);
		menu->append(menu_item_separator);
		menu->append(menu_item_remove);
	}

	if (p_is_fx) {
		menu_item_mute.set_label("Skip");
	} else {
		menu_item_mute.set_label("Mute");
	}

	menu_item_mute.set_active(p_muted);
}

void TrackRackEditor::_insert_send_to_track(int p_idx) {

	insert_send_to_track.emit(track_idx, p_idx);
}
void TrackRackEditor::_item_toggle_mute() {

	if (areas[menu_at_index].is_fx) {
		toggle_effect_skip.emit(track_idx, areas[menu_at_index].which);
	} else {
		toggle_send_mute.emit(track_idx, areas[menu_at_index].which);
	}
}
void TrackRackEditor::_item_removed() {

	if (areas[menu_at_index].is_fx) {
		remove_effect.emit(track_idx, areas[menu_at_index].which);
	} else {
		remove_send.emit(track_idx, areas[menu_at_index].which);
	}
}

void TrackRackEditor::_send_amount_changed() {
	printf("send amount changed?");
	send_amount_changed.emit(track_idx, send_popover_index, send_amount.get_adjustment()->get_value() / 100.0);
}

void TrackRackEditor::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}
TrackRackEditor::TrackRackEditor(int p_track, Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme,
		KeyBindings *p_bindings, Gtk::VScrollbar *p_v_scroll) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("track_editor"),
		Gtk::Widget(),
		send_popover(*this) {

	v_scroll = p_v_scroll;

	// This shows the GType name, which must be used in the CSS file.
	// std::cout << "GType name: " << G_OBJECT_TYPE_NAME(gobj()) << std::endl;

	// This shows that the GType still derives from GtkWidget:
	// std::cout << "Gtype is a GtkWidget?:" << GTK_IS_WIDGET(gobj()) <<
	// std::endl;

	track_idx = p_track;
	track = p_song->get_track(track_idx);
	song = p_song;
	undo_redo = p_undo_redo;
	key_bindings = p_bindings;
	theme = p_theme;
	set_has_window(true);
	set_can_focus(true);
	set_focus_on_click(true);
	// Gives Exposure & Button presses to the widget.

	set_name("pattern_editor");

	min_width = 1;
	min_height = 1;
	min_width_chars = 8;
	min_height_lines = 8;
	char_width = 1;
	font_height = 1;
	font_ascent = 1;
	separator = 1;
	v_offset = 0;

	pressing_area = -1;
	selected = false;
	mouse_over_area = -1;
	dragging = false;
	dragging_y = 0;
	drag_fx = false;

	menu_item_mute.set_label("Skip");
	menu_item_mute.signal_activate().connect(sigc::mem_fun(this, &TrackRackEditor::_item_toggle_mute));
	menu_item_mute.show();

	menu_item_separator.show();

	menu_item_remove.set_label("Remove");
	menu_item_remove.signal_activate().connect(sigc::mem_fun(this, &TrackRackEditor::_item_removed));
	menu_item_remove.show();

	menu = NULL;
	send_menu = NULL;
	menu_at_index = -1;

	send_popover.add(send_amount);

	send_amount.get_adjustment()->set_lower(0);
	send_amount.get_adjustment()->set_upper(100);
	send_amount.get_adjustment()->set_page_size(0);
	send_amount.get_adjustment()->set_value(20);
	send_amount.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &TrackRackEditor::_send_amount_changed));

	send_amount.show();
}

TrackRackEditor::~TrackRackEditor() {
	if (menu) {
		delete menu;
	}

	for (int i = 0; i < available_tracks.size(); i++) {
		delete available_tracks[i];
	}

	if (send_menu) {
		delete menu;
	}
}

//////////////////////////

void TrackRackFiller::on_size_allocate(Gtk::Allocation &allocation) {
	// Do something with the space that we have actually been given:
	//(We will not be given heights or widths less than we have requested, though
	// we might get more)

	// Use the offered allocation for this container:
	set_allocation(allocation);

	if (m_refGdkWindow) {
		m_refGdkWindow->move_resize(allocation.get_x(), allocation.get_y(),
				allocation.get_width(),
				allocation.get_height());
	}
}

void TrackRackFiller::on_realize() {
	// Do not call base class Gtk::Widget::on_realize().
	// It's intended only for widgets that set_has_window(false).

	set_realized();

	if (!m_refGdkWindow) {
		// Create the GdkWindow:

		GdkWindowAttr attributes;
		memset(&attributes, 0, sizeof(attributes));

		Gtk::Allocation allocation = get_allocation();

		// Set initial position and size of the Gdk::Window:
		attributes.x = allocation.get_x();
		attributes.y = allocation.get_y();
		attributes.width = allocation.get_width();
		attributes.height = allocation.get_height();

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK |
								Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
								Gdk::BUTTON1_MOTION_MASK | Gdk::KEY_PRESS_MASK |
								Gdk::KEY_RELEASE_MASK;
		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.wclass = GDK_INPUT_OUTPUT;

		m_refGdkWindow = Gdk::Window::create(get_parent_window(), &attributes,
				GDK_WA_X | GDK_WA_Y);
		set_window(m_refGdkWindow);

		// make the widget receive expose events
		m_refGdkWindow->set_user_data(gobj());
	}
}

void TrackRackFiller::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

bool TrackRackFiller::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG]);
	cr->rectangle(0, 0, w, h);
	cr->fill();
}

TrackRackFiller::TrackRackFiller(Theme *p_theme) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("filler"),
		Gtk::Widget() {

	theme = p_theme;
}
