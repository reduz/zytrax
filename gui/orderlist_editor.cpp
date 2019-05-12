#include "orderlist_editor.h"

void OrderlistEditor::_mouse_button_event(GdkEventButton *event, bool p_press) {

	if (p_press && event->button == 1) {
		cursor.row = (event->y - top_ofs) / fh_cache;
		if (event->x > fw_cache * 4) {
			cursor.field = CLAMP((event->x - fw_cache * 5) / fw_cache, 0, 2);
		}
		_validate_cursor();
		queue_draw();
	}
}

void OrderlistEditor::_adjust_cursor_to_view() {

	if (cursor.row < v_offset) {
		cursor.row = v_offset;
	} else if (cursor.row >= v_offset + visible_rows) {
		cursor.row = v_offset + visible_rows - 1;
	}
}
bool OrderlistEditor::on_scroll_event(GdkEventScroll *scroll_event) {

	if (scroll_event->direction == GDK_SCROLL_UP) {
		v_scroll->set_value(v_scroll->get_value() - 4);
		_adjust_cursor_to_view();
		return true;
	}
	if (scroll_event->direction == GDK_SCROLL_DOWN) {
		v_scroll->set_value(v_scroll->get_value() + 4);
		_adjust_cursor_to_view();
		return true;
	}

	return false;
}
bool OrderlistEditor::on_button_press_event(GdkEventButton *event) {
	grab_focus();
	_mouse_button_event(event, true);
	return false;
}

bool OrderlistEditor::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool OrderlistEditor::on_motion_notify_event(GdkEventMotion *motion_event) {
	return false;
}

void OrderlistEditor::_update_oderlist() {
	queue_draw();
}

void OrderlistEditor::_validate_cursor() {

	if (cursor.row < 0) {
		cursor.row = 0;
	}

	if (cursor.row > Song::ORDER_MAX) {
		cursor.row = Song::ORDER_MAX;
	}

	if (cursor.row < v_offset) {
		v_offset = cursor.row;
	} else if (cursor.row >= v_offset + visible_rows) {
		v_offset = cursor.row - visible_rows + 1;
	}
}

bool OrderlistEditor::on_key_press_event(GdkEventKey *key_event) {

	bool shift_pressed = key_event->state & GDK_SHIFT_MASK;

	if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_UP)) {
		cursor.row -= 1;
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_DOWN)) {

		cursor.row += 1;
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_PAGE_UP)) {

		cursor.row -= 16;
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_PAGE_DOWN)) {

		cursor.row += 16;
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_LEFT)) {

		if (cursor.field > 0) {
			cursor.field--;
		}
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_RIGHT)) {

		if (cursor.field < 2) {
			cursor.field++;
		}
		queue_draw();
	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_HOME)) {

		if (cursor.field > 0) {
			cursor.field = 0;
		} else {
			cursor.row = 0;
		}
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_END)) {

		if (cursor.field < 2) {
			cursor.field = 2;
		} else {
			cursor.row = Song::ORDER_MAX - 1;
		}
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_FIELD_CLEAR)) {

		undo_redo->begin_action("Clear Order");
		undo_redo->do_method(song, &Song::order_set, cursor.row, int(Song::ORDER_EMPTY));
		undo_redo->undo_method(song, &Song::order_set, cursor.row, song->order_get(cursor.row));
		undo_redo->do_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->undo_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->commit_action();
		cursor.row++;
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_INSERT)) {

		undo_redo->begin_action("Insert Empty");
		for (int i = cursor.row; i <= Song::ORDER_MAX; i++) {

			int existing = song->order_get(i);
			int fresh = i == cursor.row ? Song::ORDER_EMPTY : song->order_get(i - 1);
			if (existing == fresh) {
				continue;
			}

			undo_redo->do_method(song, &Song::order_set, i, fresh);
			undo_redo->undo_method(song, &Song::order_set, i, existing);
		}
		undo_redo->do_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->undo_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->commit_action();
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_DELETE)) {

		undo_redo->begin_action("Delete");
		for (int i = cursor.row; i <= Song::ORDER_MAX; i++) {

			int existing = song->order_get(i);
			int fresh = i == cursor.row == Song::ORDER_MAX ? Song::ORDER_EMPTY : song->order_get(i + 1);
			if (existing == fresh) {
				continue;
			}

			undo_redo->do_method(song, &Song::order_set, i, fresh);
			undo_redo->undo_method(song, &Song::order_set, i, existing);
		}
		undo_redo->do_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->undo_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->commit_action();
		_validate_cursor();
		queue_draw();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_CURSOR_NOTE_OFF)) {

		undo_redo->begin_action("Insert Skip");
		undo_redo->do_method(song, &Song::order_set, cursor.row, int(Song::ORDER_SKIP));
		undo_redo->undo_method(song, &Song::order_set, cursor.row, song->order_get(cursor.row));
		undo_redo->do_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->undo_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->commit_action();
		cursor.row++;
		_validate_cursor();
		queue_draw();

	} else if ((key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9)) {

		int number = key_event->keyval - GDK_KEY_0;
		int existing = song->order_get(cursor.row);
		int base;

		if (existing == Song::ORDER_EMPTY || existing == Song::ORDER_SKIP) {
			base = 0;
		} else {
			base = existing;
		}

		int num[3] = { base / 100, (base / 10) % 10, base % 10 };
		num[cursor.field] = number;

		int new_number = num[0] * 100 + num[1] * 10 + num[2];

		printf("base %i number %i existing %i new %i n1 %i n2 %i n3 %i field %i\n", base, number, existing, new_number, num[0], num[1], num[2], cursor.field);
		undo_redo->begin_action("Insert Number");
		undo_redo->do_method(song, &Song::order_set, cursor.row, new_number);
		undo_redo->undo_method(song, &Song::order_set, cursor.row, existing);
		undo_redo->do_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->undo_method(this, &OrderlistEditor::_update_oderlist);
		undo_redo->commit_action();

		if (cursor.field < 2) {
			cursor.field++;
		} else {
			cursor.field = 0;
			cursor.row++;
		}
		_validate_cursor();
		queue_draw();
	} else {
		return false; //not handled
	}
	return true; //handled
}

