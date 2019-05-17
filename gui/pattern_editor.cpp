#include "pattern_editor.h"

void PatternEditor::_cursor_advance() {

	cursor.row += cursor_advance;
	if (cursor.row >= get_total_rows() - 1)
		cursor.row = get_total_rows() - 1;
}

int PatternEditor::_get_rows_per_beat() const {
	static const int rows_per_beat[BEAT_ZOOM_MAX] = {
		1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64
	};
	return rows_per_beat[beat_zoom];
}

void PatternEditor::_field_clear() {

	Track::Pos from;
	from.tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
	from.column = cursor.column;

	Track::Pos to;
	to.tick = from.tick + TICKS_PER_BEAT / _get_rows_per_beat();
	to.column = cursor.column;

	List<Track::PosEvent> events;

	song->get_events_in_range(current_pattern, from, to, &events);

	if (events.size() == 0) {
		_cursor_advance();
		queue_draw();
		return;
	}

	if (song->get_event_column_type(cursor.column) == Track::Event::TYPE_COMMAND) {
		//command works a little different
		undo_redo->begin_action("Clear Command");

		for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
			Track::Event ev = E->get().event;
			Track::Event old_ev = ev;
			if (cursor.field == 0) {
				ev.a = Track::Command::EMPTY;
			} else {
				ev.b = 0;
			}
			undo_redo->do_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, ev);
			undo_redo->undo_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, old_ev);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
		}
		undo_redo->commit_action();

	} else if (cursor.field == 0 || cursor.field == 1) { // just clear whathever

		undo_redo->begin_action("Clear Event");

		for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
			Track::Event ev = E->get().event;
			Track::Event old_ev = ev;
			ev.a = Track::Note::EMPTY;
			ev.b = 0xFF;
			undo_redo->do_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, ev);
			undo_redo->undo_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, old_ev);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
		}
		undo_redo->commit_action();
	} else {
		undo_redo->begin_action("Clear Volume");

		for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
			Track::Event ev = E->get().event;
			Track::Event old_ev = ev;
			ev.b = Track::Note::EMPTY;
			undo_redo->do_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, ev);
			undo_redo->undo_method(song, &Song::set_event, current_pattern,
					cursor.column, E->get().pos.tick, old_ev);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
		}
		undo_redo->commit_action();
	}
	queue_draw();
	_cursor_advance();
	_validate_cursor();
}

bool PatternEditor::_is_in_selection(int p_column, Tick p_tick) {

	return (selection.active && p_column >= selection.begin_column && p_column <= selection.end_column && p_tick >= selection.begin_tick && p_tick < (selection.end_tick + selection.row_tick_size - 1));
}
void PatternEditor::_validate_selection() {
	if (selection.begin_column > selection.end_column) {
		SWAP(selection.begin_column, selection.end_column);
	}
	if (selection.begin_tick > selection.end_tick) {
		SWAP(selection.begin_tick, selection.end_tick);
	}
}

void PatternEditor::_validate_cursor() {

	if (song->get_track_count() == 0) {
		return;
	}

	if (cursor.row < 0)
		cursor.row = 0;
	else if (cursor.row >= get_total_rows())
		cursor.row = get_total_rows() - 1;

	if (cursor.row < v_offset)
		v_offset = cursor.row;
	if (cursor.row >= v_offset + visible_rows)
		v_offset = cursor.row - visible_rows + 1;

	if (cursor.column < h_offset) {
		h_offset = cursor.column;
	}

	while (true) {
		int base_ofs = h_offset ? get_column_offset(h_offset - 1) : 0;
		int window_size = get_allocated_width() - fw_cache * 4; //minus number row
		int cursor_end = get_column_offset(cursor.column);

		if (h_offset == song->get_event_column_count() - 1) {
			break;
		}
		if (base_ofs + window_size >= cursor_end) {
			break;
		}

		h_offset++;
	}

	queue_draw();
}

int PatternEditor::get_total_rows() const {

	return song->pattern_get_beats(current_pattern) * _get_rows_per_beat();
}

int PatternEditor::get_visible_rows() const {
	return visible_rows;
}

int PatternEditor::_cursor_get_track_begin_column() {

	Track *track;
	int automation;
	int column;
	int command;
	get_cursor_column_data(&track, command, automation, column);
	ERR_FAIL_COND_V(!track, false);

	int ccolumn = cursor.column;
	if (column != 0) {
		if (column >= 0) {
			ccolumn -= column;
		} else if (command >= 0) {
			ccolumn -= track->get_column_count() + command;
		} else {
			ccolumn -= automation + track->get_column_count() + track->get_command_column_count();
		}
	}

	return ccolumn;
}

int PatternEditor::_cursor_get_track_end_column() {
	Track *track;
	int automation;
	int column;
	int command;
	get_cursor_column_data(&track, command, automation, column);
	ERR_FAIL_COND_V(!track, false);

	int ccolumn = cursor.column;
	if (column != 0) {
		if (column >= 0) {
			ccolumn -= column;
		} else if (command >= 0) {
			ccolumn -= track->get_column_count() + command;
		} else {
			ccolumn -= automation + track->get_column_count() + track->get_command_column_count();
		}
	}

	ccolumn += track->get_event_column_count() - 1;

	return ccolumn;
}

void PatternEditor::get_cursor_column_data(Track **r_track, int &r_command_column, int &r_automation,
		int &r_track_column) {

	int cc = cursor.column;

	r_automation = -1;
	r_track_column = -1;
	r_command_column = -1;
	*r_track = NULL;

	for (int i = 0; i < song->get_track_count(); i++) {

		Track *t = song->get_track(i);
		*r_track = t;
		r_automation = -1;
		r_command_column = -1;
		r_track_column = -1;

		for (int j = 0; j < t->get_column_count(); j++) {

			r_track_column = j;
			if (cc == 0) {
				return;
			}
			cc--;
		}

		r_track_column = -1;

		for (int j = 0; j < t->get_command_column_count(); j++) {

			r_command_column = j;
			if (cc == 0) {
				return;
			}
			cc--;
		}

		r_command_column = -1;

		for (int j = 0; j < t->get_automation_count(); j++) {

			r_automation = j;

			if (cc == 0) {
				return;
			}

			cc--;
		}
	}
}

void PatternEditor::set_current_pattern(int p_pattern) {

	current_pattern = p_pattern;
	queue_draw();
}

int PatternEditor::get_current_pattern() const {
	return current_pattern;
}

void PatternEditor::set_current_octave(int p_octave) {
	current_octave = p_octave;
}
int PatternEditor::get_current_octave() const {
	return current_octave;
}

void PatternEditor::set_current_cursor_advance(int p_cursor_advance) {
	cursor_advance = p_cursor_advance;
}
int PatternEditor::get_current_cursor_advance() const {
	return cursor_advance;
}

void PatternEditor::set_current_volume_mask(int p_volume_mask, bool p_active) {
	volume_mask = p_volume_mask;
	volume_mask_active = p_active;
}
int PatternEditor::get_current_volume_mask() const {
	return volume_mask;
}
bool PatternEditor::is_current_volume_mask_active() const {
	return volume_mask_active;
}

void PatternEditor::_redraw() {
	queue_draw();
}

void PatternEditor::initialize_menus() {

	track_menu = Gio::Menu::create();
	track_menu_add = Gio::Menu::create();
	track_menu->append_section(track_menu_add);
	track_menu_add->append("New Track", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_ADD_TRACK).ascii().get_data());
	track_menu_column = Gio::Menu::create();
	track_menu->append_section(track_menu_column);
	track_menu_column->append("Add Column", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_ADD_COLUMN).ascii().get_data());
	track_menu_column->append("Remove Column", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_REMOVE_COLUMN).ascii().get_data());
	track_menu_command = Gio::Menu::create();
	track_menu->append_section(track_menu_command);
	track_menu_command->append("Add Command Column", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_ADD_COMMAND_COLUMN).ascii().get_data());
	track_menu_command->append("Remove Command Column", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_REMOVE_COMMAND_COLUMN).ascii().get_data());
	track_menu_solo = Gio::Menu::create();
	track_menu->append_section(track_menu_solo);
	track_menu_solo->append("Mute", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_MUTE).ascii().get_data());
	track_menu_solo->append("Solo", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_SOLO).ascii().get_data());
	track_menu_edit = Gio::Menu::create();
	track_menu->append_section(track_menu_edit);
	track_menu_edit->append("Move Left", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_MOVE_LEFT).ascii().get_data());
	track_menu_edit->append("Move Right", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_MOVE_RIGHT).ascii().get_data());
	track_menu_edit->append("Rename", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_RENAME).ascii().get_data());
	track_menu_remove = Gio::Menu::create();
	track_menu->append_section(track_menu_remove);
	track_menu_remove->append("Remove", key_bindings->get_keybind_detailed_name(KeyBindings::TRACK_REMOVE).ascii().get_data());
	track_popup.bind_model(track_menu, true);

	automation_menu = Gio::Menu::create();
	automation_menu_mode = Gio::Menu::create();
	automation_menu->append_section(automation_menu_mode);
	automation_menu_mode->append("Numbers (Discrete)", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS).ascii().get_data());
	automation_menu_mode->append("Small Envelope", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_SMALL).ascii().get_data());
	automation_menu_mode->append("Large Envelope", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_LARGE).ascii().get_data());

	automation_menu_move = Gio::Menu::create();
	automation_menu->append_section(automation_menu_move);
	automation_menu_move->append("Move Left", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_MOVE_LEFT).ascii().get_data());
	automation_menu_move->append("Move Right", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_MOVE_RIGHT).ascii().get_data());
	automation_menu_remove = Gio::Menu::create();
	automation_menu->append_section(automation_menu_remove);
	automation_menu_remove->append("Remove", key_bindings->get_keybind_detailed_name(KeyBindings::AUTOMATION_REMOVE).ascii().get_data());

	automation_popup.bind_model(automation_menu, true);

	track_popup.attach_to_widget(*this);
	automation_popup.attach_to_widget(*this);
}
void PatternEditor::_mouse_button_event(GdkEventButton *event, bool p_press) {

	if (song->get_track_count() == 0) {
		return;
	}

	Gdk::Rectangle posr(event->x, event->y, 1, 1);

	if (p_press && event->button == 1) {

		int closest_field = -1;
		int closest_column = -1;
		int closest_distance = 0x7FFFFFFF;

		for (List<ClickArea>::Element *E = click_areas.front(); E; E = E->next()) {

			int point_index = -1;
			float point_d = 1e20;

			int pos_x = 0;
			int pos_y = 0;
			for (List<ClickArea::AutomationPoint>::Element *F =
							E->get().automation_points.front();
					F; F = F->next()) {

				float x = F->get().x;
				float y = F->get().y;

				float d = sqrt((x - event->x) * (x - event->x) +
							   (y - event->y) * (y - event->y));

				if (d < 6) {
					if (point_index < 0 || d < point_d) {
						point_index = F->get().index;
						point_d = d;
						pos_x = x;
						pos_y = y;
					}
				}
			}

			if (point_index >= 0) {
				grabbing_point = point_index;
				grabbing_point_tick_from = E->get().automation->get_point_tick_by_index(
						current_pattern, grabbing_point);
				grabbing_point_value_from = E->get().automation->get_point_by_index(
						current_pattern, grabbing_point);
				grabbing_point_tick = grabbing_point_tick_from;
				grabbing_point_value = grabbing_point_value_from;
				grabbing_automation = E->get().automation;
				grabbing_x = E->get().fields[0].x;
				grabbing_width = E->get().fields[0].width;
				grabbing_mouse_pos_x = pos_x;
				grabbing_mouse_pos_y = pos_y;
				grabbing_mouse_prev_x = grabbing_mouse_pos_x;
				grabbing_mouse_prev_y = grabbing_mouse_pos_y;

				return;
			} else if (event->state & GDK_CONTROL_MASK &&
					   event->x >= E->get().fields[0].x &&
					   event->x < E->get().fields[0].x + E->get().fields[0].width) {
				// add it
				int x = event->x - E->get().fields[0].x;
				int y = event->y;
				int w = E->get().fields[0].width;

				Tick tick = MAX(0, (y - row_top_ofs + v_offset * row_height_cache)) *
							TICKS_PER_BEAT / (row_height_cache * _get_rows_per_beat());

				uint8_t value =
						CLAMP((x)*Automation::VALUE_MAX / w, 0, Automation::VALUE_MAX);

				grabbing_automation = E->get().automation;
				grabbing_automation->set_point(current_pattern, tick, value);
				grabbing_point = 1; // useless, can be anything here
				grabbing_point_tick_from = tick;
				grabbing_point_value_from = Automation::EMPTY;
				grabbing_point_tick = tick;
				grabbing_point_value = value;
				grabbing_x = E->get().fields[0].x;
				grabbing_width = E->get().fields[0].width;
				grabbing_mouse_pos_x = pos_x;
				grabbing_mouse_pos_y = pos_y;
				grabbing_mouse_prev_x = grabbing_mouse_pos_x;
				grabbing_mouse_prev_y = grabbing_mouse_pos_y;

				queue_draw();
				return;
			} else {
				for (int i = 0; i < E->get().fields.size(); i++) {
					int localx = event->x - E->get().fields[i].x;
					if (localx >= 0 && localx < E->get().fields[i].width) {
						cursor.field = i;
						cursor.column = E->get().column;
						queue_draw();
						cursor.row = (event->y / row_height_cache) + v_offset;
						_validate_menus();

						selection.mouse_drag_from_column = cursor.column;
						selection.mouse_drag_from_row = cursor.row;
						selection.mouse_drag_active = true;
						return;
					} else {

						int distance = localx < 0 ? -localx : localx - E->get().fields[i].width;
						if (distance < closest_distance) {
							closest_distance = distance;
							closest_field = i;
							closest_column = E->get().column;
						}
					}
				}
			}
		}

		if (closest_column >= 0) {
			cursor.field = closest_field;
			cursor.column = closest_column;
			queue_draw();
			cursor.row = (event->y / row_height_cache) + v_offset;

			selection.mouse_drag_from_column = cursor.column;
			selection.mouse_drag_from_row = cursor.row;
			selection.mouse_drag_active = true;

			_validate_menus();
			return;
		}
	}

	if (p_press && event->button == 3 && grabbing_point == -1) {
		// remove
		for (List<ClickArea>::Element *E = click_areas.front(); E; E = E->next()) {

			int point_index = -1;
			float point_d = 1e20;
			for (List<ClickArea::AutomationPoint>::Element *F =
							E->get().automation_points.front();
					F; F = F->next()) {

				float x = F->get().x;
				float y = F->get().y;

				float d = sqrt((x - event->x) * (x - event->x) +
							   (y - event->y) * (y - event->y));

				if (d < 4) {
					if (point_index < 0 || d < point_d) {
						point_index = F->get().index;
						point_d = d;
					}
				}
			}

			if (point_index >= 0) {
				undo_redo->begin_action("Remove Point");
				Tick tick = E->get().automation->get_point_tick_by_index(
						current_pattern, point_index);
				uint8_t value = E->get().automation->get_point_by_index(current_pattern,
						point_index);

				undo_redo->do_method(E->get().automation, &Automation::remove_point,
						current_pattern, tick);
				undo_redo->undo_method(E->get().automation, &Automation::set_point,
						current_pattern, tick, value);
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();
				return;
			}
		}

		//nothing found, popup relevant menus
		{

			int closest_field = -1;
			int closest_column = -1;
			int closest_distance = 0x7FFFFFFF;

			for (List<ClickArea>::Element *E = click_areas.front(); E; E = E->next()) {

				for (int i = 0; i < E->get().fields.size(); i++) {
					int localx = event->x - E->get().fields[i].x;
					if (localx >= 0 && localx < E->get().fields[i].width) {
						closest_field = i;
						closest_column = E->get().column;
						closest_distance = 0;
						break;

					} else {

						int distance = localx < 0 ? -localx : localx - E->get().fields[i].width;
						if (distance < closest_distance) {
							closest_distance = distance;
							closest_field = i;
							closest_column = E->get().column;
						}
					}
				}
				if (closest_distance == 0) {
					break;
				}
			}

			if (closest_column >= 0) {

				cursor.field = closest_field;
				cursor.column = closest_column;
				queue_draw();
				//cursor.row = (event->y / row_height_cache) + v_offset;

				_validate_menus();

				int automation = song->get_event_column_automation(closest_column);

				if (song->get_event_column_automation(closest_column) >= 0) {
					automation_popup.popup(event->button, event->time);
				} else {
					track_popup.popup(event->button, event->time);
				}
			}
		}
	}

	if (!p_press && event->button == 1) {

		if (grabbing_point >= 0) {
			grabbing_point = -1;

			undo_redo->begin_action("Move Point");
			undo_redo->do_method(grabbing_automation, &Automation::remove_point,
					current_pattern, grabbing_point_tick_from);
			undo_redo->do_method(grabbing_automation, &Automation::set_point,
					current_pattern, grabbing_point_tick,
					grabbing_point_value);
			undo_redo->undo_method(grabbing_automation, &Automation::remove_point,
					current_pattern, grabbing_point_tick);
			undo_redo->undo_method(grabbing_automation, &Automation::set_point,
					current_pattern, grabbing_point_tick_from,
					grabbing_point_value_from);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		}

		selection.mouse_drag_active = false;
	}
}

