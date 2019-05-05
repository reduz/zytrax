#include "interface.h"
#include "icons.h"

void Interface::_add_track() {

	key_bindings.action_activated.emit(KeyBindings::TRACK_ADD_TRACK);
}
void Interface::_track_edited(int p_track) {
	track_settings.show();
}

void Interface::_pattern_changed() {
	pattern_editor.set_current_pattern(pattern.get_adjustment()->get_value());
}
void Interface::_octave_changed() {
	pattern_editor.set_current_octave(octave.get_adjustment()->get_value());
}
void Interface::_step_changed() {
	pattern_editor.set_current_cursor_advance(step.get_adjustment()->get_value());
}
void Interface::_volume_changed() {
	pattern_editor.set_current_volume_mask(volume.get_adjustment()->get_value(), volume_mask.get_active());
}

void Interface::_tempo_changed() {
	undo_redo.begin_action("Set Tempo", true);
	undo_redo.do_method(&song, &Song::set_bpm, (float)tempo.get_adjustment()->get_value());
	undo_redo.undo_method(&song, &Song::set_bpm, song.get_bpm());
	undo_redo.do_method(this, &Interface::_update_editors);
	undo_redo.undo_method(this, &Interface::_update_editors);
	updating_editors = true;
	undo_redo.commit_action();
	updating_editors = false;
}
void Interface::_swing_changed() {
	undo_redo.begin_action("Set Swing", true);
	undo_redo.do_method(&song, &Song::set_swing, float(swing.get_adjustment()->get_value() * 100));
	undo_redo.undo_method(&song, &Song::set_swing, song.get_swing());
	undo_redo.do_method(this, &Interface::_update_editors);
	undo_redo.undo_method(this, &Interface::_update_editors);
	updating_editors = true;
	undo_redo.commit_action();
	updating_editors = false;
}
void Interface::_zoom_changed() {

	Gtk::TreeModel::iterator iter = zoom.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[zoom_model_columns.index];

			pattern_editor.set_beat_zoom(PatternEditor::BeatZoom(id));
		}
	}
}

void Interface::_update_editors() {

	if (updating_editors) {
		return;
	}

	updating_editors = true;
	pattern.get_adjustment()->set_value(pattern_editor.get_current_pattern());
	octave.get_adjustment()->set_value(pattern_editor.get_current_octave());
	step.get_adjustment()->set_value(pattern_editor.get_current_cursor_advance());
	volume.get_adjustment()->set_value(pattern_editor.get_current_volume_mask());
	volume_mask.set_active(pattern_editor.is_current_volume_mask_active());

	tempo.get_adjustment()->set_value(song.get_bpm());
	swing.get_adjustment()->set_value(song.get_swing() * 100);

	zoom.set_active(pattern_editor.get_beat_zoom());

	updating_editors = false;
}

