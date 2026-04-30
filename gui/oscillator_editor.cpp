#include "oscillator_editor.h"


// Discover the total amount of minimum space and natural space needed by
// this widget.
// Let's make this simple example widget always need minimum 60 by 50 and
// natural 100 by 70.


void OscillatorEditor::on_size_allocate(Gtk::Allocation &allocation) {
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

void OscillatorEditor::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void OscillatorEditor::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void OscillatorEditor::on_realize() {
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

void OscillatorEditor::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void OscillatorEditor::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void OscillatorEditor::_draw_line(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int tx, int ty, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);
	cr->set_line_width(2.0);
	cr->move_to(x,y);
	cr->line_to(tx,ty);
	cr->stroke();
}

bool OscillatorEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	_draw_fill_rect(cr,0,0,w,h,theme->colors[Theme::COLOR_BACKGROUND]);

	float prev_x;
	float prev_y;
	for(int i=0;i<w;i++) {
		float x = float(i) / float(w);
		float y = 0;
		switch(osc_type) {
			case OSC_TYPE_SINE: y = oscillator_eval<OSC_TYPE_SINE>(x,shape); break;
			case OSC_TYPE_TRIANGLE: y = oscillator_eval<OSC_TYPE_TRIANGLE>(x,shape); break;
			case OSC_TYPE_SQUARE: y = oscillator_eval<OSC_TYPE_SQUARE>(x,shape); break;
			case OSC_TYPE_SAW: y = oscillator_eval<OSC_TYPE_SAW>(x,shape); break;
			default : {}
		}

		y = y * 0.5 + 0.5;
		y = y * 0.8 + 0.1;

		if (i==0) {
			prev_x = x;
			prev_y = y;
		}
		_draw_line(cr,x*w,y*h,prev_x*w,prev_y*h,theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE]);

		prev_x = x;
		prev_y = y;
	}
	return false;
}


void OscillatorEditor::set_osc_type(OscillatorType p_type) {
	osc_type = p_type;
	queue_draw();
}

void OscillatorEditor::set_shape(float p_shape) {
	shape = p_shape;
	queue_draw();
}

OscillatorEditor::OscillatorEditor(Theme *p_theme) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("osc_editor"),
		Gtk::Widget() {

	theme = p_theme;

	set_has_window(true);
	set_name("osc_editor");

}

OscillatorEditor::~OscillatorEditor() {
}
