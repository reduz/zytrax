#include "master_vu.h"

void MasterVU::_mouse_button_event(GdkEventButton *event, bool p_press) {

	if (event->button == 1) {
		if (p_press && event->x >= grabber_x && event->x < grabber_x + grabber_w && event->y >= grabber_y && event->y < grabber_y + grabber_h) {
			grabbing_x = event->x;
			grabbing_db = song->get_main_volume_db();
			grabbing = true;
			queue_draw();
		}

		if (!p_press) {
			grabbing = false;
			queue_draw();
		}
	}
}

bool MasterVU::on_button_press_event(GdkEventButton *event) {
	grab_focus();
	_mouse_button_event(event, true);
	return false;
}

bool MasterVU::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool MasterVU::on_motion_notify_event(GdkEventMotion *motion_event) {

	if (grabbing) {
		float new_db = grabbing_db + (motion_event->x - grabbing_x) * (TRACK_MAX_DB - TRACK_MIN_DB) / vu_w;
		new_db = CLAMP(new_db, TRACK_MIN_DB, TRACK_MAX_DB);
		main_volume_db_changed.emit(new_db);
		queue_draw();
	}
	return false;
}

bool MasterVU::on_key_press_event(GdkEventKey *key_event) {

	return true;
}

bool MasterVU::on_key_release_event(GdkEventKey *key_event) {

	return false;
}

Gtk::SizeRequestMode MasterVU::get_request_mode_vfunc() const {
	// Accept the default value supplied by the base class.
	return Gtk::Widget::get_request_mode_vfunc();
}

// Discover the total amount of minimum space and natural space needed by
// this widget.
// Let's make this simple example widget always need minimum 60 by 50 and
// natural 100 by 70.

void MasterVU::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {

	minimum_width = min_width;
	natural_width = min_width;
}

void MasterVU::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void MasterVU::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = min_height;
	natural_height = min_height;
}

void MasterVU::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = min_width;
	natural_width = min_width;
}

void MasterVU::on_size_allocate(Gtk::Allocation &allocation) {
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

void MasterVU::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void MasterVU::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void MasterVU::on_realize() {
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

void MasterVU::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void MasterVU::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
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

int MasterVU::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}
void MasterVU::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void MasterVU::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

void MasterVU::_draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x + w / 4, y + h / 4);
	cr->line_to(x + w * 3 / 4, y + h / 4);
	cr->line_to(x + w / 2, y + h * 3 / 4);
	cr->line_to(x + w / 4, y + h / 4);
	cr->fill();
	cr->stroke();
}

bool MasterVU::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();
#if 0
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

		int new_width = fw;
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

