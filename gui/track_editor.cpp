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
		cr->select_font_face(theme->fonts[Theme::FONT_TRACK_EDIT].face.utf8().get_data(),
				Cairo::FONT_SLANT_NORMAL,
				theme->fonts[Theme::FONT_TRACK_EDIT].bold ? Cairo::FONT_WEIGHT_BOLD : Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(theme->fonts[Theme::FONT_TRACK_EDIT].size);
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

	if (selected) {
		_draw_rect(cr, 1, 0, w + 1, h, theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR]);
	}
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

	Gdk::Rectangle posr(event->x, event->y, 1, 1);
}

bool TrackRackEditor::on_button_press_event(GdkEventButton *event) {
	grab_focus();
	_mouse_button_event(event, true);
	return false;
}

bool TrackRackEditor::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool TrackRackEditor::on_motion_notify_event(GdkEventMotion *motion_event) {

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
		cr->select_font_face(theme->fonts[Theme::FONT_TRACK_EDIT].face.utf8().get_data(),
				Cairo::FONT_SLANT_NORMAL,
				theme->fonts[Theme::FONT_TRACK_EDIT].bold ? Cairo::FONT_WEIGHT_BOLD : Cairo::FONT_WEIGHT_NORMAL);
		cr->set_font_size(theme->fonts[Theme::FONT_TRACK_EDIT].size);
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

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG]);

	cr->rectangle(0, 0, w, h);
	cr->fill();

	String text = "Effects:";
	int title_offset = (w - _get_text_width(cr, text)) / 2;

	_draw_fill_rect(cr, 0, 0, w, font_height + separator, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR]);
	_draw_text(cr, title_offset, separator + font_ascent, text, selected ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE], false);
	int visible_slots = (h - separator) / (font_height + separator) - 2;
	if (visible_slots > 0) {
		bool can_insert = true;
		int ofs = 0;
		for (int i = 0; i < visible_slots; i++) {
			int y = (1 + i) * (font_height + separator);
			_draw_rect(cr, 0, y, w, font_height + separator, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
			int effect_idx = ofs + i;
			if (effect_idx < song->get_track(track_idx)->get_audio_effect_count()) {
				String title = song->get_track(track_idx)->get_audio_effect(effect_idx)->get_info()->caption;

				_draw_text(cr, char_width, y + separator / 2 + font_ascent, title, theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE], false);
			} else if (can_insert) {
				String insert_text = "<Insert>";
				int insert_ofs = (w - _get_text_width(cr, insert_text)) / 2;
				_draw_text(cr, insert_ofs, y + separator / 2 + font_ascent, insert_text, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT], false);
				can_insert = false;
			}
		}
	}

	_draw_fill_rect(cr, 0, h - font_height - separator * 2, w, font_height + separator * 2, theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR]);
	String out_text = "Output";
	int out_offset = (w - _get_text_width(cr, out_text)) / 2;

	_draw_text(cr, out_offset, h - font_height - separator + font_ascent, out_text, theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE], false);
	_draw_arrow(cr, w - font_height, h - font_height - separator, font_height, font_height, theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE]);

	if (selected) {
		_draw_rect(cr, -1, 0, w + 1, h, theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR]);
	}
}

void TrackRackEditor::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}
TrackRackEditor::TrackRackEditor(int p_track, Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme,
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

	set_name("pattern_editor");

	min_width = 1;
	min_height = 1;
	min_width_chars = 8;
	min_height_lines = 10;
	char_width = 1;
	font_height = 1;
	font_ascent = 1;
	separator = 1;

	selected = false;
}

TrackRackEditor::~TrackRackEditor() {
}