bool PatternEditor::on_scroll_event(GdkEventScroll *scroll_event) {

	if (scroll_event->direction == GDK_SCROLL_UP) {
		v_scroll->set_value(v_scroll->get_value() - _get_rows_per_beat());
		return true;
	}
	if (scroll_event->direction == GDK_SCROLL_DOWN) {
		v_scroll->set_value(v_scroll->get_value() + _get_rows_per_beat());
		return true;
	}

	return false;
}
bool PatternEditor::on_button_press_event(GdkEventButton *event) {
	grab_focus();
	_mouse_button_event(event, true);
	return false;
}

bool PatternEditor::on_button_release_event(GdkEventButton *release_event) {
	_mouse_button_event(release_event, false);
	return false;
}

bool PatternEditor::on_motion_notify_event(GdkEventMotion *motion_event) {

	if (song->get_track_count() == 0) {
		return false;
	}

	if (selection.mouse_drag_active) {
		int closest_field = -1;
		int closest_column = -1;
		int closest_distance = 0x7FFFFFFF;

		for (List<ClickArea>::Element *E = click_areas.front(); E; E = E->next()) {

			for (int i = 0; i < E->get().fields.size(); i++) {
				int localx = motion_event->x - E->get().fields[i].x;
				if (localx >= 0 && localx < E->get().fields[i].width) {
					closest_field = i;
					closest_column = E->get().column;
					closest_distance = 0;
					break;

				} else {

					int distance = localx < 0 ? -localx : localx - E->get().fields[i].width;
					if (distance < closest_distance) {
						closest_distance = distance;
						closest_field = i;
						closest_column = E->get().column;
					}
				}
			}
			if (closest_distance == 0) {
				break;
			}
		}

		if (closest_column >= 0) {

			int row = (motion_event->y / row_height_cache) + v_offset;
			bool prev_active = selection.active;
			selection.active = true;
			selection.begin_column = selection.mouse_drag_from_column;
			selection.begin_tick = selection.mouse_drag_from_row * TICKS_PER_BEAT / _get_rows_per_beat();
			selection.end_column = closest_column;
			selection.end_tick = row * TICKS_PER_BEAT / _get_rows_per_beat();
			selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();

			_validate_selection();
			if (prev_active != selection.active) {
				_validate_menus();
			}
			queue_draw();
			return true;
		}
	}

	if (grabbing_point >= 0) {

		grabbing_mouse_prev_x = grabbing_mouse_pos_x;
		grabbing_mouse_prev_y = grabbing_mouse_pos_y;

		int rel_x = motion_event->x - grabbing_mouse_prev_x;
		int rel_y = motion_event->y - grabbing_mouse_prev_y;

		grabbing_mouse_prev_x = motion_event->x;
		grabbing_mouse_prev_y = motion_event->y;

		grabbing_mouse_pos_x += rel_x;
		grabbing_mouse_pos_y += rel_y;

		int y = grabbing_mouse_pos_y;
		int x = grabbing_mouse_pos_x;
		Tick tick = MAX(0, (y - row_top_ofs + v_offset * row_height_cache)) *
					TICKS_PER_BEAT / (row_height_cache * _get_rows_per_beat());

		grabbing_automation->remove_point(current_pattern, grabbing_point_tick);
		while (grabbing_automation->get_point(current_pattern, tick) !=
				Automation::EMPTY) {
			tick++;
		}
		uint8_t value =
				CLAMP((x - grabbing_x) * Automation::VALUE_MAX / grabbing_width, 0,
						Automation::VALUE_MAX);

		grabbing_point_tick = tick;
		grabbing_point_value = value;

		grabbing_automation->set_point(current_pattern, tick, value);
		queue_draw();
	}

	return false;
}

void PatternEditor::_validate_menus() {

	//begin by validating cursor
	_validate_cursor();

	int current_track = get_current_track();

	key_bindings->set_action_enabled(KeyBindings::TRACK_ADD_COLUMN, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::TRACK_REMOVE_COLUMN, current_track >= 0 && song->get_track(current_track)->get_column_count() > 1);
	key_bindings->set_action_enabled(KeyBindings::TRACK_ADD_COMMAND_COLUMN, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::TRACK_REMOVE_COMMAND_COLUMN, current_track >= 0 && song->get_track(current_track)->get_command_column_count() > 0);
	key_bindings->set_action_enabled(KeyBindings::TRACK_MOVE_LEFT, current_track > 0);
	key_bindings->set_action_enabled(KeyBindings::TRACK_MOVE_RIGHT, current_track >= 0 && current_track < song->get_track_count() - 1);
	key_bindings->set_action_enabled(KeyBindings::TRACK_MUTE, current_track >= 0);
	key_bindings->set_action_checked(KeyBindings::TRACK_MUTE, current_track >= 0 && song->get_track(current_track)->is_muted());
	key_bindings->set_action_enabled(KeyBindings::TRACK_SOLO, current_track >= 0);

	key_bindings->set_action_enabled(KeyBindings::TRACK_RENAME, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::TRACK_REMOVE, current_track >= 0);

	int current_automation = current_track >= 0 ? song->get_event_column_automation(cursor.column) : -1;

	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS, current_automation >= 0);
	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_RADIO_ENVELOPE_SMALL, current_automation >= 0);
	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_RADIO_ENVELOPE_LARGE, current_automation >= 0);
	if (current_automation >= 0) {
		switch (song->get_track(current_track)->get_automation(current_automation)->get_edit_mode()) {
			case Automation::EDIT_ROWS_DISCRETE: {
				key_bindings->set_action_state(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS, key_bindings->get_keybind_name(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS));
			} break;
			case Automation::EDIT_ENVELOPE_SMALL: {
				key_bindings->set_action_state(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS, key_bindings->get_keybind_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_SMALL));
			} break;
			case Automation::EDIT_ENVELOPE_LARGE: {
				key_bindings->set_action_state(KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS, key_bindings->get_keybind_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_LARGE));
			} break;
		}
	}
	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_MOVE_LEFT, current_automation > 0);
	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_MOVE_RIGHT, current_automation >= 0 && current_automation < song->get_track(current_track)->get_automation_count() - 1);
	key_bindings->set_action_enabled(KeyBindings::AUTOMATION_REMOVE, current_automation >= 0);

	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECT_BEGIN, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECT_END, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECT_COLUMN_TRACK_ALL, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_SEMITONE, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_OCTAVE, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_SEMITONE, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_OCTAVE, current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_SET_VOLUME, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_CUT, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_COPY, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_PASTE_INSERT, clipboard.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_PASTE_OVERWRITE, clipboard.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_PASTE_MIX, clipboard.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_DISABLE, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_DOUBLE_LENGTH, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_HALVE_LENGTH, selection.active && current_track >= 0);
	key_bindings->set_action_enabled(KeyBindings::PATTERN_SELECTION_SCALE_LENGTH, selection.active && current_track >= 0);

	current_track_changed.emit(); //this should probably be optimized a bit
}

void PatternEditor::_notify_track_layout_changed() {
	track_layout_changed.emit();
}