void Interface::_on_action_activated(KeyBindings::KeyBind p_bind) {

	switch (p_bind) {
		case KeyBindings::FILE_NEW: {
		} break;
		case KeyBindings::FILE_OPEN: {
		} break;
		case KeyBindings::FILE_SAVE: {
		} break;
		case KeyBindings::FILE_SAVE_AS: {
		} break;
		case KeyBindings::FILE_QUIT: {
		} break;

		case KeyBindings::PLAYBACK_PLAY: {
		} break;
		case KeyBindings::PLAYBACK_STOP: {
		} break;
		case KeyBindings::PLAYBACK_NEXT_PATTERN: {
		} break;
		case KeyBindings::PLAYBACK_PREV_PATTERN: {
		} break;
		case KeyBindings::PLAYBACK_PLAY_PATTERN: {
		} break;
		case KeyBindings::PLAYBACK_PLAY_FROM_CURSOR: {
		} break;
		case KeyBindings::PLAYBACK_PLAY_FROM_ORDER: {
		} break;
		case KeyBindings::PLAYBACK_CURSOR_FOLLOW: {
		} break;

		case KeyBindings::EDIT_UNDO: {
			undo_redo.undo();
		} break;
		case KeyBindings::EDIT_REDO: {
			undo_redo.redo();
		} break;
		case KeyBindings::EDIT_FOCUS_PATTERN: {
			pattern_editor.grab_focus();
		} break;
		case KeyBindings::EDIT_FOCUS_ORDERLIST: {
		} break;
		case KeyBindings::EDIT_FOCUS_LAST_EDITED_EFFECT: {
		} break;

		case KeyBindings::SETTINGS_OPEN: {
		} break;
		case KeyBindings::SETTINGS_PATTERN_INPUT_KEYS: {
			String text;
			//text = "::Pattern Editing Cheatsheet::\n\n";

			text += "Navigation:\n\n";

			text += "Row Up / Down: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_MOVE_UP) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_MOVE_DOWN) + "\n";
			text += "Field Left / Right: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_MOVE_LEFT) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_MOVE_RIGHT) + "\n";
			text += "Next/Prev Column: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_TAB) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_BACKTAB) + "\n";
			text += "Page Up / Down: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_PAGE_UP) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_PAGE_DOWN) + "\n";
			text += "First/Last Track: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_HOME) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_END) + "\n";
			text += "Scroll Up / Down: " + key_bindings.get_keybind_text(KeyBindings::PATTERN_PAN_WINDOW_UP) + ", " + key_bindings.get_keybind_text(KeyBindings::PATTERN_PAN_WINDOW_DOWN) + "\n";

			text += "\n\nEditing:\n\n";

			text += "Insert/Delete Row: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_INSERT) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_DELETE) + "\n";
			text += "Clear Field: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_INSERT) + ", " + key_bindings.get_keybind_text(KeyBindings::CURSOR_DELETE) + "\n";
			text += "Raise/Lower Octave: " + key_bindings.get_keybind_text(KeyBindings::PATTERN_OCTAVE_RAISE) + ", " + key_bindings.get_keybind_text(KeyBindings::PATTERN_OCTAVE_LOWER) + "\n";
			text += "Copy Volume Mask: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_COPY_VOLUME_MASK) + "\n";
			text += "Toggle Volume Mask: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_TOGGLE_VOLUME_MASK) + "\n";
			text += "Play Note: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_PLAY_NOTE) + "\n";
			text += "Play Row: " + key_bindings.get_keybind_text(KeyBindings::CURSOR_PLAY_ROW) + "\n";
			text += "Insert Note Off: " + key_bindings.get_keybind_text(KeyBindings::PATTERN_CURSOR_NOTE_OFF) + "\n";

			text += "\n\nVirtual Piano Lower Row (C0 -> B0):\n\n";
			for (int i = 0; i < 12; i++) {
				if (i > 0) {
					text += ", ";
				}
				text += key_bindings.get_keybind_text(KeyBindings::KeyBind(KeyBindings::PIANO_C0 + i));
			}

			text += "\n\nVirtual Piano Upper Row (C1 -> E2):\n\n";
			for (int i = 0; i < 17; i++) {
				if (i > 0) {
					text += ", ";
				}
				text += key_bindings.get_keybind_text(KeyBindings::KeyBind(KeyBindings::PIANO_C1 + i));
			}

			Gtk::MessageDialog about_box(text.ascii().get_data(), false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);

			about_box.set_title("Pattern Editing Cheatsheet");
			about_box.set_position(Gtk::WIN_POS_CENTER_ALWAYS);
			about_box.run();
			about_box.hide();

		} break;
		case KeyBindings::SETTINGS_ABOUT: {
			Gtk::MessageDialog about_box(Glib::ustring(VERSION_WITH_COPYRIGHT) + "\nhttp://zytrax.org", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);

			about_box.set_title("About");
			about_box.set_position(Gtk::WIN_POS_CENTER_ALWAYS);
			about_box.run();
			about_box.hide();

		} break;
		default: {}
	}
}

void Interface::_update_selected_track() {

	int current_track = pattern_editor.get_current_track();

	for (int i = 0; i < racks.size(); i++) {

		racks[i].rack->set_selected(i == current_track);
		racks[i].volume->set_selected(i == current_track);
	}

	_ensure_selected_track_visible();
}