#endif

	int grabber_width = h / 2;

	Gdk::Cairo::set_source_rgba(cr, Theme::make_rgba(0, 0, 0));
	cr->rectangle(0, 0, w, h);
	cr->fill();
	vu_x = grabber_width / 2;
	vu_y = 0;
	vu_w = w - grabber_width;
	vu_h = h;

	Gdk::RGBA rgba;
	rgba.set_alpha(1);

	cr->set_line_width(1);

	for (int i = 0; i < vu_w; i += 2) {
		float db = TRACK_MAX_DB - float(i) * (TRACK_MAX_DB - TRACK_MIN_DB) / vu_w;
		float r = 0, g = 0, b = 0;

		if (db > 0) {
			r = 1.0;
			g = 1.0 - db / TRACK_MAX_DB;
		} else {
			r = 1.0 - db / TRACK_MIN_DB;
			g = 1.0;
		}

		float lr = r;
		float lg = g;
		float lb = b;
		float rr = r;
		float rg = g;
		float rb = b;
		{

			if (db > peak_db_l) {
				lr *= 0.3;
				lg *= 0.3;
				lb *= 0.3;
			}

			if (db > peak_db_r) {
				rr *= 0.3;
				rg *= 0.3;
				rb *= 0.3;
			}
			int middle = vu_y + vu_h / 2;
			{
				rgba.set_red(lr);
				rgba.set_green(lg);
				rgba.set_blue(lb);
				Gdk::Cairo::set_source_rgba(cr, rgba);
				cr->move_to(vu_x + vu_w - i + 0.5, vu_y);
				cr->line_to(vu_x + vu_w - i + 0.5, middle - 1);

				cr->stroke();
			}
			{
				rgba.set_red(rr);
				rgba.set_green(rg);
				rgba.set_blue(rb);
				Gdk::Cairo::set_source_rgba(cr, rgba);
				cr->move_to(vu_x + vu_w - i + 0.5, middle + 1);
				cr->line_to(vu_x + vu_w - i + 0.5, vu_y + vu_h);
				cr->stroke();
			}
		}
	}

	//white line at 0db
	rgba.set_red(1);
	rgba.set_green(1);
	rgba.set_blue(1);
	rgba.set_alpha(0.5);
	Gdk::Cairo::set_source_rgba(cr, rgba);

	int db0 = TRACK_MAX_DB * float(vu_w) / float(TRACK_MAX_DB - TRACK_MIN_DB);
	cr->move_to(vu_x + vu_w - db0 + 0.5, vu_y);
	cr->line_to(vu_x + vu_w - db0 + 0.5, vu_y + vu_h);
	cr->move_to(vu_x + vu_w - db0 + 1.5, vu_y);
	cr->line_to(vu_x + vu_w - db0 + 1.5, vu_y + vu_h);
	cr->stroke();

	//draw handle

	float track_db = song->get_main_volume_db();
	int db_handle = (TRACK_MAX_DB - track_db) * vu_w / float(TRACK_MAX_DB - TRACK_MIN_DB);

	rgba.set_red(1);
	rgba.set_green(0.5);
	rgba.set_blue(0.5);
	rgba.set_alpha(1.0);
	Gdk::Cairo::set_source_rgba(cr, rgba);

	cr->move_to(vu_x + vu_w - db_handle - 0.5, vu_y);
	cr->line_to(vu_x + vu_w - db_handle - 0.5, vu_y + vu_h);
	cr->stroke();

	rgba.set_red(1);
	rgba.set_green(1);
	rgba.set_blue(1);
	rgba.set_alpha(1.0);

	grabber_x = vu_x + vu_w - db_handle - grabber_width / 2;
	grabber_w = grabber_width;
	grabber_y = 0;
	grabber_h = vu_h;

	cr->set_line_width(2);
	_draw_rect(cr, grabber_x + 0.5, grabber_y + 0.5, grabber_w, grabber_h - 1, rgba);

	return false;
}

void MasterVU::update_peak() {
	uint64_t current_time = g_get_monotonic_time();
	double diff = double(current_time - last_time) / 1000000.0;
	last_time = current_time;

	{

		float current_peak_l = song->get_peak_volume_db_l();

		float new_peak_l;
		if (current_peak_l > peak_db_l) {
			new_peak_l = current_peak_l;
		} else {
			//decrement
			new_peak_l = peak_db_l - 48 * diff; //24db per second?
		}

		if (new_peak_l < TRACK_MIN_DB) { //so it stops redrawing eventually;
			new_peak_l = TRACK_MIN_DB;
		}

		if (new_peak_l != peak_db_l) {
			peak_db_l = new_peak_l;
			queue_draw();
		}
	}

	{

		float current_peak_r = song->get_peak_volume_db_r();

		float new_peak_r;
		if (current_peak_r > peak_db_r) {
			new_peak_r = current_peak_r;
		} else {
			//decrement
			new_peak_r = peak_db_r - 48 * diff; //24db per second?
		}

		if (new_peak_r < TRACK_MIN_DB) { //so it stops redrawing eventually;
			new_peak_r = TRACK_MIN_DB;
		}

		if (new_peak_r != peak_db_r) {
			peak_db_r = new_peak_r;
			queue_draw();
		}
	}
}

void MasterVU::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}
MasterVU::MasterVU(Song *p_song, UndoRedo *p_undo_redo, Theme *p_theme) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("main_vu"),
		Gtk::Widget() {

	// This shows the GType name, which must be used in the CSS file.
	// std::cout << "GType name: " << G_OBJECT_TYPE_NAME(gobj()) << std::endl;

	// This shows that the GType still derives from GtkWidget:
	// std::cout << "Gtype is a GtkWidget?:" << GTK_IS_WIDGET(gobj()) <<
	// std::endl;

	song = p_song;
	undo_redo = p_undo_redo;
	theme = p_theme;
	set_has_window(true);
	// Gives Exposure & Button presses to the widget.

	set_name("main_vu");

	min_width = 1;
	min_height = 1;
	char_width = 1;
	font_height = 1;
	font_ascent = 1;
	grabbing = false;

	grabber_x = grabber_y = grabber_w = grabber_h = -1;
	last_time = 0;
	peak_db_l = -100;
	peak_db_r = -100;
}

MasterVU::~MasterVU() {
}