void PatternEditor::_on_action_activated(KeyBindings::KeyBind p_bind) {

	if (p_bind == KeyBindings::TRACK_ADD_TRACK) {
		//always available
		Track *track = new Track;
		String name = "New Track";
		int idx = 1;
		while (true) {
			bool exists = false;
			for (int i = 0; i < song->get_track_count(); i++) {
				if (song->get_track(i)->get_name() == name) {
					exists = true;
					break;
				}
			}

			if (exists) {
				idx++;
				name = "New Track " + String::num(idx);
			} else {
				break;
			}
		}

		track->set_name(name);
		track->add_send(Track::SEND_SPEAKERS);

		undo_redo->begin_action("Add Track");
		undo_redo->do_method(
				song, &Song::add_track, track);
		undo_redo->undo_method(
				song, &Song::remove_track, song->get_track_count());
		undo_redo->do_method(this, &PatternEditor::_redraw);
		undo_redo->undo_method(this, &PatternEditor::_redraw);
		undo_redo->do_method(this, &PatternEditor::_validate_menus);
		undo_redo->undo_method(this, &PatternEditor::_validate_menus);
		undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
		undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);
		undo_redo->do_data(track);
		undo_redo->commit_action();
	}

	int current_track = get_current_track();
	if (current_track >= 0) {
		switch (p_bind) {

			case KeyBindings::TRACK_ADD_COLUMN: {

				undo_redo->begin_action("Add Column");
				undo_redo->do_method(
						song->get_track(current_track), &Track::set_columns,
						song->get_track(current_track)->get_column_count() + 1);
				undo_redo->undo_method(
						song->get_track(current_track), &Track::set_columns,
						song->get_track(current_track)->get_column_count());
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);

				undo_redo->commit_action();
			} break;
			case KeyBindings::TRACK_REMOVE_COLUMN: {

				ERR_FAIL_COND(song->get_track(current_track)->get_column_count() <= 1);
				undo_redo->begin_action("Remove Column");
				undo_redo->do_method(
						song->get_track(current_track), &Track::set_columns,
						song->get_track(current_track)->get_column_count() - 1);
				undo_redo->undo_method(
						song->get_track(current_track), &Track::set_columns,
						song->get_track(current_track)->get_column_count());
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->commit_action();

			} break;
			case KeyBindings::TRACK_ADD_COMMAND_COLUMN: {

				undo_redo->begin_action("Add Command Column");
				undo_redo->do_method(
						song->get_track(current_track), &Track::set_command_columns,
						song->get_track(current_track)->get_command_column_count() + 1);
				undo_redo->undo_method(
						song->get_track(current_track), &Track::set_command_columns,
						song->get_track(current_track)->get_command_column_count());
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);

				undo_redo->commit_action();
			} break;
			case KeyBindings::TRACK_REMOVE_COMMAND_COLUMN: {

				ERR_FAIL_COND(song->get_track(current_track)->get_command_column_count() == 0);
				undo_redo->begin_action("Remove Command Column");
				undo_redo->do_method(
						song->get_track(current_track), &Track::set_command_columns,
						song->get_track(current_track)->get_command_column_count() - 1);
				undo_redo->undo_method(
						song->get_track(current_track), &Track::set_command_columns,
						song->get_track(current_track)->get_command_column_count());
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->commit_action();

			} break;
			case KeyBindings::TRACK_SOLO: {

				bool unsolo = true;

				for (int i = 0; i < song->get_track_count(); i++) {

					if (i == current_track) {
						if (song->get_track(i)->is_muted()) {
							unsolo = false;
							break;
						}
					} else {
						if (!song->get_track(i)->is_muted()) {
							unsolo = false;
							break;
						}
					}
				}

				undo_redo->begin_action("Solo");
				for (int i = 0; i < song->get_track_count(); i++) {

					if (unsolo) {
						undo_redo->do_method(song->get_track(i), &Track::set_muted,
								false);
					} else {
						undo_redo->do_method(song->get_track(i), &Track::set_muted,
								i != current_track);
					}
					undo_redo->undo_method(song->get_track(i), &Track::set_muted,
							song->get_track(i)->is_muted());
				}
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->commit_action();

			} break;
			case KeyBindings::TRACK_MUTE: {

				undo_redo->begin_action("Mute");
				undo_redo->do_method(song->get_track(current_track), &Track::set_muted,
						!song->get_track(current_track)->is_muted());
				undo_redo->undo_method(song->get_track(current_track),
						&Track::set_muted,
						song->get_track(current_track)->is_muted());
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->commit_action();

			} break;

			case KeyBindings::TRACK_RENAME: {

				/*undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);*/

				Gtk::MessageDialog dialog("Enter Track Name:",
						false /* use_markup */, Gtk::MESSAGE_QUESTION,
						Gtk::BUTTONS_OK_CANCEL);
				dialog.set_title("Rename Track");

				dialog.set_transient_for(*static_cast<Gtk::Window *>(get_toplevel()));
				dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

				Gtk::Entry entry;
				entry.set_text(song->get_track(current_track)->get_name().utf8().get_data());
				dialog.get_vbox()->pack_start(entry, Gtk::PACK_SHRINK);
				entry.show();
				//make sure pressing enter also causes rename
				entry.signal_activate().connect(sigc::bind<int>(sigc::mem_fun(dialog, &Gtk::MessageDialog::response), int(Gtk::RESPONSE_OK)));

				if (dialog.run() == Gtk::RESPONSE_OK) {
					String name = entry.get_text().c_str();
					if (name != song->get_track(current_track)->get_name()) {
						undo_redo->begin_action("Track Rename");
						undo_redo->do_method(song->get_track(current_track), &Track::set_name, name);
						undo_redo->undo_method(song->get_track(current_track), &Track::set_name, song->get_track(current_track)->get_name());
						undo_redo->do_method(this, &PatternEditor::_redraw);
						undo_redo->undo_method(this, &PatternEditor::_redraw);
						undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
						undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);
						undo_redo->commit_action();
					}
				}

			} break;
			case KeyBindings::TRACK_MOVE_LEFT: {

				ERR_FAIL_COND(current_track == 0);

				cursor.column -= song->get_track(current_track - 1)->get_event_column_count();

				undo_redo->begin_action("Track Move Left");
				undo_redo->do_method(song, &Song::swap_tracks, current_track,
						current_track - 1);
				undo_redo->undo_method(song, &Song::swap_tracks, current_track,
						current_track - 1);
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->commit_action();

			} break;
			case KeyBindings::TRACK_MOVE_RIGHT: {

				ERR_FAIL_COND(current_track == song->get_track_count() - 1);

				cursor.column += song->get_track(current_track + 1)->get_event_column_count();

				undo_redo->begin_action("Track Move Right");
				undo_redo->do_method(song, &Song::swap_tracks, current_track,
						current_track + 1);
				undo_redo->undo_method(song, &Song::swap_tracks, current_track,
						current_track + 1);
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->commit_action();

			} break;
			case KeyBindings::TRACK_REMOVE: {

				undo_redo->begin_action("Remove");
				undo_redo->do_method(song, &Song::remove_track, current_track);
				undo_redo->undo_method(song, &Song::add_track_at_pos,
						song->get_track(current_track),
						current_track);
				for (int i = 0; i < song->get_track_count(); i++) {
					if (i == current_track) {
						continue;
					}
					Track *track = song->get_track(i);
					for (int j = 0; j < track->get_send_count(); j++) {
						int send_to = track->get_send_track(j);
						if (send_to == Track::SEND_SPEAKERS) {
							continue; //nothing to do
						}
						int removed = 0;
						if (send_to < current_track) {
							continue;

						} else if (send_to > current_track) {
							undo_redo->do_method(track, &Track::set_send_track, j - removed, send_to - 1);
							undo_redo->undo_method(track, &Track::set_send_track, j - removed, send_to);
						} else {
							//removee
							undo_redo->do_method(track, &Track::remove_send, j - removed);
							undo_redo->undo_method(track, &Track::add_send, current_track, j - removed);
							undo_redo->undo_method(track, &Track::set_send_amount, j - removed, track->get_send_amount(j));
							undo_redo->undo_method(track, &Track::set_send_mute, j - removed, track->is_send_muted(j));
							removed++;
						}
					}
				}
				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->undo_data(song->get_track(current_track));
				undo_redo->do_method(this, &PatternEditor::_validate_menus);
				undo_redo->undo_method(this, &PatternEditor::_validate_menus);
				undo_redo->do_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->undo_method(this, &PatternEditor::_notify_track_layout_changed);
				undo_redo->commit_action();

			} break;

			case KeyBindings::PATTERN_SELECT_BEGIN: {
				if (!selection.active) {
					selection.begin_column = cursor.column;
					selection.begin_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.end_column = cursor.column;
					selection.end_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
					selection.active = true;
					_validate_menus();
				} else {
					selection.begin_column = cursor.column;
					selection.begin_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
					_validate_selection();
				}

				queue_draw();
			} break;

			case KeyBindings::PATTERN_SELECT_END: {
				if (!selection.active) {
					selection.begin_column = cursor.column;
					selection.begin_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.end_column = cursor.column;
					selection.end_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
					selection.active = true;
					_validate_menus();
				} else {
					selection.end_column = cursor.column;
					selection.end_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
					_validate_selection();
				}

				queue_draw();

			} break;
			case KeyBindings::PATTERN_SELECT_COLUMN_TRACK_ALL: {

				Tick pattern_ticks = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;
				pattern_ticks -= TICKS_PER_BEAT / _get_rows_per_beat();

				int pattern_columns = song->get_event_column_count();

				Track *track;
				int automation;
				int column;
				int command;
				get_cursor_column_data(&track, command, automation, column);
				ERR_FAIL_COND(!track);

				int track_begin_column;

				if (column >= 0) {
					track_begin_column = cursor.column - column;
				} else if (command >= 0) {
					track_begin_column = cursor.column - column - track->get_column_count();
				} else {
					track_begin_column = cursor.column - automation - (track->get_column_count() + track->get_command_column_count());
				}

				int track_event_columns = track->get_event_column_count();
				int track_end_column = track_begin_column + track_event_columns - 1;

				//printf("track begin: %i, track end %i\n", track_begin_column, track_end_column);

				if (selection.active && selection.begin_column == track_begin_column && selection.end_column == track_end_column && selection.begin_tick == 0 && selection.end_tick == pattern_ticks) {
					//in track (or single column), switch to all (if there is anything to switch to)
					if (selection.begin_column != 0 || selection.end_column != pattern_columns - 1) {
						selection.begin_column = 0;
						selection.end_column = pattern_columns - 1;
						queue_draw();
						break;
					}
				}

				if (selection.active && selection.begin_column == cursor.column && selection.end_column == cursor.column && selection.begin_tick == 0 && selection.end_tick == pattern_ticks) {
					//in column, switch to track, unless track is a single column
					selection.begin_column = track_begin_column;
					selection.end_column = track_end_column;
					queue_draw();
					break;
				}

				//select column

				selection.begin_column = cursor.column;
				selection.end_column = cursor.column;
				selection.begin_tick = 0;
				selection.end_tick = pattern_ticks;
				selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
				selection.active = true;
				_validate_menus();
				queue_draw();

			} break;
			case KeyBindings::PATTERN_SELECTION_DISABLE: {

				selection.active = false;
				_validate_menus();
				queue_draw();

			} break;
				/* selection AREA actions */

			case KeyBindings::PATTERN_SELECTION_RAISE_NOTES_SEMITONE:
			case KeyBindings::PATTERN_SELECTION_RAISE_NOTES_OCTAVE:
			case KeyBindings::PATTERN_SELECTION_LOWER_NOTES_SEMITONE:
			case KeyBindings::PATTERN_SELECTION_LOWER_NOTES_OCTAVE: {

				int amount = 0;
				String action;
				switch (p_bind) {
					case KeyBindings::PATTERN_SELECTION_RAISE_NOTES_SEMITONE:
						amount = 1;
						action = "Raise Semitone";
						break;
					case KeyBindings::PATTERN_SELECTION_RAISE_NOTES_OCTAVE:
						amount = 12;
						action = "Raise Octave";
						break;
					case KeyBindings::PATTERN_SELECTION_LOWER_NOTES_SEMITONE:
						amount = -1;
						action = "Lower Semitone";
						break;
					case KeyBindings::PATTERN_SELECTION_LOWER_NOTES_OCTAVE:
						amount = -12;
						action = "Raise Octave";
						break;
				}

				int column_from;
				int column_to;
				Tick tick_from;
				Tick tick_to;

				if (selection.active) {
					column_from = selection.begin_column;
					tick_from = selection.begin_tick;
					column_to = selection.end_column;
					tick_to = selection.end_tick + selection.row_tick_size - 1;
				} else {
					column_from = cursor.column;
					column_to = cursor.column;
					tick_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					tick_to = tick_from + selection.row_tick_size - 1;
				}

				List<Track::PosEvent> events;
				song->get_events_in_range(current_pattern, Track::Pos(tick_from, column_from), Track::Pos(tick_to, column_to), &events);

				if (events.empty()) {
					break;
				}

				undo_redo->begin_action(action, true);

				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
					Track::Event ev = E->get().event;
					if (ev.type == Track::Event::TYPE_NOTE && ev.a < Track::Note::MAX_NOTE) {
						undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						ev.a = CLAMP(int(ev.a + amount), 0, Track::Note::MAX_NOTE);
						undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
					}
				}

				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();
			} break;

			case KeyBindings::PATTERN_SELECTION_SET_VOLUME: {
				int column_from;
				int column_to;
				Tick tick_from;
				Tick tick_to;

				if (selection.active) {
					column_from = selection.begin_column;
					tick_from = selection.begin_tick;
					column_to = selection.end_column;
					tick_to = selection.end_tick + selection.row_tick_size - 1;
				} else {
					column_from = cursor.column;
					column_to = cursor.column;
					tick_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					tick_to = tick_from + selection.row_tick_size - 1;
				}

				List<Track::PosEvent> events;
				song->get_events_in_range(current_pattern, Track::Pos(tick_from, column_from), Track::Pos(tick_to, column_to), &events);

				if (events.empty()) {
					break;
				}

				undo_redo->begin_action("Set Volume Mask", true);

				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
					Track::Event ev = E->get().event;
					if (ev.type == Track::Event::TYPE_NOTE && ev.a < Track::Note::MAX_NOTE) {
						undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						ev.b = volume_mask;
						undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
					}
				}

				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();

			} break;
			case KeyBindings::PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION: {

				if (!selection.active) {
					break;
				}

				bool valid = false;
				for (int i = selection.begin_column; i <= selection.begin_column; i++) {
					Track::Event ev_first = song->get_event(current_pattern, i, selection.begin_tick);
					Track::Event ev_last = song->get_event(current_pattern, i, selection.end_tick);
					if (ev_first.type == Track::Event::TYPE_NOTE && ev_first.a != Track::Note::EMPTY && ev_last.a != Track::Note::EMPTY) {
						valid = true;
						break;
					}
					if (ev_first.type == Track::Event::TYPE_AUTOMATION && ev_first.a != Automation::EMPTY && ev_last.a != Automation::EMPTY) {
						valid = true;
						break;
					}

					if (ev_first.type == Track::Event::TYPE_COMMAND) {
						valid = true;
						break;
					}
				}

				if (!valid) {
					break;
				}

				undo_redo->begin_action("Interpolate", true);

				Tick tick_from = selection.begin_tick;
				Tick tick_to = selection.end_tick;

				for (int i = selection.begin_column; i <= selection.begin_column; i++) {
					Track::Event ev_first = song->get_event(current_pattern, i, selection.begin_tick);
					Track::Event ev_last = song->get_event(current_pattern, i, selection.end_tick);
					if (ev_first.type == Track::Event::TYPE_NOTE && ev_first.a != Track::Note::EMPTY && ev_last.a != Track::Note::EMPTY) {
						//interpolate notes
						List<Track::PosEvent> events;
						song->get_events_in_range(current_pattern, Track::Pos(tick_from, i), Track::Pos(tick_to, i), &events);
						int volume_from = ev_first.b == Track::Note::EMPTY ? 99 : ev_first.b;
						int volume_to = ev_last.b == Track::Note::EMPTY ? 99 : ev_last.b;

						for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {

							Track::Event ev = E->get().event;

							undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);

							float c = float(E->get().pos.tick - tick_from) / float(tick_to - tick_from);
							int volume = CLAMP(int(volume_from * (1.0 - c) + volume_to * c), 0, 99);
							ev.b = volume;

							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						}
					}

					if (ev_first.type == Track::Event::TYPE_COMMAND) {
						//interpolate notes
						List<Track::PosEvent> events;
						song->get_events_in_range(current_pattern, Track::Pos(tick_from, i), Track::Pos(tick_to, i), &events);
						int param_from = ev_first.b;
						int param_to = ev_last.b;

						for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {

							Track::Event ev = E->get().event;

							undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);

							float c = float(E->get().pos.tick - tick_from) / float(tick_to - tick_from);
							int param = CLAMP(int(param_from * (1.0 - c) + param_to * c), 0, 99);
							ev.b = param;

							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						}
					}

					if (ev_first.type == Track::Event::TYPE_AUTOMATION && ev_first.a != Automation::EMPTY && ev_last.a != Automation::EMPTY) {

						//interpolate automations
						int value_from = ev_first.a == Automation::EMPTY ? 99 : ev_first.a;
						int value_to = ev_last.a == Automation::EMPTY ? 99 : ev_last.a;

						Tick tick_increment = TICKS_PER_BEAT / _get_rows_per_beat();
						Tick tick_pos = tick_from;
						while (tick_pos < tick_to) {
							Track::Event ev = song->get_event(current_pattern, i, tick_pos);
							undo_redo->undo_method(song, &Song::set_event, current_pattern, i, tick_pos, ev);

							float c = float(tick_pos - tick_from) / float(tick_to - tick_from);
							int value = CLAMP(int(value_from * (1.0 - c) + value_to * c), 0, 99);
							ev.a = value;

							undo_redo->do_method(song, &Song::set_event, current_pattern, i, tick_pos, ev);

							tick_pos += tick_increment;
						}
					}
				}

				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();
			} break;
			case KeyBindings::PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION: {

				Gtk::MessageDialog dialog("Amplify(%):",
						false /* use_markup */, Gtk::MESSAGE_QUESTION,
						Gtk::BUTTONS_OK_CANCEL);
				dialog.set_title("Amplify Volume / Automation");
				dialog.set_transient_for(*static_cast<Gtk::Window *>(get_toplevel()));
				dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

				Gtk::Entry entry;
				entry.set_text(String::num(last_amplify_value).ascii().get_data());
				dialog.get_vbox()->pack_start(entry, Gtk::PACK_SHRINK);
				entry.show();
				//make sure pressing enter also causes enter
				entry.signal_activate().connect(sigc::bind<int>(sigc::mem_fun(dialog, &Gtk::MessageDialog::response), int(Gtk::RESPONSE_OK)));

				if (dialog.run() == Gtk::RESPONSE_OK) {
					int amount = String(entry.get_text().c_str()).to_int();
					last_amplify_value = amount;
					float ratio = float(amount) / 100;

					int column_from;
					int column_to;
					Tick tick_from;
					Tick tick_to;

					if (selection.active) {
						column_from = selection.begin_column;
						tick_from = selection.begin_tick;
						column_to = selection.end_column;
						tick_to = selection.end_tick + selection.row_tick_size - 1;
					} else {
						column_from = cursor.column;
						column_to = cursor.column;
						tick_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
						tick_to = tick_from + selection.row_tick_size - 1;
					}

					List<Track::PosEvent> events;
					song->get_events_in_range(current_pattern, Track::Pos(tick_from, column_from), Track::Pos(tick_to, column_to), &events);

					if (events.empty()) {
						break;
					}

					undo_redo->begin_action("Amplify", true);

					for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
						Track::Event ev = E->get().event;
						undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);

						if (ev.type == Track::Event::TYPE_NOTE) {
							if (ev.b == Track::Note::EMPTY) {
								ev.b = Track::Note::MAX_VOLUME;
							}
							ev.b = int(CLAMP(float(ev.b) * ratio, 0, Track::Note::MAX_VOLUME));
							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						} else if (ev.type == Track::Event::TYPE_AUTOMATION) {
							ev.a = int(CLAMP(float(ev.a) * ratio, 0, Automation::VALUE_MAX));
							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						}
					}

					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
				}
			} break;
			case KeyBindings::PATTERN_SELECTION_DOUBLE_LENGTH:
			case KeyBindings::PATTERN_SELECTION_HALVE_LENGTH:
			case KeyBindings::PATTERN_SELECTION_SCALE_LENGTH: {

				if (!selection.active) {
					break;
				}
				float scale = 1.0;
				String action;
				switch (p_bind) {
					case KeyBindings::PATTERN_SELECTION_DOUBLE_LENGTH:
						scale = 2.0;
						action = "Double Length";
						break;
					case KeyBindings::PATTERN_SELECTION_HALVE_LENGTH:
						scale = 0.5;
						action = "Halve Length";
						break;
					case KeyBindings::PATTERN_SELECTION_SCALE_LENGTH: {
						action = "Scale Length";
						Gtk::MessageDialog dialog("Scale Ratio:",
								false /* use_markup */, Gtk::MESSAGE_QUESTION,
								Gtk::BUTTONS_OK_CANCEL);
						dialog.set_title("Scale Selection Length");
						dialog.set_transient_for(*static_cast<Gtk::Window *>(get_toplevel()));
						dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
						Gtk::Entry entry;
						entry.set_text(String::num(last_scale_value).ascii().get_data());
						dialog.get_vbox()->pack_start(entry, Gtk::PACK_SHRINK);
						entry.show();
						//make sure pressing enter also causes enter
						entry.signal_activate().connect(sigc::bind<int>(sigc::mem_fun(dialog, &Gtk::MessageDialog::response), int(Gtk::RESPONSE_OK)));

						if (dialog.run() == Gtk::RESPONSE_OK) {

							scale = String(entry.get_text().c_str()).to_double();
							last_scale_value = scale;
						} else {
							return;
						}
					} break;
				}

				int column_from;
				int column_to;
				Tick tick_from;
				Tick tick_to;

				if (selection.active) {
					column_from = selection.begin_column;
					tick_from = selection.begin_tick;
					column_to = selection.end_column;
					tick_to = selection.end_tick + selection.row_tick_size - 1;
				} else {
					column_from = cursor.column;
					column_to = cursor.column;
					tick_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					tick_to = tick_from + selection.row_tick_size - 1;
				}

				List<Track::PosEvent> events;
				song->get_events_in_range(current_pattern, Track::Pos(tick_from, column_from), Track::Pos(tick_to, column_to), &events);

				if (events.empty()) {
					break;
				}

				undo_redo->begin_action(action, true);

				//erase stuff first
				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
					Track::Event ev = Track::Event::make_empty(E->get().event.type);

					undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
				}

				//replace
				Tick max_len = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;

				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {

					Tick new_pos = E->get().pos.tick * scale;
					if (new_pos >= max_len) {
						continue;
					}
					Track::Event ev = song->get_event(current_pattern, E->get().pos.column, new_pos);
					undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, new_pos, ev);
					undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, new_pos, E->get().event);
				}

				//readd undone stuff
				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
					Track::Event ev = E->get().event;
					undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
				}

				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();

			} break;
			/* CLIPBOARD */
			case KeyBindings::PATTERN_SELECTION_CUT:
			case KeyBindings::PATTERN_SELECTION_COPY: {

				int column_from = selection.begin_column;
				int column_to = selection.end_column;
				Tick tick_from = selection.begin_tick;

				Tick tick_to = selection.end_tick + selection.row_tick_size - 1;

				List<Track::PosEvent> events;
				song->get_events_in_range(current_pattern, Track::Pos(tick_from, column_from), Track::Pos(tick_to, column_to), &events);

				if (events.empty()) {
					clipboard.active = false;
					_validate_menus();
					break;
				}

				clipboard.active = true;
				clipboard.columns = column_to - column_from + 1;
				clipboard.ticks = tick_to - tick_from + 1; //else paste insert wont work
				clipboard.events.clear();

				for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
					Track::PosEvent pe = E->get();
					pe.pos.tick -= tick_from;
					pe.pos.column -= column_from;
					clipboard.events.push_back(pe);
				}

				if (p_bind == KeyBindings::PATTERN_SELECTION_CUT) {
					//also cut

					undo_redo->begin_action("Selection Zap (cut)");

					for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {
						Track::Event ev = E->get().event;
						undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);

						ev = Track::Event::make_empty(ev.type);

						undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
					}
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
				}

				_validate_menus();

			} break;
			case KeyBindings::PATTERN_SELECTION_PASTE_INSERT:
			case KeyBindings::PATTERN_SELECTION_PASTE_OVERWRITE:
			case KeyBindings::PATTERN_SELECTION_PASTE_MIX: {
				if (!clipboard.active) {
					break;
				}

				String action;
				switch (p_bind) {
					case KeyBindings::PATTERN_SELECTION_PASTE_INSERT: action = "Paste Insert"; break;
					case KeyBindings::PATTERN_SELECTION_PASTE_OVERWRITE: action = "Paste Overwrite"; break;
					case KeyBindings::PATTERN_SELECTION_PASTE_MIX: action = "Paste Mix"; break;
				}
				undo_redo->begin_action(action);

				int last_column = song->get_event_column_count() - 1;

				//clear space to paste (pre)
				if (p_bind != KeyBindings::PATTERN_SELECTION_PASTE_MIX) {

					Tick clear_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					Tick clear_to = clear_from + clipboard.ticks;

					int clear_from_colum = cursor.column;
					int clear_to_column = cursor.column + clipboard.columns - 1;
					clear_to_column = MIN(clear_to_column, last_column);

					for (int i = clear_from_colum; i <= clear_to_column; i++) {

						List<Track::PosEvent> events;
						song->get_events_in_range(current_pattern, Track::Pos(clear_from, i), Track::Pos(clear_to, i), &events);
						for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {

							if (E->get().event.type != song->get_event_column_type(i)) {
								continue;
							}

							Track::Event ev = Track::Event::make_empty(E->get().event.type);

							undo_redo->do_method(song, &Song::set_event, current_pattern, i, E->get().pos.tick, ev);
						}
					}
				}

				//paste
				{

					Tick paste_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();

					for (List<Track::PosEvent>::Element *E = clipboard.events.front(); E; E = E->next()) {

						int column = cursor.column + E->get().pos.column;

						if (column > last_column) {

							continue;
						}
						if (song->get_event_column_type(column) != E->get().event.type) {
							//different type, do nothing
							continue;
						}

						Tick tick = paste_from + E->get().pos.tick;

						undo_redo->do_method(song, &Song::set_event, current_pattern, column, tick, E->get().event);
						undo_redo->undo_method(song, &Song::set_event, current_pattern, column, tick, song->get_event(current_pattern, column, tick));
					}
				}

				//move everything down on insert
				if (p_bind == KeyBindings::PATTERN_SELECTION_PASTE_INSERT) {
					Tick move_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					Tick move_to = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;

					int move_from_colum = cursor.column;
					int move_to_column = cursor.column + clipboard.columns - 1;
					move_to_column = MIN(move_to_column, last_column);

					for (int i = move_from_colum; i <= move_to_column; i++) {

						if (move_from + clipboard.ticks >= move_to) {
							continue; //nothing to do
						}

						//erase

						List<Track::PosEvent> erase_events;
						song->get_events_in_range(current_pattern, Track::Pos(move_from + clipboard.ticks, i), Track::Pos(move_to, i), &erase_events);
						for (List<Track::PosEvent>::Element *E = erase_events.front(); E; E = E->next()) {
							Track::Event ev = Track::Event::make_empty(E->get().event.type);
							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, ev);
						}
						//move
						List<Track::PosEvent> move_events;
						song->get_events_in_range(current_pattern, Track::Pos(move_from, i), Track::Pos(move_to - clipboard.ticks, i), &move_events);
						for (List<Track::PosEvent>::Element *E = move_events.front(); E; E = E->next()) {
							undo_redo->do_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick + clipboard.ticks, E->get().event);
							Track::Event ev = Track::Event::make_empty(E->get().event.type);
							undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick + clipboard.ticks, ev);
						}
						//restore

						for (List<Track::PosEvent>::Element *E = erase_events.front(); E; E = E->next()) {
							undo_redo->undo_method(song, &Song::set_event, current_pattern, E->get().pos.column, E->get().pos.tick, E->get().event);
						}
					}
				}

				//clear space to paste (post)
				if (p_bind != KeyBindings::PATTERN_SELECTION_PASTE_MIX) {

					Tick clear_from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
					Tick clear_to = clear_from + clipboard.ticks;

					int clear_from_colum = cursor.column;
					int clear_to_column = cursor.column + clipboard.columns - 1;
					clear_to_column = MIN(clear_to_column, last_column);

					for (int i = clear_from_colum; i <= clear_to_column; i++) {

						List<Track::PosEvent> events;
						song->get_events_in_range(current_pattern, Track::Pos(clear_from, i), Track::Pos(clear_to, i), &events);
						for (List<Track::PosEvent>::Element *E = events.front(); E; E = E->next()) {

							if (E->get().event.type != song->get_event_column_type(i)) {
								continue;
							}

							undo_redo->undo_method(song, &Song::set_event, current_pattern, i, E->get().pos.tick, E->get().event);
						}
					}
				}

				undo_redo->do_method(this, &PatternEditor::_redraw);
				undo_redo->undo_method(this, &PatternEditor::_redraw);
				undo_redo->commit_action();

			} break;
		}

		int current_automation = song->get_event_column_automation(cursor.column);

		if (current_automation >= 0) {

			switch (p_bind) {

				case KeyBindings::AUTOMATION_RADIO_DISCRETE_ROWS: {

					Automation *a = song->get_track(current_track)
											->get_automation(current_automation);
					undo_redo->begin_action("Automation Display Numbers");
					undo_redo->do_method(a, &Automation::set_edit_mode,
							Automation::EDIT_ROWS_DISCRETE);
					undo_redo->undo_method(a, &Automation::set_edit_mode,
							a->get_edit_mode());
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();

				} break;
				case KeyBindings::AUTOMATION_RADIO_ENVELOPE_SMALL: {

					Automation *a = song->get_track(current_track)
											->get_automation(current_automation);
					undo_redo->begin_action("Automation Display Small");
					undo_redo->do_method(a, &Automation::set_edit_mode,
							Automation::EDIT_ENVELOPE_SMALL);
					undo_redo->undo_method(a, &Automation::set_edit_mode,
							a->get_edit_mode());
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();

				} break;
				case KeyBindings::AUTOMATION_RADIO_ENVELOPE_LARGE: {

					Automation *a = song->get_track(current_track)
											->get_automation(current_automation);
					undo_redo->begin_action("Automation Display Large");
					undo_redo->do_method(a, &Automation::set_edit_mode,
							Automation::EDIT_ENVELOPE_LARGE);
					undo_redo->undo_method(a, &Automation::set_edit_mode,
							a->get_edit_mode());
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();

				} break;
				case KeyBindings::AUTOMATION_MOVE_LEFT: {

					ERR_FAIL_COND(current_automation == 0);

					cursor.column -= 1;

					undo_redo->begin_action("Automation Move Left");
					undo_redo->do_method(song->get_track(current_track),
							&Track::swap_automations, current_automation,
							current_automation - 1);
					undo_redo->undo_method(song->get_track(current_track),
							&Track::swap_automations, current_automation,
							current_automation - 1);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();

				} break;
				case KeyBindings::AUTOMATION_MOVE_RIGHT: {

					ERR_FAIL_COND(current_automation == song->get_track(current_track)->get_automation_count() - 1);

					cursor.column += 1;

					undo_redo->begin_action("Automation Move Right");
					undo_redo->do_method(song->get_track(current_track),
							&Track::swap_automations, current_automation,
							current_automation + 1);
					undo_redo->undo_method(song->get_track(current_track),
							&Track::swap_automations, current_automation,
							current_automation + 1);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();

				} break;
				case KeyBindings::AUTOMATION_REMOVE: {

					Automation *a = song->get_track(current_track)
											->get_automation(current_automation);
					undo_redo->begin_action("Remove Automation");
					undo_redo->do_method(song->get_track(current_track),
							&Track::remove_automation, current_automation);
					undo_redo->undo_method(song->get_track(current_track),
							&Track::add_automation, a, current_automation);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->do_method(this, &PatternEditor::_validate_menus);
					undo_redo->undo_method(this, &PatternEditor::_validate_menus);
					undo_redo->commit_action();
				} break;
			}
		}
	}
}