void Interface::_ensure_selected_track_visible() {
	int current_track = pattern_editor.get_current_track();

	if (racks.size() == 0 || current_track < 0 || current_track >= racks.size()) {
		return;
	}

	int total_size = track_hbox.get_allocated_width();
	int page_offset = track_scroll.get_hadjustment()->get_value();
	int page_size = track_scroll.get_allocated_width();

	int ofs = 0;
	int size = 0;
	for (int i = 0; i <= current_track; i++) {
		ofs += size;
		size = 0;
		size += racks[i].rack->get_allocated_width();
		size += racks[i].volume->get_allocated_width();
	}

	if (ofs < page_offset) {
		track_scroll.get_hadjustment()->set_value(ofs);
	} else if (ofs + size > page_offset + page_size) {
		track_scroll.get_hadjustment()->set_value(ofs - page_size + size);
	}

	printf("total width: %i, upper %i\n", total_size, page_size);
}
void Interface::_update_tracks() {

	printf("UPDATE TRACKS DOOD\n");
	double previous_scroll = track_scroll.get_hadjustment()->get_value();
	int current_track = pattern_editor.get_current_track();

	for (int i = 0; i < racks.size(); i++) {
		delete racks[i].rack;
		delete racks[i].volume;
	}
	racks.clear();
	for (int i = 0; i < song.get_track_count(); i++) {
		TrackRacks rack;
		rack.volume = new TrackRackVolume(i, &song, &undo_redo, &theme, &key_bindings);
		rack.rack = new TrackRackEditor(i, &song, &undo_redo, &theme, &key_bindings);

		track_hbox.pack_start(*rack.volume, Gtk::PACK_SHRINK);
		track_hbox.pack_start(*rack.rack, Gtk::PACK_SHRINK);
		rack.rack->set_selected(i == current_track);
		rack.volume->set_selected(i == current_track);
		rack.volume->show();
		rack.rack->show();
		racks.push_back(rack);
	}

	track_scroll.get_hadjustment()->set_value(previous_scroll);
	_ensure_selected_track_visible();
}

void Interface::_update_volume_mask() {

	updating_editors = true;
	volume.get_adjustment()->set_value(pattern_editor.get_current_volume_mask());
	volume_mask.set_active(pattern_editor.is_current_volume_mask_active());
	updating_editors = false;
}

void Interface::_update_octave() {

	updating_editors = true;
	octave.get_adjustment()->set_value(pattern_editor.get_current_octave());
	updating_editors = false;
}

void Interface::_update_pattern() {

	updating_editors = true;
	pattern.get_adjustment()->set_value(pattern_editor.get_current_pattern());
	updating_editors = false;
}

void Interface::_update_step() {
	updating_editors = true;
	step.get_adjustment()->set_value(pattern_editor.get_current_cursor_advance());
	updating_editors = false;
}
void Interface::_update_zoom() {

	updating_editors = true;
	zoom.set_active(pattern_editor.get_beat_zoom());
	updating_editors = false;
}

