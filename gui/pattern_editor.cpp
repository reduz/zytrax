#include "pattern_editor.h"

void PatternEditor::_cursor_advance() {

	cursor.row += cursor_advance;
	if (cursor.row >= get_total_rows() - 1)
		cursor.row = get_total_rows() - 1;
}

void PatternEditor::_add_option_to_menu(Gtk::Menu *menu, const char *p_text,
		int p_menu,
		Vector<Gtk::MenuItem *> &items) {

	Gtk::MenuItem *item = new Gtk::MenuItem;
	item->set_label(p_text);
	item->signal_activate().connect(
			sigc::bind(sigc::mem_fun(this, &PatternEditor::_menu_option), p_menu));
	item->show();
	menu->append(*item);
	items.push_back(item);
}

void PatternEditor::_add_check_option_to_menu(Gtk::Menu *menu, bool p_checked,
		bool p_radio, const char *p_text,
		int p_menu,
		Vector<Gtk::MenuItem *> &items) {

	Gtk::CheckMenuItem *item = new Gtk::CheckMenuItem;
	item->set_label(p_text);
	item->show();
	item->set_active(p_checked);
	item->set_draw_as_radio(p_radio);
	item->signal_activate().connect(
			sigc::bind(sigc::mem_fun(this, &PatternEditor::_menu_option), p_menu));
	menu->append(*item);
	items.push_back(item);
}

void PatternEditor::_add_separator_to_menu(Gtk::Menu *menu,
		Vector<Gtk::MenuItem *> &items, const String &p_text) {

	Gtk::SeparatorMenuItem *sep = new Gtk::SeparatorMenuItem;
	if (p_text != String())
		sep->set_label(p_text.utf8().get_data());
	sep->show();
	menu->append(*sep);
	items.push_back(sep);
}