void PatternEditor::_update_shift_selection() {

	bool prev_active = selection.active;
	selection.active = true;
	selection.begin_column = selection.shift_from_column;
	selection.begin_tick = selection.shift_from_row * TICKS_PER_BEAT / _get_rows_per_beat();
	selection.end_column = cursor.column;
	selection.end_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
	selection.row_tick_size = TICKS_PER_BEAT / _get_rows_per_beat();
	selection.shift_active = true;

	_validate_selection();
	if (prev_active != selection.active) {
		_validate_menus();
	}
}

bool PatternEditor::on_key_press_event(GdkEventKey *key_event) {

	bool shift_pressed = key_event->state & GDK_SHIFT_MASK;
	if (!shift_pressed) {
		//shift selection will always begin from a previous state

		selection.shift_active = false;
	}

	if (song->get_track_count() == 0) {
		return false;
	}

	if (!selection.shift_active) {

		selection.shift_from_column = cursor.column;
		selection.shift_from_row = cursor.row;
	}

	if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_UP)) {
		cursor.row -= cursor_advance;
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}
	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_DOWN)) {
		cursor.row += cursor_advance;
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}
	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_UP_1_ROW)) {
		cursor.row -= 1;
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_DOWN_1_ROW)) {
		cursor.row += 1;
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_PAGE_UP)) {
		cursor.row -= song->pattern_get_beats_per_bar(current_pattern) * _get_rows_per_beat();
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_PAGE_DOWN)) {
		cursor.row += song->pattern_get_beats_per_bar(current_pattern) * _get_rows_per_beat();
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_LEFT)) {

		if (cursor.field == 0) {
			if (cursor.column > 0) {
				cursor.column--;

				Track *track;
				int automation;
				int command;
				int column;
				get_cursor_column_data(&track, command, automation, column);
				ERR_FAIL_COND_V(!track, false);

				if (automation >= 0) {
					if (track->get_automation(automation)->get_edit_mode() !=
							Automation::EDIT_ROWS_DISCRETE) {
						cursor.field = 0;
					} else {
						cursor.field = 1;
					}
				} else if (command >= 0) {
					cursor.field = 2;
				} else { //note
					cursor.field = 3;
				}
				_validate_menus();
			}
		} else {
			cursor.field--;
		}
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_MOVE_RIGHT)) {

		Track *track;
		int automation;
		int column;
		int command;
		get_cursor_column_data(&track, command, automation, column);
		ERR_FAIL_COND_V(!track, false);

		int max_field = 1;

		if (automation >= 0) {
			if (track->get_automation(automation)->get_edit_mode() !=
					Automation::EDIT_ROWS_DISCRETE) {
				max_field = 0;
			} else {
				max_field = 1;
			}
		} else if (command >= 0) {
			max_field = 2;
		} else {
			max_field = 3;
		}

		if (cursor.field == max_field) {
			if (cursor.column < song->get_event_column_count() - 1) {
				cursor.column++;
				cursor.field = 0;
				_validate_menus();
			}
		} else {
			cursor.field++;
		}
		_validate_cursor();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_TAB)) {

		if (cursor.column < song->get_event_column_count() - 1) {
			cursor.column++;
			cursor.field = 0;
		}
		_validate_cursor();
		_validate_menus();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_BACKTAB)) {

		if (cursor.field > 0)
			cursor.field = 0;
		else if (cursor.column > 0) {
			cursor.column--;
		}
		_validate_cursor();
		_validate_menus();
	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_HOME)) {

		if (cursor.field != 0) {
			cursor.field = 0;
		} else {

			Track *track;
			int automation;
			int column;
			int command;
			get_cursor_column_data(&track, command, automation, column);
			ERR_FAIL_COND_V(!track, false);

			if (column != 0) {
				if (column >= 0) {
					cursor.column -= column;
				} else if (command >= 0) {
					cursor.column -= track->get_column_count() + command;
				} else {
					cursor.column -= automation + track->get_column_count() + track->get_command_column_count();
				}
			} else {

				if (cursor.column == 0) {
					cursor.row = 0;
				} else {
					cursor.column = 0;
				}
			}
		}
		_validate_cursor();
		_validate_menus();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind_noshift(key_event, KeyBindings::CURSOR_END)) {

		Track *track;
		int automation;
		int column;
		int command;
		get_cursor_column_data(&track, command, automation, column);
		ERR_FAIL_COND_V(!track, false);

		int pattern_w = song->get_event_column_count();

		int event_ofs;
		if (column >= 0) {
			event_ofs = column;
		} else if (command >= 0) {
			event_ofs = command + track->get_column_count();
		} else {
			event_ofs = automation + track->get_column_count() + track->get_command_column_count();
		}
		int event_total = track->get_event_column_count();

		if (event_ofs < event_total - 1) {
			cursor.column += event_total - event_ofs - 1;
			cursor.field = 0;
		} else if (cursor.column < pattern_w - 1) {
			cursor.column = pattern_w - 1;
			cursor.field = 0;
		} else {
			cursor.row = song->pattern_get_beats(current_pattern) * _get_rows_per_beat() - 1;
			cursor.field = 0;
		}

		_validate_cursor();
		_validate_menus();
		if (shift_pressed) {
			_update_shift_selection();
		}

	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_PAN_WINDOW_UP)) {

		if (v_offset > 0) {
			if (v_offset + visible_rows - 1 == cursor.row)
				cursor.row--;
			v_offset--;
			queue_draw();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_PAN_WINDOW_DOWN)) {

		if (v_offset + visible_rows < get_total_rows()) {
			if (cursor.row <= v_offset) {
				cursor.row++;
			}
			v_offset++;
			queue_draw();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_INSERT) || key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TRACK_INSERT)) {

		List<Track::PosEvent> events;

		Track::Pos from;
		from.tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
		from.column = cursor.column;

		Track::Pos to;
		to.tick = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;
		to.column = cursor.column;

		if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TRACK_INSERT)) {
			from.column = _cursor_get_track_begin_column();
			to.column = _cursor_get_track_end_column();
		}
		song->get_events_in_range(current_pattern, from, to, &events);

		if (events.size()) {

			undo_redo->begin_action("Insert", true);

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = Track::Event::make_empty(E->get().event.type);

				undo_redo->do_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, E->get().pos.tick, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->do_method(
						song, &Song::set_event, current_pattern, E->get().pos.column,
						E->get().pos.tick + TICKS_PER_BEAT / _get_rows_per_beat(), ev);
				ev = Track::Event::make_empty(ev.type);
				undo_redo->undo_method(
						song, &Song::set_event, current_pattern, E->get().pos.column,
						E->get().pos.tick + TICKS_PER_BEAT / _get_rows_per_beat(), ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, E->get().pos.tick, ev);
			}

			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_DELETE) || key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TRACK_DELETE)) {

		List<Track::PosEvent> events;

		Track::Pos from;
		from.tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
		from.column = cursor.column;

		Track::Pos to;
		to.tick = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;
		to.column = cursor.column;

		if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TRACK_DELETE)) {
			from.column = _cursor_get_track_begin_column();
			to.column = _cursor_get_track_end_column();
		}

		song->get_events_in_range(current_pattern, from, to, &events);

		if (events.size()) {

			undo_redo->begin_action("Delete", true);

			Tick limit = from.tick;

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = Track::Event::make_empty(E->get().event.type);

				undo_redo->do_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, E->get().pos.tick, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Tick new_ofs = E->get().pos.tick - TICKS_PER_BEAT / _get_rows_per_beat();

				if (new_ofs < limit)
					continue;

				Track::Event ev = E->get().event;
				undo_redo->do_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, new_ofs, ev);
				ev = Track::Event::make_empty(ev.type);
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, new_ofs, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						E->get().pos.column, E->get().pos.tick, ev);
			}

			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		}

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TOGGLE_VOLUME_MASK)) {

		Track *track;
		int automation;
		int column;
		int command;
		get_cursor_column_data(&track, command, automation, column);
		ERR_FAIL_COND_V(!track, false);

		if (column >= 0) {
			volume_mask_active = !volume_mask_active;
			volume_mask_changed.emit();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_COPY_VOLUME_MASK)) {

		Track *track;
		int automation;
		int column;
		int command;
		get_cursor_column_data(&track, command, automation, column);
		ERR_FAIL_COND_V(!track, false);

		if (column >= 0) {

			Track::Event ev =
					song->get_event(current_pattern, cursor.column,
							cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());

			if (ev.b != Track::Note::EMPTY) {
				volume_mask = ev.b;
				volume_mask_changed.emit();
			}
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_OCTAVE_LOWER)) {
		if (current_octave > 0) {
			current_octave--;
			octave_changed.emit();
		}

	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_OCTAVE_RAISE)) {
		if (current_octave < 8) {
			current_octave++;
			octave_changed.emit();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_PREV_PATTERN)) {
		if (current_pattern > 0) {
			current_pattern--;
			pattern_changed.emit();
			_validate_cursor();
			queue_draw();
		}

	} else if (key_bindings->is_keybind(key_event, KeyBindings::PATTERN_NEXT_PATTERN)) {
		if (current_pattern < Song::MAX_PATTERN - 1) {
			current_pattern++;
			pattern_changed.emit();
			_validate_cursor();
			queue_draw();
		}
	} else if (cursor.field == 0 && song->get_event_column_type(cursor.column) == Track::Event::TYPE_NOTE && key_bindings->is_keybind(key_event, KeyBindings::CURSOR_PLAY_NOTE)) {

		Tick from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
		Tick to = (cursor.row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

		song->play_event_range(current_pattern, cursor.column, cursor.column, from, to);

		_cursor_advance();
		_validate_cursor();

	} else if (cursor.field == 0 && song->get_event_column_type(cursor.column) == Track::Event::TYPE_NOTE && key_bindings->is_keybind(key_event, KeyBindings::CURSOR_PLAY_ROW)) {

		Tick from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
		Tick to = (cursor.row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

		song->play_event_range(current_pattern, 0, song->get_event_column_count() - 1, from, to);

		_cursor_advance();
		queue_draw();

	} else {

		//set step or zoom

		for (int i = 0; i < 10; i++) {
			if (key_bindings->is_keybind(key_event, KeyBindings::KeyBind(KeyBindings::CURSOR_ADVANCE_1 + i))) {
				cursor_advance = i + 1;
				step_changed.emit();
				return true;
			}
			if (key_bindings->is_keybind(key_event, KeyBindings::KeyBind(KeyBindings::CURSOR_ZOOM_1 + i))) {
				Tick cursor_on_tick = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
				int ofs_v = cursor.row - v_offset;
				beat_zoom = BeatZoom(i);
				cursor.row = cursor_on_tick * _get_rows_per_beat() / TICKS_PER_BEAT;
				v_offset = MAX(0, cursor.row - ofs_v);
				zoom_changed.emit();
				queue_draw();
				return true;
			}
		}

		// check field

		Track *track;
		int automation;
		int column;
		int command;
		get_cursor_column_data(&track, command, automation, column);
		ERR_FAIL_COND_V(!track, false);

		if (column >= 0) {

			if (cursor.field == 0) {
				// put a note
				for (int i = KeyBindings::PIANO_C0; i <= KeyBindings::PIANO_E2; i++) {
					if (key_bindings->is_keybind(key_event, KeyBindings::KeyBind(i))) {
						int note = i - KeyBindings::PIANO_C0;

						Track::Event ev =
								song->get_event(current_pattern, cursor.column,
										cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());

						if (volume_mask_active) {
							ev.b = volume_mask;
						}

						Track::Event old_ev = ev;
						ev.a = current_octave * 12 + note;
						if (ev.a >= 120) {
							ev.a = 119;
						}
						undo_redo->begin_action("Add Note");
						undo_redo->do_method(
								song, &Song::set_event, current_pattern, cursor.column,
								Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
						undo_redo->undo_method(
								song, &Song::set_event, current_pattern, cursor.column,
								Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
						undo_redo->do_method(this, &PatternEditor::_redraw);
						undo_redo->undo_method(this, &PatternEditor::_redraw);
						undo_redo->commit_action();

						{ //preview play

							Tick from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
							Tick to = (cursor.row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

							song->play_event_range(current_pattern, cursor.column, cursor.column, from, to);
						}

						_cursor_advance();
						_validate_cursor();

						return true;
					}
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::PATTERN_CURSOR_NOTE_OFF)) {

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());
					Track::Event old_ev = ev;
					ev.a = Track::Note::OFF;
					undo_redo->begin_action("Add Note Off");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();

					{ //preview play

						Tick from = cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
						Tick to = (cursor.row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

						song->play_event_range(current_pattern, cursor.column, cursor.column, from, to);
					}

					_cursor_advance();
					_validate_cursor();
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}

			} else if (cursor.field == 1) {
				// put octave
				if ((key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9)) {

					int octave = key_event->keyval - GDK_KEY_0;

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());
					Track::Event old_ev = ev;

					if (old_ev.a == Track::Note::EMPTY) {
						return true; // no add octave on empty
					}

					ev.a = (ev.a % 12) + octave * 12;

					undo_redo->begin_action("Set Octave");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					_cursor_advance();
					_validate_cursor();

					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}

			} else if (cursor.field == 2) {
				// put volume 1
				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {

					int vol_g = key_event->keyval - GDK_KEY_0;

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());
					Track::Event old_ev = ev;

					if (old_ev.b == Track::Note::EMPTY) {
						ev.b = 0;
					}

					ev.b = (ev.b % 10) + vol_g * 10;

					volume_mask = ev.b;
					volume_mask_changed.emit();

					undo_redo->begin_action("Set Volume");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					cursor.field = 3;

					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}

			} else if (cursor.field == 3) {
				// put volume 2
				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {

					int vol_l = key_event->keyval - GDK_KEY_0;

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());

					Track::Event old_ev = ev;

					if (old_ev.b == Track::Note::EMPTY) {
						ev.b = 0;
					}

					ev.b -= (ev.b % 10);
					ev.b += vol_l;

					volume_mask = ev.b;
					volume_mask_changed.emit();

					undo_redo->begin_action("Set Volume");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					cursor.field = 2;
					_cursor_advance();
					_validate_cursor();
					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}
			}
		} else if (command >= 0) {
			if (cursor.field == 0) {
				//command
				if ((key_event->keyval >= GDK_KEY_a && key_event->keyval <= GDK_KEY_z)) {

					int command = 'a' + (key_event->keyval - GDK_KEY_a);

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());
					Track::Event old_ev = ev;

					ev.a = command;

					undo_redo->begin_action("Set Command");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					_cursor_advance();
					_validate_cursor();

					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}
			} else {
				//parameter

				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {

					int param = key_event->keyval - GDK_KEY_0;

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());
					Track::Event old_ev = ev;

					if (cursor.field == 1) {
						ev.b = param * 10 + (ev.b % 10);
					} else {
						ev.b = ((ev.b / 10) % 10) * 10 + param;
					}

					undo_redo->begin_action("Set Parameter");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					if (cursor.field == 1) {
						cursor.field = 2;
					} else {
						cursor.field = 1;
						_cursor_advance();
						_validate_cursor();
					}

					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}
			}
		} else if (automation >= 0) {

			Automation *a = track->get_automation(automation);
			if (a->get_edit_mode() == Automation::EDIT_ROWS_DISCRETE) {

				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {
					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / _get_rows_per_beat());

					Track::Event old_ev = ev;

					if (ev.a == Automation::EMPTY) {
						ev.a = 0;
					}

					if (cursor.field == 0) {
						ev.a = (key_event->keyval - GDK_KEY_0) * 10 + ev.a % 10;
					} else {
						ev.a = (key_event->keyval - GDK_KEY_0) + (ev.a / 10) * 10;
					}

					undo_redo->begin_action("Set Automation Point");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / _get_rows_per_beat()), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					if (cursor.field == 0) {
						cursor.field = 1;
					} else {
						cursor.field = 0;
						_cursor_advance();
						_validate_cursor();
					}
				}
			}

			if (key_bindings->is_keybind(key_event,
						KeyBindings::CURSOR_FIELD_CLEAR)) {
				_field_clear();
			}
		}

		return false;
	}

	return true;
}