bool OrderlistEditor::on_key_release_event(GdkEventKey *key_event) {

	return false;
}

void OrderlistEditor::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {
	minimum_width = fw_cache * 9; //7 and half to each side
	natural_width = fw_cache * 9; //7 and half to each side
}

void OrderlistEditor::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = 64;
	natural_height = 64;
}

void OrderlistEditor::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = 64;
	natural_height = 64;
}

void OrderlistEditor::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = fw_cache * 9; //7 and half to each side
	natural_width = fw_cache * 9; //7 and half to each side
}

void OrderlistEditor::on_size_allocate(Gtk::Allocation &allocation) {
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

void OrderlistEditor::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void OrderlistEditor::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void OrderlistEditor::on_realize() {
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

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK | Gdk::SCROLL_MASK |
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

void OrderlistEditor::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void OrderlistEditor::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
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

void OrderlistEditor::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void OrderlistEditor::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

void OrderlistEditor::_draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x + w / 4, y + h / 4);
	cr->line_to(x + w * 3 / 4, y + h / 4);
	cr->line_to(x + w / 2, y + h * 3 / 4);
	cr->line_to(x + w / 4, y + h / 4);
	cr->fill();
	cr->stroke();
}

void OrderlistEditor::_v_scroll_changed() {
	if (drawing) {
		return;
	}

	v_offset = v_scroll->get_value();
	queue_draw();
}

bool OrderlistEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {

	drawing = true;

	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_BACKGROUND]);
	cr->rectangle(0, 0, w, h);
	cr->fill();

	theme->select_font_face(cr);

	Cairo::FontExtents fe;
	cr->get_font_extents(fe);

	// Believe it or not, this the only reliable way to get the width of
	// a monospace char in GTK. Yes.

	Cairo::TextExtents te;
	cr->get_text_extents("XXX", te);
	int fw = te.width;
	cr->get_text_extents("XX", te);
	fw -= te.width;

	int fh = fe.height;
	int fa = fe.ascent;
	int sep = 1;
	fh += sep;

	int row_height = fh;

	if (fw_cache != fw || fh_cache != fh) {
		queue_resize();
		fw_cache = fw;
		fh_cache = fh;
	}

	visible_rows = (h - top_ofs) / fh;

	Gdk::RGBA note_color = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];
	Gdk::RGBA order_color = theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_BEAT];

	_draw_fill_rect(cr, w / 2, 0, w / 2, h, theme->colors[Theme::COLOR_PATTERN_EDITOR_BG]);

	for (int i = 0; i < visible_rows; i++) {
		char text[4] = { '0', '0', '0', 0 };
		int row = i + v_offset;

		if (row > Song::ORDER_MAX) {
			break;
		}
		text[0] = '0' + row / 100;
		text[1] = '0' + (row / 10) % 10;
		text[2] = '0' + row % 10;

		_draw_text(cr, fw, i * row_height + fa + top_ofs, text, order_color);

		int pattern = song->order_get(row);

		if (pattern == Song::ORDER_EMPTY) {
			text[0] = '.';
			text[1] = '.';
			text[2] = '.';
		} else if (pattern == Song::ORDER_SKIP) {
			text[0] = '-';
			text[1] = '-';
			text[2] = '-';
		} else {
			text[0] = '0' + pattern / 100;
			text[1] = '0' + (pattern / 10) % 10;
			text[2] = '0' + pattern % 10;
		}

		_draw_text(cr, fw + fw * 4, i * row_height + fa + top_ofs, text, note_color);

		if (has_focus() && row == cursor.row) {
			_draw_rect(cr, fw + fw * 4 + fw * cursor.field, i * row_height + top_ofs, fw, fh - 1, theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR]);
		}
	}

	v_scroll->set_upper(Song::ORDER_MAX + 1);
	v_scroll->set_page_size(visible_rows);
	v_scroll->set_value(v_offset);

	if (has_focus()) {
		cr->set_source_rgba(1, 1, 1, 1);
		cr->rectangle(0, 0, w, h);
		cr->stroke();
	}

	drawing = false;

	return true;
}

void OrderlistEditor::set_vscroll(Glib::RefPtr<Gtk::Adjustment> p_v_scroll) {
	v_scroll = p_v_scroll;
	v_scroll->signal_value_changed().connect(sigc::mem_fun(*this, &OrderlistEditor::_v_scroll_changed));
}

void OrderlistEditor::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}

OrderlistEditor::OrderlistEditor(Song *p_song, UndoRedo *p_undo_redo,
		Theme *p_theme, KeyBindings *p_bindings) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("orderlist_editor"),
		Gtk::Widget() {

	song = p_song;
	undo_redo = p_undo_redo;
	key_bindings = p_bindings;
	theme = p_theme;
	set_has_window(true);
	set_can_focus(true);
	set_focus_on_click(true);
	// Gives Exposure & Button presses to the widget.

	set_name("orderlist_editor");

	v_offset = 0;

	visible_rows = 4;

	cursor.row = 0;
	cursor.field = 0;

	fw_cache = 0;
	fh_cache = 0;

	top_ofs = 4;

	drawing = false;
}

OrderlistEditor::~OrderlistEditor() {
}