void Interface::_on_application_startup() {
	key_bindings.initialize();

	menu = Gio::Menu::create();

	file_menu = Gio::Menu::create();
	file_menu_file = Gio::Menu::create();
	file_menu->append_section(file_menu_file);
	file_menu_file->append("New", key_bindings.get_keybind_detailed_name(KeyBindings::FILE_NEW).ascii().get_data());
	file_menu_file->append("Open", key_bindings.get_keybind_detailed_name(KeyBindings::FILE_OPEN).ascii().get_data());
	file_menu_file->append("Save", key_bindings.get_keybind_detailed_name(KeyBindings::FILE_SAVE).ascii().get_data());
	file_menu_file->append("Save As", key_bindings.get_keybind_detailed_name(KeyBindings::FILE_SAVE_AS).ascii().get_data());
	file_menu_exit = Gio::Menu::create();
	file_menu->append_section(file_menu_exit);
	file_menu_exit->append("Quit", key_bindings.get_keybind_detailed_name(KeyBindings::FILE_QUIT).ascii().get_data());
	menu->append_submenu("File", file_menu);

	play_menu = Gio::Menu::create();
	play_menu_play = Gio::Menu::create();
	play_menu->append_section(play_menu_play);
	play_menu_play->append("Play Song", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY).ascii().get_data());
	play_menu_play->append("Stop", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_STOP).ascii().get_data());
	play_menu_seek = Gio::Menu::create();
	play_menu->append_section(play_menu_seek);
	play_menu_seek->append("Skip to Next Pattern", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_NEXT_PATTERN).ascii().get_data());
	play_menu_seek->append("Skip to Prev Pattern", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_PREV_PATTERN).ascii().get_data());
	play_menu_pattern = Gio::Menu::create();
	play_menu->append_section(play_menu_pattern);
	play_menu_pattern->append("Play Current Pattern", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_PATTERN).ascii().get_data());
	play_menu_pattern->append("Play From Cursor", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_FROM_CURSOR).ascii().get_data());
	play_menu_pattern->append("Play From Current Order", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_FROM_ORDER).ascii().get_data());
	play_menu_extra = Gio::Menu::create();
	play_menu->append_section(play_menu_extra);
	play_menu_extra->append("Toggle Follow Song", key_bindings.get_keybind_detailed_name(KeyBindings::PLAYBACK_CURSOR_FOLLOW).ascii().get_data());

	menu->append_submenu("Play", play_menu);

	edit_menu = Gio::Menu::create();
	edit_menu_undo = Gio::Menu::create();
	edit_menu->append_section(edit_menu_undo);
	edit_menu_undo->append("Undo", key_bindings.get_keybind_detailed_name(KeyBindings::EDIT_UNDO).ascii().get_data());
	edit_menu_undo->append("Redo", key_bindings.get_keybind_detailed_name(KeyBindings::EDIT_REDO).ascii().get_data());
	edit_menu_focus = Gio::Menu::create();
	edit_menu->append_section(edit_menu_focus);
	edit_menu_focus->append("Focus on Pattern", key_bindings.get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_PATTERN).ascii().get_data());
	edit_menu_focus->append("Focus on Orderlist", key_bindings.get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_ORDERLIST).ascii().get_data());
	edit_menu_focus->append("Open Last Edited Effect", key_bindings.get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_LAST_EDITED_EFFECT).ascii().get_data());
	menu->append_submenu("Edit", edit_menu);

	track_menu = Gio::Menu::create();
	track_menu_add = Gio::Menu::create();
	track_menu->append_section(track_menu_add);
	track_menu_add->append("New Track", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_ADD_TRACK).ascii().get_data());
	track_menu_column = Gio::Menu::create();
	track_menu->append_section(track_menu_column);
	track_menu_column->append("Add Column", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_ADD_COLUMN).ascii().get_data());
	track_menu_column->append("Remove Column", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_REMOVE_COLUMN).ascii().get_data());
	track_menu_solo = Gio::Menu::create();
	track_menu->append_section(track_menu_solo);
	track_menu_solo->append("Mute", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_MUTE).ascii().get_data());
	track_menu_solo->append("Solo", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_SOLO).ascii().get_data());
	track_menu_edit = Gio::Menu::create();
	track_menu->append_section(track_menu_edit);
	track_menu_edit->append("Move Left", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_MOVE_LEFT).ascii().get_data());
	track_menu_edit->append("Move Right", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_MOVE_RIGHT).ascii().get_data());
	track_menu_edit->append("Rename", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_RENAME).ascii().get_data());
	track_menu_remove = Gio::Menu::create();
	track_menu->append_section(track_menu_remove);
	track_menu_remove->append("Remove", key_bindings.get_keybind_detailed_name(KeyBindings::TRACK_REMOVE).ascii().get_data());
	menu->append_submenu("Track", track_menu);

	automation_action = add_action_bool("AutomationMenu", false);
	automation_action->set_enabled(false);
	automation_menu = Gio::Menu::create();
	automation_menu_item = Gio::MenuItem::create("Automation", automation_menu);
	//automation_menu_item->set_attribute_value("submenu-action", Glib::Variant<Glib::ustring>::create("gui.AutomationMenu"));
	g_menu_item_set_attribute(automation_menu_item->gobj(), "submenu-action", "s",
			"win.AutomationMenu");
	menu->append_item(automation_menu_item);

	automation_menu_visible = Gio::Menu::create();
	automation_menu->append_section(automation_menu_visible);
	automation_menu_visible->append("Visible", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_TOGGLE_VISIBLE).ascii().get_data());
	automation_menu_mode = Gio::Menu::create();
	automation_menu->append_section(automation_menu_mode);
	automation_menu_mode->append("Numbers", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_NUMBERS).ascii().get_data());
	automation_menu_mode->append("Small Envelope", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_SMALL).ascii().get_data());
	automation_menu_mode->append("Large Envelope", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_RADIO_ENVELOPE_LARGE).ascii().get_data());

	automation_menu_move = Gio::Menu::create();
	automation_menu->append_section(automation_menu_move);
	automation_menu_move->append("Move Left", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_MOVE_LEFT).ascii().get_data());
	automation_menu_move->append("Move Right", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_MOVE_RIGHT).ascii().get_data());
	automation_menu_remove = Gio::Menu::create();
	automation_menu->append_section(automation_menu_remove);
	automation_menu_remove->append("Remove", key_bindings.get_keybind_detailed_name(KeyBindings::AUTOMATION_REMOVE).ascii().get_data());

	select_menu = Gio::Menu::create();
	select_menu_select = Gio::Menu::create();
	select_menu->append_section(select_menu_select);
	select_menu_select->append("Begin", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_BEGIN).ascii().get_data());
	select_menu_select->append("End", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_END).ascii().get_data());
	select_menu_select->append("Column/Track/Pattern", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_COLUMN_TRACK_ALL).ascii().get_data());
	select_menu_select->append("Clear", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_DISABLE).ascii().get_data());
	select_menu_clipboard = Gio::Menu::create();
	select_menu->append_section(select_menu_clipboard);
	select_menu_clipboard->append("Cut", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_CUT).ascii().get_data());
	select_menu_clipboard->append("Copy", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_COPY).ascii().get_data());
	select_menu_clipboard->append("Paste Insert", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_INSERT).ascii().get_data());
	select_menu_clipboard->append("Paste Overwrite", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_OVERWRITE).ascii().get_data());
	select_menu_clipboard->append("Paste Mix", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_MIX).ascii().get_data());
	select_menu_transpose = Gio::Menu::create();
	select_menu->append_section(select_menu_transpose);
	select_menu_transpose->append("Raise Note(s) a Semitone", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_SEMITONE).ascii().get_data());
	select_menu_transpose->append("Raise Note(s) an Octave", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_OCTAVE).ascii().get_data());
	select_menu_transpose->append("Lower Note(s) a Semitone", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_SEMITONE).ascii().get_data());
	select_menu_transpose->append("Lower Note(s) an Octave", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_OCTAVE).ascii().get_data());
	select_menu_operations = Gio::Menu::create();
	select_menu->append_section(select_menu_operations);
	select_menu_operations->append("Set Volume Mask", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_SET_VOLUME).ascii().get_data());
	select_menu_operations->append("Interpolate Volume or Automation", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION).ascii().get_data());
	select_menu_operations->append("Amplify Volume or Automation", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION).ascii().get_data());
	select_menu_length = Gio::Menu::create();
	select_menu->append_section(select_menu_length);
	select_menu_length->append("Double Length", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_DOUBLE_LENGTH).ascii().get_data());
	select_menu_length->append("Halve Length", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_HALVE_LENGTH).ascii().get_data());
	select_menu_length->append("Scale Length", key_bindings.get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_SCALE_LENGTH).ascii().get_data());
	menu->append_submenu("Selection", select_menu);

	settings_menu = Gio::Menu::create();
	settings_menu_preferences = Gio::Menu::create();
	settings_menu->append_section(settings_menu_preferences);
	settings_menu_preferences->append("Preferences", key_bindings.get_keybind_detailed_name(KeyBindings::SETTINGS_OPEN).ascii().get_data());
	settings_menu_cheat = Gio::Menu::create();
	settings_menu->append_section(settings_menu_cheat);
	settings_menu_cheat->append("Pattern Editing Cheat-Sheet", key_bindings.get_keybind_detailed_name(KeyBindings::SETTINGS_PATTERN_INPUT_KEYS).ascii().get_data());
	settings_menu_about = Gio::Menu::create();
	settings_menu->append_section(settings_menu_about);
	settings_menu_about->append("About", key_bindings.get_keybind_detailed_name(KeyBindings::SETTINGS_ABOUT).ascii().get_data());
	menu->append_submenu("Settings", settings_menu);

	application->set_menubar(menu);

	key_bindings.action_activated.connect(sigc::mem_fun(*this, &Interface::_on_action_activated));
}