bool PatternEditor::on_key_release_event(GdkEventKey *key_event) {

	bool shift_pressed = key_event->state & GDK_SHIFT_MASK;
	if (!shift_pressed) {
		selection.shift_active = false;
	}

	return false;
}

Gtk::SizeRequestMode PatternEditor::get_request_mode_vfunc() const {
	// Accept the default value supplied by the base class.
	return Gtk::Widget::get_request_mode_vfunc();
}

// Discover the total amount of minimum space and natural space needed by
// this widget.
// Let's make this simple example widget always need minimum 60 by 50 and
// natural 100 by 70.
void PatternEditor::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {
	minimum_width = 64;
	natural_width = 64;
}

void PatternEditor::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = 64;
	natural_height = 64;
}

void PatternEditor::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = 64;
	natural_height = 64;
}

void PatternEditor::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = 64;
	natural_width = 64;
}

void PatternEditor::on_size_allocate(Gtk::Allocation &allocation) {
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

void PatternEditor::on_map() {
	// Call base class:
	Gtk::Widget::on_map();
}

void PatternEditor::on_unmap() {
	// Call base class:
	Gtk::Widget::on_unmap();
}

void PatternEditor::on_realize() {
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

void PatternEditor::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void PatternEditor::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
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

void PatternEditor::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void PatternEditor::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

void PatternEditor::_draw_arrow(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x + w / 4, y + h / 4);
	cr->line_to(x + w * 3 / 4, y + h / 4);
	cr->line_to(x + w / 2, y + h * 3 / 4);
	cr->line_to(x + w / 4, y + h / 4);
	cr->fill();
	cr->stroke();
}

void PatternEditor::_v_scroll_changed() {
	if (drawing) {
		return;
	}

	v_offset = v_scroll->get_value();
	queue_draw();
}

void PatternEditor::_h_scroll_changed() {

	if (drawing) {
		return;
	}

	int offset = h_scroll->get_value();

	h_offset = 0;
	while (true) {
		if (h_offset == song->get_event_column_count() - 1) {
			break;
		}
		if (offset < get_column_offset(h_offset)) {
			break;
		}
		h_offset++;
	}
	queue_draw();
}

int PatternEditor::get_column_offset(int p_column) {

	int offset = 0;
	int idx = 0;

	int track_sep = theme->constants[Theme::CONSTANT_PATTERN_EDITOR_TRACK_SEPARATION];

	for (int i = 0; i < song->get_track_count(); i++) {

		Track *t = song->get_track(i);
		for (int j = 0; j < t->get_column_count(); j++) {

			if (idx == p_column + 1) {
				return offset;
			}

			int cw = fw_cache * 6; //base column width
			if (j == 0) {
				cw += fh_cache; //name
			} else {
				cw += fw_cache; //separator
			}
			offset += cw;
			idx++;
		}

		for (int j = 0; j < t->get_command_column_count(); j++) {

			if (idx == p_column + 1) {
				return offset;
			}

			int cw = fw_cache * 4; //base column width
			offset += cw;
			idx++;
		}

		for (int j = 0; j < t->get_automation_count(); j++) {

			if (idx == p_column + 1) {
				return offset;
			}

			Automation *a = t->get_automation(j);

			int cw = fh_cache; //base column width
			switch (a->get_edit_mode()) {
				case Automation::EDIT_ROWS_DISCRETE: {
					cw += fw_cache * 2;
				} break;
				case Automation::EDIT_ENVELOPE_SMALL: {
					cw += fw_cache * 4;
				} break;
				case Automation::EDIT_ENVELOPE_LARGE: {
					cw += fw_cache * 8;
				} break;
			}

			idx++;

			offset += cw;
		}

		offset += track_sep;
	}

	return offset;
}

bool PatternEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {

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

	fw_cache = fw;
	fh_cache = fh;

	row_height_cache = fh;
	int top_ofs = 4;

	row_top_ofs = top_ofs;

	Gdk::RGBA track_sep_color =
			theme->colors[Theme::COLOR_PATTERN_EDITOR_TRACK_SEPARATOR];
	Gdk::RGBA cursorcol = theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR];
	int track_sep_w =
			theme->constants[Theme::CONSTANT_PATTERN_EDITOR_TRACK_SEPARATION];

	click_areas.clear();

	visible_rows = (h - top_ofs) / fh;

	int beats_per_bar = song->pattern_get_beats_per_bar(current_pattern);
	int pattern_length = song->pattern_get_beats(current_pattern) * _get_rows_per_beat();

	for (int i = 0; i < visible_rows; i++) {

		int row = v_offset + i;
		int beat = row / _get_rows_per_beat();
		int subbeat = row % _get_rows_per_beat();

		if (row >= pattern_length) {
			break;
		}

		bool is_playing = playback_pattern == current_pattern && playback_row == row;

		if (subbeat == 0 || i == 0) {
			if (beat % beats_per_bar == 0)
				_draw_text(cr, 0, top_ofs + i * fh + fa, String::num(beat),
						is_playing ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_BAR]);
			else
				_draw_text(cr, 0, top_ofs + i * fh + fa, String::num(beat),
						is_playing ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_BEAT]);

		} else {
			char text[3] = { '0' + (subbeat / 10), '0' + (subbeat % 10), 0 };
			_draw_text(cr, fw, top_ofs + i * fh + fa, text,
					is_playing ? theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR] : theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_SUB_BEAT]);
		}
	}

	int ofs = fw * 4;
	Gdk::RGBA bgcol = theme->colors[Theme::COLOR_PATTERN_EDITOR_BG];
	_draw_fill_rect(cr, ofs, 0, w - ofs, h, bgcol);

	int idx = 0;

	for (int i = 0; i < song->get_track_count(); i++) {

		Track *t = song->get_track(i);

		bool drawn = false;

		for (int j = 0; j < t->get_column_count(); j++) {

			if (idx < h_offset) {
				idx++;
				continue;
			}

			if (j == 0) {

				int as = 2;
				Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_TRACK_NAME];
				if (t->is_muted()) {
					c.set_alpha(c.get_alpha() * 0.5);
				}
				_draw_text(cr, ofs + fh - fa, top_ofs + as, t->get_name(), c, true);
				ofs += fh;
			}

			{
				// fill fields for click areas
				ClickArea ca;
				ca.column = idx;
				ClickArea::Field f;
				f.width = fw;
				f.x = ofs;
				ca.fields.push_back(f);
				f.x += fw * 2;
				ca.fields.push_back(f);
				f.x += fw * 2;
				ca.fields.push_back(f);
				f.x += fw;
				ca.fields.push_back(f);

				click_areas.push_back(ca);
			}

			drawn = true;
			int extrahl = (j < t->get_column_count() - 1) ? fw : 0;

			for (int k = 0; k < visible_rows; k++) {

				char rowstr[7] = { '.', '.', '.', ' ', '.', '.', 0 };

				int row = v_offset + k;

				if (row >= pattern_length) {
					break;
				}

				int beat = row / _get_rows_per_beat();
				int subbeat = row % _get_rows_per_beat();
				Tick from_tick = row * TICKS_PER_BEAT / _get_rows_per_beat();
				Tick to_tick = (row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

				bool bg_selected = _is_in_selection(idx, from_tick);

				Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];
				Gdk::RGBA csel = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_SELECTED];

				if (t->is_muted()) {
					c.set_alpha(c.get_alpha() * 0.5);
					csel.set_alpha(csel.get_alpha() * 0.5);
				}
				Gdk::RGBA bgc = bgcol;

				if (bg_selected) {
					bgc = theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_SELECTED];
					if (subbeat == 0 || k == 0) {
						if ((beat % beats_per_bar) == 0)
							_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 6 + extrahl,
									fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR_SELECTED]);
						else
							_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 6 + extrahl,
									fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT_SELECTED]);
					} else {
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 6 + extrahl,
								fh,
								bgc);
					}

				} else if (subbeat == 0 || k == 0) {
					if ((beat % beats_per_bar) == 0)
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 6 + extrahl,
								fh,
								theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR]);
					else
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 6 + extrahl,
								fh,
								theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
				}

				Track::Pos from, to;
				from.tick = from_tick;
				from.column = j;
				to.tick = to_tick;
				to.column = j;

				List<Track::PosNote> pn;

				t->get_notes_in_range(current_pattern, from, to, &pn);
				Vector<Track::PosNote> valid;
				for (List<Track::PosNote>::Element *E = pn.front(); E; E = E->next()) {

					if (E->get().pos.column != j || E->get().pos.tick >= to_tick)
						continue;
					valid.push_back(E->get());
				}

				if (valid.size() == 0) {

					_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, bg_selected ? csel : c);
				} else if (valid.size() == 1) {

					Track::PosNote n = pn.front()->get();

					if (_is_in_selection(idx, n.pos.tick)) {
						//in-selection
						c = csel;
					} else if (n.pos.tick != from.tick) {
						c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
						if (t->is_muted()) {
							c.set_alpha(c.get_alpha() * 0.5);
						}
					}
					if (n.note.note == Track::Note::OFF) {
						rowstr[0] = '=';
						rowstr[1] = '=';
						rowstr[2] = '=';
					} else if (n.note.note < 120) {
						static const char *note[12] = { "C-", "C#", "D-", "D#", "E-", "F-",
							"F#", "G-", "G#", "A-", "A#", "B-" };
						static const char octave[12] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
						rowstr[0] = note[n.note.note % 12][0];
						rowstr[1] = note[n.note.note % 12][1];
						rowstr[2] = octave[n.note.note / 12]; // octave
					}

					if (n.note.volume < 100) {
						rowstr[4] = '0' + n.note.volume / 10;
						rowstr[5] = '0' + n.note.volume % 10;
					}

					_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);

				} else {

					int base_x = ofs;
					int base_y = top_ofs + k * fh;

					for (int l = 0; l < valid.size(); l++) {

						int h = (fh - 2) / valid.size();
						int w = fw * 3 - 2;
						int vw = fw * 2 - 2;

						Gdk::RGBA col = c;

						if (_is_in_selection(idx, valid[l].pos.tick)) {
							col = csel;
						}

						if (valid[l].note.note < 120) {
							_draw_fill_rect(cr, base_x, base_y + h * l, fw * 3, h - 1, col);
							_draw_rect(cr, base_x + valid[l].note.note * w / 120,
									base_y + 1 + h * l, 2, h - 2, bgc);
						} else if (valid[l].note.note == Track::Note::OFF) {
							_draw_rect(cr, base_x + 0, base_y + 1 + h * l, fw * 3, 1, col);
							_draw_rect(cr, base_x, base_y + 1 + h * l + h - 2, fw * 3, 1, col);
						}

						if (valid[l].note.volume < 100) {
							_draw_fill_rect(cr, base_x + fw * 4, base_y + h * l, fw * 2,
									h - 1, col);
							_draw_rect(cr, base_x + fw * 4 + valid[l].note.volume * vw / 100,
									base_y + 1 + h * l, 2, h - 2, bgc);
						}
					}
				}

				if (has_focus() && idx == cursor.column && cursor.row == row) {

					int field_ofs[4] = { 0, 2, 4, 5 };
					int cursor_x = ofs + field_ofs[cursor.field] * fw;
					int cursor_y = top_ofs + k * fh - sep;
					_draw_rect(cr, cursor_x - 1, cursor_y - 1, fw + 1, fh + 1, cursorcol);
				}
			}

			if (j < t->get_column_count() - 1)
				ofs += fw; //

			ofs += fw * 6;
			idx++;
		}

		for (int j = 0; j < t->get_command_column_count(); j++) {

			if (idx < h_offset) {
				idx++;
				continue;
			}

			ofs += fw;

			{
				// fill fields for click areas
				ClickArea ca;
				ca.column = idx;
				ClickArea::Field f;
				f.width = fw;
				f.x = ofs;
				ca.fields.push_back(f);
				f.x += fw;
				ca.fields.push_back(f);
				f.x += fw;
				ca.fields.push_back(f);

				click_areas.push_back(ca);
			}

			drawn = true;
			int extrahl = (j < t->get_column_count() - 1) ? fw : 0;

			for (int k = 0; k < visible_rows; k++) {

				char rowstr[4] = { '.', '0', '0', 0 };

				int row = v_offset + k;

				if (row >= pattern_length) {
					break;
				}

				int beat = row / _get_rows_per_beat();
				int subbeat = row % _get_rows_per_beat();
				Tick from_tick = row * TICKS_PER_BEAT / _get_rows_per_beat();
				Tick to_tick = (row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

				bool bg_selected = _is_in_selection(idx, from_tick);

				Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];
				Gdk::RGBA csel = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_SELECTED];

				if (t->is_muted()) {
					c.set_alpha(c.get_alpha() * 0.5);
					csel.set_alpha(csel.get_alpha() * 0.5);
				}
				Gdk::RGBA bgc = bgcol;

				if (bg_selected) {
					bgc = theme->colors[Theme::COLOR_PATTERN_EDITOR_BG];
					if (subbeat == 0 || k == 0) {
						if ((beat % beats_per_bar) == 0)
							_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 3 + extrahl,
									fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR_SELECTED]);
						else
							_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 3 + extrahl,
									fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT_SELECTED]);
					} else {
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 3 + extrahl,
								fh,
								bgc);
					}

				} else if (subbeat == 0 || k == 0) {
					if ((beat % beats_per_bar) == 0)
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 3 + extrahl,
								fh,
								theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BAR]);
					else
						_draw_fill_rect(cr, ofs, top_ofs + k * fh - sep, fw * 3 + extrahl,
								fh,
								theme->colors[Theme::COLOR_PATTERN_EDITOR_HL_BEAT]);
				}

				Track::Pos from, to;
				from.tick = from_tick;
				from.column = j;
				to.tick = to_tick;
				to.column = j;

				List<Track::PosCommand> pc;

				t->get_commands_in_range(current_pattern, from, to, &pc);
				Vector<Track::PosCommand> valid;
				for (List<Track::PosCommand>::Element *E = pc.front(); E; E = E->next()) {

					if (E->get().pos.column != j || E->get().pos.tick >= to_tick)
						continue;
					valid.push_back(E->get());
				}

				if (valid.size() == 0) {

					_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, bg_selected ? csel : c);
				} else if (valid.size() == 1) {

					Track::PosCommand n = pc.front()->get();

					if (_is_in_selection(idx, n.pos.tick)) {
						//in-selection
						c = csel;
					} else if (n.pos.tick != from.tick) {
						c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
						if (t->is_muted()) {
							c.set_alpha(c.get_alpha() * 0.5);
						}
					}
					if (n.command.command == Track::Command::EMPTY) {
						rowstr[0] = '.';
					} else if (n.command.command < 120) {
						rowstr[0] = n.command.command; //its an actual ascii!
						if (rowstr[0] >= 'a' && rowstr[0] <= 'z') {
							rowstr[0] = 'A' + (rowstr[0] - 'a'); //uppercase it
						}
					}

					if (n.command.parameter < 100) { //should be always true but..
						rowstr[1] = '0' + n.command.parameter / 10;
						rowstr[2] = '0' + n.command.parameter % 10;
					}

					_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);

				} else {

					int base_x = ofs;
					int base_y = top_ofs + k * fh;

					for (int l = 0; l < valid.size(); l++) {

						int h = (fh - 2) / valid.size();
						int w = fw * 3 - 2;
						int vw = fw * 2 - 2;

						Gdk::RGBA col = c;

						if (_is_in_selection(idx, valid[l].pos.tick)) {
							col = csel;
						}

						if (valid[l].command.parameter < 120) {
							_draw_fill_rect(cr, base_x, base_y + h * l, fw * 3, h - 1, col);
							_draw_rect(cr, base_x + valid[l].command.command * w / 99,
									base_y + 1 + h * l, 2, h - 2, bgc);
						}
					}
				}

				if (has_focus() && idx == cursor.column && cursor.row == row) {

					int field_ofs[4] = { 0, 1, 2 };
					int cursor_x = ofs + field_ofs[cursor.field] * fw;
					int cursor_y = top_ofs + k * fh - sep;
					_draw_rect(cr, cursor_x - 1, cursor_y - 1, fw + 1, fh + 1, cursorcol);
				}
			}

			ofs += fw * 3;
			idx++;
		}

		for (int j = 0; j < t->get_automation_count(); j++) {

			Automation *a = t->get_automation(j);

			if (idx < h_offset) {
				idx++;
				continue;
			}

			{

				int as = 2;

				Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_NAME];

				if (t->is_muted()) {
					c.set_alpha(c.get_alpha() * 0.5);
				}

				_draw_text(cr, ofs + fh - fa, top_ofs + as,
						a->get_control_port()->get_name().utf8().get_data(), c,
						true);
			}
			ofs += fh;

			switch (a->get_edit_mode()) {
				case Automation::EDIT_ROWS_DISCRETE: {

					{
						// fill fields for click areas
						ClickArea ca;
						ca.column = idx;
						ClickArea::Field f;
						f.width = fw;
						f.x = ofs;
						ca.fields.push_back(f);
						f.x += fw;
						ca.fields.push_back(f);
						click_areas.push_back(ca);
					}

					for (int k = 0; k < visible_rows; k++) {

						char rowstr[3] = { '.', '.', 0 };

						int row = v_offset + k;

						if (row >= pattern_length) {
							break;
						}

						int beat = row / _get_rows_per_beat();
						int subbeat = row % _get_rows_per_beat();
						Tick from = row * TICKS_PER_BEAT / _get_rows_per_beat();
						Tick to = (row + 1) * TICKS_PER_BEAT / _get_rows_per_beat();

						bool bg_selected = _is_in_selection(idx, from);

						Gdk::RGBA c =
								theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE];
						Gdk::RGBA csel = theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_SELECTED];

						if (t->is_muted()) {
							c.set_alpha(c.get_alpha() * 0.5);
							csel.set_alpha(csel.get_alpha() * 0.5);
						}

						Gdk::RGBA bgc = bgcol;

						if (bg_selected) {
							bgc = theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_SELECTED];

							if (subbeat == 0 || k == 0) {
								if ((beat % beats_per_bar) == 0)
									_draw_fill_rect(
											cr, ofs, top_ofs + k * fh - sep, fw * 2, fh,
											theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR_SELECTED]);
								else
									_draw_fill_rect(
											cr, ofs, top_ofs + k * fh - sep, fw * 2, fh,
											theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT_SELECTED]);
							} else {
								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, fw * 2, fh, bgc);
							}
						} else if (subbeat == 0 || k == 0) {
							if ((beat % beats_per_bar) == 0)
								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, fw * 2, fh,
										theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR]);
							else
								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, fw * 2, fh,
										theme
												->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT]);
						}

						int first;
						int count;

						a->get_points_in_range(current_pattern, from, to, first, count);

						if (count == 0) {

							_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, bg_selected ? csel : c);
						} else if (count == 1) {

							int val = a->get_point_by_index(current_pattern, first);

							Tick point_tick = a->get_point_tick_by_index(current_pattern, first);
							if (_is_in_selection(idx, point_tick)) {
								c = csel;
							} else if (point_tick != from) {
								c = theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_NOFIT];
								if (t->is_muted()) {
									c.set_alpha(c.get_alpha() * 0.5);
								}
							}

							rowstr[0] = '0' + val / 10;
							rowstr[1] = '0' + val % 10;

							_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);

						} else {

							int base_x = ofs;
							int base_y = top_ofs + k * fh;

							for (int l = 0; l < count; l++) {

								int h = (fh - 2) / count;
								int w = fw * 2 - 2;
								Tick point_tick = a->get_point_tick_by_index(current_pattern, first + l);

								int val = a->get_point_by_index(current_pattern, first + l);
								Gdk::RGBA col;
								if (_is_in_selection(idx, point_tick)) {
									col = csel;
								} else {
									col = c;
								}

								_draw_fill_rect(cr, base_x, base_y + h * l, fw * 2, h - 1, col);
								_draw_rect(cr, base_x + val * w / Automation::VALUE_MAX,
										base_y + 1 + h * l, 2, h - 2, bgc);
							}
						}

						if (has_focus() && idx == cursor.column && cursor.row == row) {

							int cursor_pos_x = ofs + cursor.field * fw;
							int cursor_pos_y = top_ofs + k * fh - sep;
							_draw_rect(cr, cursor_pos_x - 1, cursor_pos_y - 1, fw + 1, fh + 1,
									cursorcol);
						}
					}

					ofs += fw * 2;
				} break;
				case Automation::EDIT_ENVELOPE_SMALL:
				case Automation::EDIT_ENVELOPE_LARGE: {

					int w = a->get_edit_mode() == Automation::EDIT_ENVELOPE_SMALL ? 4 : 8;
					w *= fw;

					Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE];
					Gdk::RGBA csel = theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_SELECTED];

					if (t->is_muted()) {
						c.set_alpha(c.get_alpha() * 0.5);
						csel.set_alpha(csel.get_alpha() * 0.5);
					}

					Gdk::RGBA cpoint =
							theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_POINT];

					if (t->is_muted()) {
						cpoint.set_alpha(cpoint.get_alpha() * 0.5);
					}

					// fill fields for click areas
					ClickArea ca;
					ca.column = idx;
					ClickArea::Field f;
					f.width = w;
					f.x = ofs;
					ca.fields.push_back(f);

					for (int k = 0; k < visible_rows; k++) {

						int row = v_offset + k;

						if (row >= pattern_length) {
							break;
						}

						int beat = row / _get_rows_per_beat();
						int subbeat = row % _get_rows_per_beat();
						Tick from = row * TICKS_PER_BEAT / _get_rows_per_beat();
						bool bg_selected = _is_in_selection(idx, from);

						bool draw_outline = false;

						if (bg_selected) {

							if (subbeat == 0 || k == 0) {
								if ((beat % beats_per_bar) == 0)
									_draw_fill_rect(
											cr, ofs, top_ofs + k * fh - sep, w, fh,
											theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR_SELECTED]);
								else
									_draw_fill_rect(
											cr, ofs, top_ofs + k * fh - sep, w, fh,
											theme
													->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT_SELECTED]);
							} else {

								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, w, fh,
										theme->colors[Theme::COLOR_PATTERN_EDITOR_BG_SELECTED]);
								draw_outline = true;
							}

						} else if (subbeat == 0 || k == 0) {

							if ((beat % beats_per_bar) == 0)
								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, w, fh,
										theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR]);
							else
								_draw_fill_rect(
										cr, ofs, top_ofs + k * fh - sep, w, fh,
										theme
												->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT]);
						} else {
							draw_outline = true;
						}

						if (draw_outline) {

							Gdk::RGBA col = bg_selected ? theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT_SELECTED] : theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT];
							_draw_fill_rect(
									cr, ofs, top_ofs + k * fh - sep + fh - 1, w, 1,
									col);
							_draw_fill_rect(
									cr, ofs, top_ofs + k * fh - sep, 1, fh,
									col);
							_draw_fill_rect(
									cr, ofs + w - 1, top_ofs + k * fh - sep, 1, fh,
									col);
						}

						float prev = -1;
						for (int l = 0; l < fh; l++) {

							Tick at = (fh * row + l) * (TICKS_PER_BEAT / _get_rows_per_beat()) / fh;
							bool selected = _is_in_selection(idx, at);

							float tofs = a->interpolate_offset(current_pattern, at);
							if (prev == -1)
								prev = tofs;

							if (tofs >= 0)
								_draw_fill_rect(cr, ofs + tofs * w, top_ofs + k * fh - sep + l, 2,
										1, selected ? csel : c);

							prev = tofs;
						}

						if (has_focus() && idx == cursor.column && cursor.row == row) {

							int cursor_ofs_x = ofs;
							int cursor_ofs_y = top_ofs + k * fh - sep;

							_draw_rect(cr, cursor_ofs_x - 1, cursor_ofs_y - 1, w + 1, fh + 1,
									cursorcol);
						}
					}

					Tick pfrom = (v_offset) * (TICKS_PER_BEAT / _get_rows_per_beat());
					Tick pto = (v_offset + visible_rows) * (TICKS_PER_BEAT / _get_rows_per_beat());
					int first;
					int count;
					a->get_points_in_range(current_pattern, pfrom, pto, first, count);
					ca.automation = a;

					for (int l = first; l < first + count; l++) {
						int x = (a->get_point_by_index(current_pattern, l) * w /
								 Automation::VALUE_MAX);
						int y = a->get_point_tick_by_index(current_pattern, l) * fh /
										(TICKS_PER_BEAT / _get_rows_per_beat()) -
								v_offset * fh;
						_draw_fill_rect(cr, ofs + x - 2, top_ofs + y - 2 - sep, 5, 5, cpoint);

						ClickArea::AutomationPoint ap;
						ap.tick = a->get_point_tick_by_index(current_pattern, l);
						ap.x = ofs + x;
						ap.y = top_ofs + y /*- sep*/;
						ap.index = l;
						ca.automation_points.push_back(ap);
					}

					click_areas.push_back(ca);

					ofs += w;
				} break;
			}

			idx++;
		}

		if (drawn) {

			_draw_fill_rect(cr, ofs, 0, track_sep_w, h, track_sep_color);
			ofs += track_sep_w;
		}
	}

	if (has_focus()) {
		cr->set_source_rgba(1, 1, 1, 1);
		cr->rectangle(0, 0, w, h);
	}

	cr->stroke();

	v_scroll->set_upper(song->pattern_get_beats(current_pattern) * _get_rows_per_beat());
	v_scroll->set_page_size(visible_rows);
	v_scroll->set_value(v_offset);

	{
		int total = get_column_offset(song->get_event_column_count());
		h_scroll->set_upper(total);
		h_scroll->set_page_size(w - fw * 4);
		int hofs = h_offset == 0 ? 0 : get_column_offset(h_offset - 1);
		//printf("total %i, pagesize %i, offset %i\n", total, w - fw * 4, hofs);
		h_scroll->set_value(hofs);
	}

	drawing = false;