void PatternEditor::_menu_option(int p_option) {

	switch (p_option) {

		case TRACK_MENU_ADD_COLUMN: {

			undo_redo->begin_action("Add Column");
			undo_redo->do_method(
					song->get_track(current_menu_track), &Track::set_columns,
					song->get_track(current_menu_track)->get_column_count() + 1);
			undo_redo->undo_method(
					song->get_track(current_menu_track), &Track::set_columns,
					song->get_track(current_menu_track)->get_column_count());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		} break;
		case TRACK_MENU_REMOVE_COLUMN: {

			undo_redo->begin_action("Remove Column");
			undo_redo->do_method(
					song->get_track(current_menu_track), &Track::set_columns,
					song->get_track(current_menu_track)->get_column_count() - 1);
			undo_redo->undo_method(
					song->get_track(current_menu_track), &Track::set_columns,
					song->get_track(current_menu_track)->get_column_count());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case TRACK_MENU_SOLO: {

			undo_redo->begin_action("Solo");
			for (int i = 0; i < song->get_track_count(); i++) {

				undo_redo->do_method(song->get_track(i), &Track::set_muted,
						i != current_menu_track);
				undo_redo->undo_method(song->get_track(i), &Track::set_muted,
						song->get_track(i)->is_muted());
			}
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case TRACK_MENU_MUTE: {

			undo_redo->begin_action("Mute");
			undo_redo->do_method(song->get_track(current_menu_track), &Track::set_muted,
					!song->get_track(current_menu_track)->is_muted());
			undo_redo->undo_method(song->get_track(current_menu_track),
					&Track::set_muted,
					song->get_track(current_menu_track)->is_muted());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case TRACK_MENU_EDIT_AUTOMATIONS: {

		} break;
		case TRACK_MENU_RENAME: {

		} break;
		case TRACK_MENU_MOVE_LEFT: {

			undo_redo->begin_action("Track Move Left");
			undo_redo->do_method(song, &Song::swap_tracks, current_menu_track,
					current_menu_track - 1);
			undo_redo->undo_method(song, &Song::swap_tracks, current_menu_track,
					current_menu_track - 1);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case TRACK_MENU_MOVE_RIGHT: {

			undo_redo->begin_action("Track Move Right");
			undo_redo->do_method(song, &Song::swap_tracks, current_menu_track,
					current_menu_track + 1);
			undo_redo->undo_method(song, &Song::swap_tracks, current_menu_track,
					current_menu_track + 1);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case TRACK_MENU_REMOVE: {

			undo_redo->begin_action("Remove");
			undo_redo->do_method(song, &Song::remove_track, current_menu_track);
			undo_redo->undo_method(song, &Song::add_track_at_pos,
					song->get_track(current_menu_track),
					current_menu_track);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->undo_data(song->get_track(current_menu_track));
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_VISIBLE: {

			Automation *a = song->get_track(current_menu_track)
									->get_automation(current_menu_automation);
			undo_redo->begin_action("Toggle Automation Visibility");
			undo_redo->do_method(a, &Automation::set_visible, !a->is_visible());
			undo_redo->undo_method(a, &Automation::set_visible, a->is_visible());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_MODE_ROWS: {

			Automation *a = song->get_track(current_menu_track)
									->get_automation(current_menu_automation);
			undo_redo->begin_action("Automation Display Numbers");
			undo_redo->do_method(a, &Automation::set_display_mode,
					Automation::DISPLAY_ROWS);
			undo_redo->undo_method(a, &Automation::set_display_mode,
					a->get_display_mode());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_MODE_SMALL: {

			Automation *a = song->get_track(current_menu_track)
									->get_automation(current_menu_automation);
			undo_redo->begin_action("Automation Display Small");
			undo_redo->do_method(a, &Automation::set_display_mode,
					Automation::DISPLAY_SMALL);
			undo_redo->undo_method(a, &Automation::set_display_mode,
					a->get_display_mode());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_MODE_LARGE: {

			Automation *a = song->get_track(current_menu_track)
									->get_automation(current_menu_automation);
			undo_redo->begin_action("Automation Display Large");
			undo_redo->do_method(a, &Automation::set_display_mode,
					Automation::DISPLAY_LARGE);
			undo_redo->undo_method(a, &Automation::set_display_mode,
					a->get_display_mode());
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_MOVE_LEFT: {

			undo_redo->begin_action("Automation Move Left");
			undo_redo->do_method(song->get_track(current_menu_track),
					&Track::swap_automations, current_menu_automation,
					current_menu_automation - 1);
			undo_redo->undo_method(song->get_track(current_menu_track),
					&Track::swap_automations, current_menu_automation,
					current_menu_automation - 1);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_MOVE_RIGHT: {

			undo_redo->begin_action("Automation Move Right");
			undo_redo->do_method(song->get_track(current_menu_track),
					&Track::swap_automations, current_menu_automation,
					current_menu_automation + 1);
			undo_redo->undo_method(song->get_track(current_menu_track),
					&Track::swap_automations, current_menu_automation,
					current_menu_automation + 1);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
		case AUTOMATION_MENU_REMOVE: {

			Automation *a = song->get_track(current_menu_track)
									->get_automation(current_menu_automation);
			undo_redo->begin_action("Remove Automation");
			undo_redo->do_method(song->get_track(current_menu_track),
					&Track::remove_automation, current_menu_automation);
			undo_redo->undo_method(song->get_track(current_menu_track),
					&Track::add_automation, a, current_menu_automation);
			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();

		} break;
	}
}

void PatternEditor::_field_clear() {

	Track::Pos from;
	from.tick = cursor.row * TICKS_PER_BEAT / rows_per_beat;
	from.column = cursor.column;

	Track::Pos to;
	to.tick = from.tick + TICKS_PER_BEAT / rows_per_beat;
	to.column = cursor.column;

	List<Track::PosEvent> events;

	song->get_events_in_range(current_pattern, from, to, &events);

	if (events.size() == 0) {
		_cursor_advance();
		queue_draw();
		return;
	}

	if (cursor.field == 0 || cursor.field == 1) { // just clear whathever

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
}

void PatternEditor::_validate_cursor() {

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

	queue_draw();
}

int PatternEditor::get_total_rows() const {

	return song->pattern_get_beats(current_pattern) * rows_per_beat;
}

int PatternEditor::get_visible_rows() const {
	return visible_rows;
}

void PatternEditor::get_cursor_column_data(Track **r_track, int &r_automation,
		int &r_track_column) {

	int cc = cursor.column;

	r_automation = -1;
	r_track_column = -1;
	*r_track = NULL;

	for (int i = 0; i < song->get_track_count(); i++) {

		Track *t = song->get_track(i);
		*r_track = t;
		r_automation = -1;
		r_track_column = -1;

		for (int j = 0; j < t->get_column_count(); j++) {

			r_track_column = j;
			if (cc == 0) {
				return;
			}
			cc--;
		}

		r_track_column = -1;

		for (int j = 0; j < t->get_automation_count(); j++) {

			if (!t->get_automation(i)->is_visible())
				continue;

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

void PatternEditor::_redraw() {
	queue_draw();
}

void PatternEditor::_mouse_button_event(GdkEventButton *event, bool p_press) {

	Gdk::Rectangle posr(event->x, event->y, 1, 1);

	if (p_press && event->button == 1) {

		// test trackbuttons
		for (List<TrackButton>::Element *E = track_buttons.front(); E;
				E = E->next()) {

			if (E->get().r.intersects(posr)) {
				Track *track = song->get_track(E->get().track);

				if (track_menu) {
					// I dont understand how to delete all the items, great usability
					// there... so i create a new one
					delete track_menu;
					for (int i = 0; i < track_menu_items.size(); i++) {
						delete track_menu_items[i];
					}
					track_menu_items.clear();
				}

				track_menu = new Gtk::Menu;

				if (track->get_column_count() < 16) {
					_add_option_to_menu(track_menu, "Add Column", TRACK_MENU_ADD_COLUMN,
							track_menu_items);
				}
				if (track->get_column_count() > 1) {

					_add_option_to_menu(track_menu, "Remove Column",
							TRACK_MENU_REMOVE_COLUMN, track_menu_items);
				}

				_add_separator_to_menu(track_menu, track_menu_items);

				_add_option_to_menu(track_menu, "Solo", TRACK_MENU_SOLO,
						track_menu_items);
				_add_option_to_menu(track_menu, "Muted", TRACK_MENU_MUTE,
						track_menu_items);
				_add_separator_to_menu(track_menu, track_menu_items);
				_add_option_to_menu(track_menu, "Automations..",
						TRACK_MENU_EDIT_AUTOMATIONS, track_menu_items);
				_add_separator_to_menu(track_menu, track_menu_items);
				_add_option_to_menu(track_menu, "Rename..", TRACK_MENU_RENAME,
						track_menu_items);

				_add_separator_to_menu(track_menu, track_menu_items);
				_add_option_to_menu(track_menu, "Add Effect or Synth..", TRACK_MENU_ADD_EFFECT,
						track_menu_items);
				int effect_count = song->get_track(E->get().track)->get_audio_effect_count();
				if (track->get_audio_effect_count()) {
					_add_separator_to_menu(track_menu, track_menu_items, "Effects");
					for (int i = 0; i < effect_count; i++) {
						_add_option_to_menu(track_menu, track->get_audio_effect(i)->get_info()->caption.utf8().get_data(), BASE_EFFECT + i,
								track_menu_items);
					}
				}

				if (song->get_track_count() > 1) {
					_add_separator_to_menu(track_menu, track_menu_items);
					if (E->get().track > 0)
						_add_option_to_menu(track_menu, "Move Left", TRACK_MENU_MOVE_LEFT,
								track_menu_items);
					if (E->get().track < song->get_track_count() - 1)
						_add_option_to_menu(track_menu, "Move Right", TRACK_MENU_MOVE_RIGHT,
								track_menu_items);
				}
				_add_separator_to_menu(track_menu, track_menu_items);
				_add_option_to_menu(track_menu, "Remove Track", TRACK_MENU_REMOVE,
						track_menu_items);
				current_menu_track = E->get().track;

				track_menu->popup_at_pointer((GdkEvent *)event);
				return;
			}
		}

		for (List<AutomationButton>::Element *E = automation_buttons.front(); E;
				E = E->next()) {

			if (E->get().r.intersects(posr)) {

				if (automation_menu) {
					// I dont understand how to delete all the items, great usability
					// there... so i create a new one
					delete automation_menu;
					for (int i = 0; i < automation_menu_items.size(); i++) {
						delete automation_menu_items[i];
					}
					automation_menu_items.clear();
				}

				automation_menu = new Gtk::Menu;

				_add_check_option_to_menu(automation_menu,
						song->get_track(E->get().track)
								->get_automation(E->get().automation)
								->is_visible(),
						false, "Visible", AUTOMATION_MENU_VISIBLE,
						automation_menu_items);

				_add_separator_to_menu(automation_menu, automation_menu_items);

				_add_check_option_to_menu(
						automation_menu,
						song->get_track(E->get().track)
										->get_automation(E->get().automation)
										->get_display_mode() == Automation::DISPLAY_ROWS,
						true, "Numbers", AUTOMATION_MENU_MODE_ROWS, automation_menu_items);

				_add_check_option_to_menu(
						automation_menu,
						song->get_track(E->get().track)
										->get_automation(E->get().automation)
										->get_display_mode() == Automation::DISPLAY_SMALL,
						true, "Small Envelope", AUTOMATION_MENU_MODE_SMALL,
						automation_menu_items);

				_add_check_option_to_menu(
						automation_menu,
						song->get_track(E->get().track)
										->get_automation(E->get().automation)
										->get_display_mode() == Automation::DISPLAY_LARGE,
						true, "Large Envelope", AUTOMATION_MENU_MODE_LARGE,
						automation_menu_items);

				if (song->get_track(E->get().track)->get_automation_count() > 1) {
					_add_separator_to_menu(automation_menu, automation_menu_items);
					if (E->get().automation > 0)
						_add_option_to_menu(automation_menu, "Move Left",
								AUTOMATION_MENU_MOVE_LEFT,
								automation_menu_items);
					if (E->get().automation <
							song->get_track(E->get().track)->get_automation_count() - 1)
						_add_option_to_menu(automation_menu, "Move Right",
								AUTOMATION_MENU_MOVE_RIGHT,
								automation_menu_items);
				}
				_add_separator_to_menu(automation_menu, automation_menu_items);
				_add_option_to_menu(automation_menu, "Remove", AUTOMATION_MENU_REMOVE,
						automation_menu_items);

				current_menu_track = E->get().track;
				current_menu_automation = E->get().automation;

				automation_menu->popup_at_pointer((GdkEvent *)event);

				return;
			}
		}

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

				if (d < 4) {
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

			} else if (event->state & GDK_CONTROL_MASK &&
					   event->x >= E->get().fields[0].x &&
					   event->x < E->get().fields[0].x + E->get().fields[0].width) {
				// add it
				int x = event->x - E->get().fields[0].x;
				int y = event->y;
				int w = E->get().fields[0].width;

				Tick tick = MAX(0, (y - row_top_ofs + v_offset * row_height_cache)) *
							TICKS_PER_BEAT / (row_height_cache * rows_per_beat);

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
			} else {
				for (int i = 0; i < E->get().fields.size(); i++) {
					int localx = event->x - E->get().fields[i].x;
					if (localx >= 0 && localx < E->get().fields[i].width) {
						cursor.field = i;
						cursor.column = E->get().column;
						queue_draw();
						cursor.row = (event->y / row_height_cache) + v_offset;

						return;
					}
				}
			}
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
	}
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
					TICKS_PER_BEAT / (row_height_cache * rows_per_beat);

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

bool PatternEditor::on_key_press_event(GdkEventKey *key_event) {

	printf("state: %x\n",
			(key_event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK |
										GDK_MOD1_MASK | GDK_SUPER_MASK | GDK_META_MASK)));
	;
	if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_MOVE_UP)) {
		cursor.row -= cursor.skip;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_MOVE_DOWN)) {
		cursor.row += cursor.skip;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_MOVE_UP_1_ROW)) {
		cursor.row -= 1;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_MOVE_DOWN_1_ROW)) {
		cursor.row += 1;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_PAGE_UP)) {
		cursor.row -=
				song->pattern_get_beats_per_bar(current_pattern) * rows_per_beat;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_PAGE_DOWN)) {
		cursor.row +=
				song->pattern_get_beats_per_bar(current_pattern) * rows_per_beat;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_MOVE_LEFT)) {

		if (cursor.field == 0) {
			if (cursor.column > 0) {
				cursor.column--;

				Track *track;
				int automation;
				int column;
				get_cursor_column_data(&track, automation, column);
				ERR_FAIL_COND_V(!track, false);

				if (automation >= 0) {
					if (track->get_automation(automation)->get_display_mode() !=
							Automation::DISPLAY_ROWS) {
						cursor.field = 0;
					} else {
						cursor.field = 1;
					}
				} else {
					cursor.field = 3;
				}
			}
		} else {
			cursor.field--;
		}
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::CURSOR_MOVE_RIGHT)) {

		Track *track;
		int automation;
		int column;
		get_cursor_column_data(&track, automation, column);
		ERR_FAIL_COND_V(!track, false);

		int max_field = 1;

		if (automation >= 0) {
			if (track->get_automation(automation)->get_display_mode() !=
					Automation::DISPLAY_ROWS) {
				max_field = 0;
			} else {
				max_field = 1;
			}
		} else {
			max_field = 3;
		}

		if (cursor.field == max_field) {
			if (cursor.column < song->get_event_column_count() - 1) {
				cursor.column++;
				cursor.field = 0;
			}
		} else {
			cursor.field++;
		}
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_TAB)) {

		if (cursor.column < song->get_event_column_count() - 1) {
			cursor.column++;
			cursor.field = 0;
		}
		_validate_cursor();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_BACKTAB)) {

		if (cursor.field > 0)
			cursor.field = 0;
		else if (cursor.column > 0)
			cursor.column--;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_HOME)) {

		cursor.column = 0;
		cursor.field = 0;
		_validate_cursor();

	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_END)) {

		cursor.column = song->get_event_column_count() - 1;
		cursor.field = 0;
		_validate_cursor();
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::PATTERN_PAN_WINDOW_UP)) {

		if (v_offset > 0) {
			if (v_offset + visible_rows - 1 == cursor.row)
				cursor.row--;
			v_offset--;
			queue_draw();
		}
	} else if (key_bindings->is_keybind(key_event,
					   KeyBindings::PATTERN_PAN_WINDOW_DOWN)) {

		if (v_offset + visible_rows < get_total_rows()) {
			if (cursor.row <= v_offset) {
				cursor.row++;
			}
			v_offset++;
			queue_draw();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_INSERT)) {

		List<Track::PosEvent> events;

		Track::Pos from;
		from.tick = cursor.row * TICKS_PER_BEAT / rows_per_beat;
		from.column = cursor.column;

		Track::Pos to;
		to.tick = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;
		to.column = cursor.column;

		song->get_events_in_range(current_pattern, from, to, &events);

		if (events.size()) {

			undo_redo->begin_action("Insert", true);

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				ev.a = 0xFF;
				ev.b = 0xFF;

				undo_redo->do_method(song, &Song::set_event, current_pattern,
						cursor.column, E->get().pos.tick, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->do_method(
						song, &Song::set_event, current_pattern, cursor.column,
						E->get().pos.tick + TICKS_PER_BEAT / rows_per_beat, ev);
				ev.a = 0xFF;
				ev.b = 0xFF;
				undo_redo->undo_method(
						song, &Song::set_event, current_pattern, cursor.column,
						E->get().pos.tick + TICKS_PER_BEAT / rows_per_beat, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						cursor.column, E->get().pos.tick, ev);
			}

			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		}
	} else if (key_bindings->is_keybind(key_event, KeyBindings::CURSOR_DELETE)) {

		List<Track::PosEvent> events;

		Track::Pos from;
		from.tick = cursor.row * TICKS_PER_BEAT / rows_per_beat;
		from.column = cursor.column;

		Track::Pos to;
		to.tick = song->pattern_get_beats(current_pattern) * TICKS_PER_BEAT;
		to.column = cursor.column;

		song->get_events_in_range(current_pattern, from, to, &events);

		if (events.size()) {

			undo_redo->begin_action("Delete", true);

			Tick limit = from.tick;

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				ev.a = 0xFF;
				ev.b = 0xFF;

				undo_redo->do_method(song, &Song::set_event, current_pattern,
						cursor.column, E->get().pos.tick, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Tick new_ofs = E->get().pos.tick - TICKS_PER_BEAT / rows_per_beat;

				if (new_ofs < limit)
					continue;

				Track::Event ev = E->get().event;
				undo_redo->do_method(song, &Song::set_event, current_pattern,
						cursor.column, new_ofs, ev);
				ev.a = 0xFF;
				ev.b = 0xFF;
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						cursor.column, new_ofs, ev);
			}

			for (List<Track::PosEvent>::Element *E = events.front(); E;
					E = E->next()) {

				Track::Event ev = E->get().event;
				undo_redo->undo_method(song, &Song::set_event, current_pattern,
						cursor.column, E->get().pos.tick, ev);
			}

			undo_redo->do_method(this, &PatternEditor::_redraw);
			undo_redo->undo_method(this, &PatternEditor::_redraw);
			undo_redo->commit_action();
		}

	} else {

		// check field

		Track *track;
		int automation;
		int column;
		get_cursor_column_data(&track, automation, column);
		ERR_FAIL_COND_V(!track, false);

		if (column >= 0) {

			if (cursor.field == 0) {
				// put a note
				for (int i = KeyBindings::PIANO_C0; i <= KeyBindings::PIANO_E2; i++) {
					if (key_bindings->is_keybind(key_event, KeyBindings::KeyBind(i))) {
						int note = i - KeyBindings::PIANO_C0;

						Track::Event ev =
								song->get_event(current_pattern, cursor.column,
										cursor.row * TICKS_PER_BEAT / rows_per_beat);
						Track::Event old_ev = ev;
						ev.a = current_octave * 12 + note;
						undo_redo->begin_action("Add Note");
						undo_redo->do_method(
								song, &Song::set_event, current_pattern, cursor.column,
								Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
						undo_redo->undo_method(
								song, &Song::set_event, current_pattern, cursor.column,
								Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
						undo_redo->do_method(this, &PatternEditor::_redraw);
						undo_redo->undo_method(this, &PatternEditor::_redraw);
						undo_redo->commit_action();
						_cursor_advance();

						return true;
					}
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::PATTERN_CURSOR_NOTE_OFF)) {

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / rows_per_beat);
					Track::Event old_ev = ev;
					ev.a = Track::Note::OFF;
					undo_redo->begin_action("Add Note Off");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					_cursor_advance();
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}

			} else if (cursor.field == 1) {
				// put octave
				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {

					int octave = key_event->keyval - GDK_KEY_0;

					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / rows_per_beat);
					Track::Event old_ev = ev;

					if (old_ev.a == Track::Note::EMPTY) {
						return true; // no add octave on empty
					}

					ev.a = (ev.a % 12) + octave * 12;

					undo_redo->begin_action("Set Octave");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					_cursor_advance();

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
									cursor.row * TICKS_PER_BEAT / rows_per_beat);
					Track::Event old_ev = ev;

					if (old_ev.b == Track::Note::EMPTY) {
						ev.b = 0;
					}

					ev.b = (ev.b % 10) + vol_g * 10;

					undo_redo->begin_action("Set Volume");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
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
									cursor.row * TICKS_PER_BEAT / rows_per_beat);

					Track::Event old_ev = ev;

					if (old_ev.b == Track::Note::EMPTY) {
						ev.b = 0;
					}

					ev.b -= (ev.b % 10);
					ev.b += vol_l;

					undo_redo->begin_action("Set Volume");
					undo_redo->do_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					cursor.field = 2;
					_cursor_advance();

					return true;
				}

				if (key_bindings->is_keybind(key_event,
							KeyBindings::CURSOR_FIELD_CLEAR)) {
					_field_clear();
				}
			}
		} else if (automation >= 0) {

			Automation *a = track->get_automation(automation);
			if (a->get_display_mode() == Automation::DISPLAY_ROWS) {

				if (key_event->keyval >= GDK_KEY_0 && key_event->keyval <= GDK_KEY_9) {
					Track::Event ev =
							song->get_event(current_pattern, cursor.column,
									cursor.row * TICKS_PER_BEAT / rows_per_beat);

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
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), ev);
					undo_redo->undo_method(
							song, &Song::set_event, current_pattern, cursor.column,
							Tick(cursor.row * TICKS_PER_BEAT / rows_per_beat), old_ev);
					undo_redo->do_method(this, &PatternEditor::_redraw);
					undo_redo->undo_method(this, &PatternEditor::_redraw);
					undo_redo->commit_action();
					if (cursor.field == 0) {
						cursor.field = 1;
					} else {
						cursor.field = 0;
						_cursor_advance();
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
	printf("keypress\n");
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

bool PatternEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	Gdk::Cairo::set_source_rgba(cr, theme->colors[Theme::COLOR_BACKGROUND]);
	cr->rectangle(0, 0, w, h);
	cr->fill();

	cr->select_font_face(theme->fonts[Theme::FONT_PATTERN].face.utf8().get_data(),
			Cairo::FONT_SLANT_NORMAL,
			theme->fonts[Theme::FONT_PATTERN].bold ? Cairo::FONT_WEIGHT_BOLD : Cairo::FONT_WEIGHT_NORMAL);
	cr->set_font_size(theme->fonts[Theme::FONT_PATTERN].size);
	Cairo::FontExtents fe;
	cr->get_font_extents(fe);

	int fw = fe.max_x_advance;
	int fh = fe.height;
	int fa = fe.ascent;
	int sep = 1;
	fh += sep;

	row_height_cache = fh;
	int top_ofs = 4;

	row_top_ofs = top_ofs;

	Gdk::RGBA track_sep_color =
			theme->colors[Theme::COLOR_PATTERN_EDITOR_TRACK_SEPARATOR];
	Gdk::RGBA cursorcol = theme->colors[Theme::COLOR_PATTERN_EDITOR_CURSOR];
	int track_sep_w =
			theme->constants[Theme::CONSTANT_PATTERN_EDITOR_TRACK_SEPARATION];

	track_buttons.clear();
	automation_buttons.clear();
	click_areas.clear();

	visible_rows = (h - top_ofs) / fh;

	int beats_per_bar = song->pattern_get_beats_per_bar(current_pattern);

	for (int i = 0; i < visible_rows; i++) {

		int row = v_offset + i;
		int beat = row / rows_per_beat;
		int subbeat = row % rows_per_beat;

		if (subbeat == 0 || i == 0) {
			if (beat % beats_per_bar == 0)
				_draw_text(cr, 0, top_ofs + i * fh + fa, String::num(beat),
						theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_BAR]);
			else
				_draw_text(cr, 0, top_ofs + i * fh + fa, String::num(beat),
						theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_BEAT]);

		} else {
			char text[3] = { '0' + (subbeat / 10), '0' + (subbeat % 10), 0 };
			_draw_text(cr, fw, top_ofs + i * fh + fa, text,
					theme->colors[Theme::COLOR_PATTERN_EDITOR_ROW_SUB_BEAT]);
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

				int as = fh;
				_draw_arrow(cr, ofs, top_ofs, as, as,
						theme->colors[Theme::COLOR_PATTERN_EDITOR_TRACK_NAME]);
				_draw_text(cr, ofs + fh - fa, top_ofs + as, t->get_name(),
						theme->colors[Theme::COLOR_PATTERN_EDITOR_TRACK_NAME], true);
				TrackButton tb;
				tb.track = i;

				tb.r = Gdk::Rectangle(ofs, top_ofs, as, as);
				track_buttons.push_back(tb);
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
				int beat = row / rows_per_beat;
				int subbeat = row % rows_per_beat;

				Gdk::RGBA c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];
				Gdk::RGBA bgc = bgcol;

				if (subbeat == 0 || k == 0) {
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
				from.tick = row * TICKS_PER_BEAT / rows_per_beat;
				from.column = j;
				Tick to_tick = (row + 1) * TICKS_PER_BEAT / rows_per_beat;
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

					_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);
				} else if (valid.size() == 1) {

					Track::PosNote n = pn.front()->get();
					if (n.pos.tick != from.tick)
						c = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
					if (n.note.note == Track::Note::OFF) {
						rowstr[0] = '=';
						rowstr[1] = '=';
						rowstr[2] = '=';
					} else if (n.note.note < 120) {
						static const char *note[12] = { "C-", "C#", "D-", "D#", "E-", "F-",
							"F#", "G-", "G#", "A-", "A#", "B-" };
						rowstr[0] = note[n.note.note % 12][0];
						rowstr[1] = note[n.note.note % 12][1];
						rowstr[2] = '0' + n.note.note / 12; // octave
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

						if (valid[l].note.note < 120) {
							_draw_fill_rect(cr, base_x, base_y + h * l, fw * 3, h - 1, c);
							_draw_rect(cr, base_x + valid[l].note.note * w / 120,
									base_y + 1 + h * l, 2, h - 2, bgc);
						} else if (valid[l].note.note == Track::Note::OFF) {
							_draw_rect(cr, base_x + 0, base_y + 1 + h * l, fw * 3, 1, c);
							_draw_rect(cr, base_x, base_y + 1 + h * l + h - 2, fw * 3, 1, c);
						}

						if (valid[l].note.volume < 100) {
							_draw_fill_rect(cr, base_x + fw * 4, base_y + h * l, fw * 2,
									h - 1, c);
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
				ofs +=
						theme->constants[Theme::CONSTANT_PATTERN_EDITOR_COLUMN_SEPARATION];
			ofs += fw * 6;
			idx++;
		}

		for (int j = 0; j < t->get_automation_count(); j++) {

			Automation *a = t->get_automation(j);

			if (idx < h_offset) {
				if (a->is_visible())
					idx++;
				continue;
			}

			{

				int as = fh;

				_draw_arrow(cr, ofs, top_ofs, as, as,
						theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_NAME]);
				_draw_text(cr, ofs + fh - fa, top_ofs + as,
						a->get_control_port()->get_name().utf8().get_data(),
						theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_NAME],
						true);

				AutomationButton ab;
				ab.track = i;
				ab.automation = j;
				ab.r = Gdk::Rectangle(ofs, top_ofs, as, as);
				automation_buttons.push_back(ab);
			}
			ofs += fh;

			if (!a->is_visible()) {
				continue;
			}

			switch (a->get_display_mode()) {
				case Automation::DISPLAY_ROWS: {

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
						int beat = row / rows_per_beat;
						int subbeat = row % rows_per_beat;

						Gdk::RGBA c =
								theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE];
						Gdk::RGBA bgc = bgcol;

						if (subbeat == 0 || k == 0) {
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

						Tick from = row * TICKS_PER_BEAT / rows_per_beat;
						Tick to = (row + 1) * TICKS_PER_BEAT / rows_per_beat;

						int first;
						int count;

						a->get_points_in_range(current_pattern, from, to, first, count);

						if (count == 0) {

							_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);
						} else if (count == 1) {

							int val = a->get_point_by_index(current_pattern, first);

							if (a->get_point_tick_by_index(current_pattern, first) != from)
								c = theme->colors
											[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_NOFIT];

							rowstr[0] = '0' + val / 10;
							rowstr[1] = '0' + val % 10;

							_draw_text(cr, ofs, top_ofs + k * fh + fa, rowstr, c);

						} else {

							int base_x = ofs;
							int base_y = top_ofs + k * fh;

							for (int l = 0; l < count; l++) {

								int h = (fh - 2) / count;
								int w = fw * 2 - 2;
								int val = a->get_point_by_index(current_pattern, first + l);

								_draw_fill_rect(cr, base_x, base_y + h * l, fw * 2, h - 1, c);
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
				case Automation::DISPLAY_SMALL:
				case Automation::DISPLAY_LARGE: {

					int w = a->get_display_mode() == Automation::DISPLAY_SMALL ? 4 : 8;
					w *= fw;

					Gdk::RGBA c =
							theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_VALUE];
					Gdk::RGBA cpoint =
							theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_POINT];

					// fill fields for click areas
					ClickArea ca;
					ca.column = idx;
					ClickArea::Field f;
					f.width = w;
					f.x = ofs;
					ca.fields.push_back(f);

					for (int k = 0; k < visible_rows; k++) {

						int row = v_offset + k;
						int beat = row / rows_per_beat;
						int subbeat = row % rows_per_beat;

						if (subbeat == 0 || k == 0) {
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

							_draw_fill_rect(
									cr, ofs, top_ofs + k * fh - sep + fh, w, 1,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT]);
							_draw_fill_rect(
									cr, ofs, top_ofs + k * fh - sep, 1, fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT]);
							_draw_fill_rect(
									cr, ofs + w - 1, top_ofs + k * fh - sep, 1, fh,
									theme->colors[Theme::COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT]);
						}

						float prev = -1;
						for (int l = 0; l < fh; l++) {

							Tick at = (fh * row + l) * (TICKS_PER_BEAT / rows_per_beat) / fh;

							float tofs = a->interpolate_offset(current_pattern, at);
							if (prev == -1)
								prev = tofs;

							if (tofs >= 0)
								_draw_fill_rect(cr, ofs + tofs * w, top_ofs + k * fh - sep + l, 2,
										1, c);

							prev = tofs;
						}

						if (has_focus() && idx == cursor.column && cursor.row == row) {

							int cursor_ofs_x = ofs;
							int cursor_ofs_y = top_ofs + k * fh - sep;

							_draw_rect(cr, cursor_ofs_x - 1, cursor_ofs_y - 1, w + 1, fh + 1,
									cursorcol);
						}
					}

					Tick pfrom = (v_offset) * (TICKS_PER_BEAT / rows_per_beat);
					Tick pto = (v_offset + visible_rows) * (TICKS_PER_BEAT / rows_per_beat);
					int first;
					int count;
					a->get_points_in_range(current_pattern, pfrom, pto, first, count);
					ca.automation = a;

					for (int l = first; l < first + count; l++) {
						int x = (a->get_point_by_index(current_pattern, l) * w /
								 Automation::VALUE_MAX);
						int y = a->get_point_tick_by_index(current_pattern, l) * fh /
										(TICKS_PER_BEAT / rows_per_beat) -
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
	return true;
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
	rows_per_beat = 4;

	Track *t = new Track;
	t->set_columns(2);
	song->add_track(t);
	visible_rows = 4;
	current_octave = 5;
	cursor_advance = 1;

	cursor.row = 0;
	cursor.field = 0;
	cursor.column = 0;
	cursor.skip = 1;

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

	grabbing_point = -1;
	track_menu = NULL;
	automation_menu = NULL;
}

PatternEditor::~PatternEditor() {
	if (track_menu) {
		delete track_menu;
		for (int i = 0; i < track_menu_items.size(); i++) {
			delete track_menu_items[i];
		}
	}
	if (automation_menu) {
		delete automation_menu;
		for (int i = 0; i < automation_menu_items.size(); i++) {
			delete automation_menu_items[i];
		}
	}
}