Interface::Interface(Gtk::Application *p_application, AudioEffectFactory *p_fx_factory) :
		key_bindings(p_application, this),
		track_settings(p_fx_factory),
		pattern_editor(&song, &undo_redo, &theme, &key_bindings) {

	application = p_application;
	application->signal_startup().connect(sigc::mem_fun(*this, &Interface::_on_application_startup));

	updating_editors = false;

	fx_factory = p_fx_factory;
	add(main_vbox);

	main_vbox.pack_start(grid, Gtk::PACK_SHRINK);

	grid.attach(play_hbox, 0, 0, 1, 1);

	prev_pattern_icon = create_image_from_icon("PrevPattern");
	prev_pattern.set_image(prev_pattern_icon);
	play_hbox.pack_start(prev_pattern, Gtk::PACK_SHRINK);

	play_icon = create_image_from_icon("Play");
	play.set_image(play_icon);
	play_hbox.pack_start(play, Gtk::PACK_SHRINK);

	stop_icon = create_image_from_icon("Stop");
	stop.set_image(stop_icon);
	play_hbox.pack_start(stop, Gtk::PACK_SHRINK);

	next_pattern_icon = create_image_from_icon("NextPattern");
	next_pattern.set_image(next_pattern_icon);
	play_hbox.pack_start(next_pattern, Gtk::PACK_SHRINK);

	sep1.set_text("    ");
	play_hbox.pack_start(sep1, Gtk::PACK_SHRINK);

	play_pattern_icon = create_image_from_icon("PlayPattern");
	play_pattern.set_image(play_pattern_icon);
	play_hbox.pack_start(play_pattern, Gtk::PACK_SHRINK);

	play_cursor_icon = create_image_from_icon("PlayFromCursor");
	play_cursor.set_image(play_cursor_icon);
	play_hbox.pack_start(play_cursor, Gtk::PACK_SHRINK);

	sep2.set_text("    ");
	play_hbox.pack_start(sep2, Gtk::PACK_SHRINK);

	add_track_icon = create_image_from_icon("AddTrack");
	add_track.set_image(add_track_icon);
	add_track.set_label(" New Track");
	add_track.set_always_show_image(true);
	play_hbox.pack_start(add_track, Gtk::PACK_SHRINK);
	add_track.signal_clicked().connect(sigc::mem_fun(*this, &Interface::_add_track));

	//play_hbox.pack_start(spacer1, Gtk::PACK_EXPAND_WIDGET);

	tempo_label.set_text(" Tempo: ");
	grid.attach(tempo_label, 4, 0, 1, 1);
	grid.attach(tempo, 5, 0, 1, 1);
	tempo.set_adjustment(Gtk::Adjustment::create(1, 30, 299));
	tempo.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_tempo_changed));

	swing_label.set_text(" Swing(%): ");
	grid.attach(swing_label, 6, 0, 1, 1);
	grid.attach(swing, 7, 0, 1, 1);
	swing.set_adjustment(Gtk::Adjustment::create(0, 0, 100));
	swing.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_swing_changed));

	grid.attach(pattern_hbox, 0, 1, 1, 1);

	pattern_label.set_text(" Pattern: ");
	pattern_hbox.pack_start(pattern_label, Gtk::PACK_SHRINK);
	pattern_hbox.pack_start(pattern, Gtk::PACK_SHRINK);
	pattern.set_adjustment(Gtk::Adjustment::create(0, 0, 999));
	pattern.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_pattern_changed));

	pattern_settings_icon = create_image_from_icon("Settings");
	pattern_settings.set_image(pattern_settings_icon);
	pattern_hbox.pack_start(pattern_settings, Gtk::PACK_SHRINK);

	/*pattern_length_label.set_text(" Length: ");
	top_hbox.pack_start(pattern_length_label, Gtk::PACK_SHRINK);
	top_hbox.pack_start(pattern_length, Gtk::PACK_SHRINK);
	pattern_length_set_next.set_label("Next..");
	top_hbox.pack_start(pattern_length_set_next, Gtk::PACK_SHRINK);
*/
	zoom_label.set_text(" Zoom: ");
	pattern_hbox.pack_start(zoom_label, Gtk::PACK_SHRINK);
	pattern_hbox.pack_start(zoom, Gtk::PACK_SHRINK);

	{

		zoomlist_store = Gtk::ListStore::create(zoom_model_columns);
		zoom.set_model(zoomlist_store);

		const char *beat_zoom_text[PatternEditor::BEAT_ZOOM_MAX] = {
			"1 Beat",
			"1/2 Beat",
			"1/3 Beat",
			"1/4 Beat",
			"1/6 Beat",
			"1/8 Beat",
			"1/12 Beat",
			"1/16 Beat",
			"1/24 Beat",
			"1/32 Beat",
			"1/48 Beat",
			"1/64 Beat"
		};

		for (int i = 0; i < PatternEditor::BEAT_ZOOM_MAX; i++) {
			Gtk::TreeModel::Row row = *(zoomlist_store->append());
			row[zoom_model_columns.name] = beat_zoom_text[i];
			row[zoom_model_columns.index] = i;
			zoom_rows.push_back(row);
		}

		zoom.pack_start(zoom_model_columns.name);
	}

	zoom.set_active(zoom_rows[3]);
	zoom.signal_changed().connect(sigc::mem_fun(*this, &Interface::_zoom_changed));

	grid.attach(spacer1, 1, 1, 1, 1);
	spacer1.set_hexpand(true);

	volume_mask.set_label(" Volume: ");
	grid.attach(volume_mask, 2, 1, 1, 1);
	grid.attach(volume, 3, 1, 1, 1);
	volume.set_adjustment(Gtk::Adjustment::create(0, 0, 99));
	volume.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_volume_changed));
	volume_mask.signal_clicked().connect(sigc::mem_fun(*this, &Interface::_volume_changed));

	octave_label.set_text(" Octave: ");
	grid.attach(octave_label, 4, 1, 1, 1);
	grid.attach(octave, 5, 1, 1, 1);
	octave.set_adjustment(Gtk::Adjustment::create(0, 0, 10));
	octave.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_octave_changed));

	step_label.set_text(" Step: ");
	grid.attach(step_label, 6, 1, 1, 1);
	grid.attach(step, 7, 1, 1, 1);
	step.set_adjustment(Gtk::Adjustment::create(1, 0, 16));
	step.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(*this, &Interface::_step_changed));

	main_vbox.pack_start(main_split, Gtk::PACK_EXPAND_WIDGET);

	main_split.pack1(main_hbox, true, false);

	main_hbox.pack_start(pattern_vbox, Gtk::PACK_EXPAND_WIDGET);
	pattern_vbox.pack_start(pattern_editor, Gtk::PACK_EXPAND_WIDGET);
	pattern_editor.track_edited.connect(sigc::mem_fun(this, &Interface::_track_edited));
	main_hbox.pack_start(pattern_vscroll, Gtk::PACK_SHRINK);
	pattern_vbox.pack_start(pattern_hscroll, Gtk::PACK_SHRINK);

	main_split.pack2(track_scroll, false, false);
	track_scroll.add(track_hbox);
	track_scroll.set_propagate_natural_height(true);
	track_scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_NEVER);

	pattern_editor.track_edited.connect(sigc::mem_fun(this, &Interface::_track_edited));
	pattern_editor.track_layout_changed.connect(sigc::mem_fun(this, &Interface::_update_tracks));
	pattern_editor.current_track_changed.connect(sigc::mem_fun(this, &Interface::_update_selected_track));
	pattern_editor.volume_mask_changed.connect(sigc::mem_fun(this, &Interface::_update_volume_mask));
	pattern_editor.octave_changed.connect(sigc::mem_fun(this, &Interface::_update_octave));
	pattern_editor.pattern_changed.connect(sigc::mem_fun(this, &Interface::_update_pattern));
	pattern_editor.step_changed.connect(sigc::mem_fun(this, &Interface::_update_step));
	pattern_editor.zoom_changed.connect(sigc::mem_fun(this, &Interface::_update_zoom));

	pattern_editor.set_hscroll(pattern_hscroll.get_adjustment());
	pattern_editor.set_vscroll(pattern_vscroll.get_adjustment());

	show_all_children();

	//pattern_editor.init();
	_update_editors();
}

Interface::~Interface() {

	for (int i = 0; i < menu_items.size(); i++) {
		delete menu_items[i];
	}
	for (int i = 0; i < racks.size(); i++) {
		delete racks[i].rack;
		delete racks[i].volume;
	}
}