#if 0
	const double scale_x = (double)allocation.get_width() / ;
	const double scale_y = (double)allocation.get_height() / m_scale;

	// paint the background
	refStyleContext->render_background(cr,
			allocation.get_x(), allocation.get_y(),
			allocation.get_width(), allocation.get_height());

	Gdk::Cairo::set_source_rgba(cr, refStyleContext->get_color(state));
	cr->move_to(155. * scale_x, 165. * scale_y);
	cr->line_to(155. * scale_x, 838. * scale_y);
	cr->line_to(265. * scale_x, 900. * scale_y);
	cr->line_to(849. * scale_x, 564. * scale_y);
	cr->line_to(849. * scale_x, 438. * scale_y);
	cr->line_to(265. * scale_x, 100. * scale_y);
	cr->line_to(155. * scale_x, 165. * scale_y);
	cr->move_to(265. * scale_x, 100. * scale_y);
	cr->line_to(265. * scale_x, 652. * scale_y);
	cr->line_to(526. * scale_x, 502. * scale_y);
	cr->move_to(369. * scale_x, 411. * scale_y);
	cr->line_to(633. * scale_x, 564. * scale_y);
	cr->move_to(369. * scale_x, 286. * scale_y);
	cr->line_to(369. * scale_x, 592. * scale_y);
	cr->move_to(369. * scale_x, 286. * scale_y);
	cr->line_to(849. * scale_x, 564. * scale_y);
	cr->move_to(633. * scale_x, 564. * scale_y);
	cr->line_to(155. * scale_x, 838. * scale_y);
	cr->storke();
#endif
	return false;
}

void PatternEditor::set_beat_zoom(BeatZoom p_zoom) {
	beat_zoom = p_zoom;
	queue_draw();
}
PatternEditor::BeatZoom PatternEditor::get_beat_zoom() const {
	return beat_zoom;
}

int PatternEditor::get_current_track() const {

	return song->get_event_column_track(cursor.column);
}

void PatternEditor::set_playback_cursor(int p_pattern, Tick p_tick) {
	int cursor_row = p_tick * _get_rows_per_beat() / TICKS_PER_BEAT;
	if (cursor_row != cursor.row && p_pattern != current_pattern) {
		cursor.row = cursor_row;
		current_pattern = p_pattern;
		_validate_cursor();
		queue_draw();
	}
}
Tick PatternEditor::get_cursor_tick() const {
	return cursor.row * TICKS_PER_BEAT / _get_rows_per_beat();
}

void PatternEditor::set_hscroll(Glib::RefPtr<Gtk::Adjustment> p_h_scroll) {
	h_scroll = p_h_scroll;
	h_scroll->signal_value_changed().connect(sigc::mem_fun(*this, &PatternEditor::_h_scroll_changed));
}
void PatternEditor::set_vscroll(Glib::RefPtr<Gtk::Adjustment> p_v_scroll) {
	v_scroll = p_v_scroll;
	v_scroll->signal_value_changed().connect(sigc::mem_fun(*this, &PatternEditor::_v_scroll_changed));
}

void PatternEditor::redraw_and_validate_cursor() {
	_validate_cursor();
	queue_draw();
}

void PatternEditor::set_playback_pos(int p_pattern, Tick p_tick) {
	int cursor_row = p_tick * _get_rows_per_beat() / TICKS_PER_BEAT;

	if (playback_pattern != p_pattern || playback_row != cursor_row) {
		playback_pattern = p_pattern;
		playback_row = cursor_row;
		queue_draw();
	}
}

void PatternEditor::on_parsing_error(
		const Glib::RefPtr<const Gtk::CssSection> &section,
		const Glib::Error &error) {}
PatternEditor::PatternEditor(Song *p_song, UndoRedo *p_undo_redo,
		Theme *p_theme, KeyBindings *p_bindings) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("pattern_editor"),
		Gtk::Widget() {

	// This shows the GType name, which must be used in the CSS file.
	// std::cout << "GType name: " << G_OBJECT_TYPE_NAME(gobj()) << std::endl;

	// This shows that the GType still derives from GtkWidget:
	// std::cout << "Gtype is a GtkWidget?:" << GTK_IS_WIDGET(gobj()) <<
	// std::endl;

	song = p_song;
	undo_redo = p_undo_redo;
	key_bindings = p_bindings;
	theme = p_theme;
	set_has_window(true);
	set_can_focus(true);
	set_focus_on_click(true);
	// Gives Exposure & Button presses to the widget.

	set_name("pattern_editor");

	v_offset = 0;
	h_offset = 0;

	current_pattern = 0;
	beat_zoom = BEAT_ZOOM_4;
	volume_mask = 99;
	volume_mask_active = true;

	visible_rows = 4;
	current_octave = 5;
	cursor_advance = 1;

	cursor.row = 0;
	cursor.field = 0;
	cursor.column = 0;

	selection.active = false;
	selection.shift_active = false;
	selection.mouse_drag_active = false;
	clipboard.active = false;
#if 0
	Track *t = new Track;
	t->set_columns(2);
	song->add_track(t);

	t->set_note(0, Track::Pos(0, 0), Track::Note(60, 99));
	t->set_note(0, Track::Pos(TICKS_PER_BEAT, 1),
			Track::Note(88, Track::Note::EMPTY));
	t->set_note(0, Track::Pos(TICKS_PER_BEAT * 4, 1),
			Track::Note(Track::Note::OFF));
	t->set_note(0, Track::Pos(66, 1), Track::Note(63, 33));
	t->set_note(0, Track::Pos(TICKS_PER_BEAT * 3, 0), Track::Note(28, 80));
	t->set_note(0, Track::Pos(TICKS_PER_BEAT * 3 + 1, 0),
			Track::Note(Track::Note::OFF, 30));
	t->add_automation(new Automation(t->get_volume_port()));
	t->add_automation(new Automation(t->get_swing_port()));
	t->add_automation(new Automation(t->get_pan_port()));
	t->get_automation(0)->set_display_mode(Automation::DISPLAY_ROWS);
	t->get_automation(0)->set_point(0, 0, 160);
	t->get_automation(0)->set_point(0, TICKS_PER_BEAT * 3 + 2, 22);
	t->get_automation(0)->set_point(0, TICKS_PER_BEAT * 5, 22);
	t->get_automation(0)->set_point(0, TICKS_PER_BEAT * 5 + 2, 80);
	t->get_automation(0)->set_point(0, TICKS_PER_BEAT * 5 + 4, 44);
	t->get_automation(1)->set_display_mode(Automation::DISPLAY_SMALL);
	t->get_automation(2)->set_display_mode(Automation::DISPLAY_LARGE);
	t->get_automation(2)->set_point(0, 0, 22);
	t->get_automation(2)->set_point(0, TICKS_PER_BEAT * 2, 22);
	t->get_automation(2)->set_point(0, TICKS_PER_BEAT * 3, 44);
#endif
	grabbing_point = -1;
	fw_cache = 0;
	fh_cache = 0;
	last_amplify_value = 100;
	last_scale_value = 1.0;

	playback_pattern = -1;
	playback_row = -1;

	p_bindings->action_activated.connect(sigc::mem_fun(*this, &PatternEditor::_on_action_activated));
}

PatternEditor::~PatternEditor() {
}
