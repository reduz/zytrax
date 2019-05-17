#include "interface.h"
#include "icons.h"

void Interface::_add_track() {

	key_bindings->action_activated.emit(KeyBindings::TRACK_ADD_TRACK);
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
	if (updating_editors) {
		return;
	}
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
	if (updating_editors) {
		return;
	}

	undo_redo.begin_action("Set Swing", true);
	undo_redo.do_method(&song, &Song::set_swing, float(swing.get_adjustment()->get_value() / 100.0));
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

void Interface::_update_title() {

	String song_name;
	if (song.get_name() != String()) {
		song_name = song.get_name();
	}
	if (song_path != String()) {
		int last = MAX(song_path.find_last("/"), song_path.find_last("\\"));
		if (last != -1) {
			song_name = song_path.substr(last + 1, song_path.length());
		}
	}

	String title;

	if (song_name != String()) {
		title = String() + VERSION_SOFTWARE_NAME + " " + VERSION_MKSTRING + " - " + song_name;
	} else {
		title = String() + VERSION_SOFTWARE_NAME + " " + VERSION_MKSTRING + " " + VERSION_COPYRIGHT;
	}

	if (undo_redo.get_current_version() != save_version) {
		title += " (*)";
	}

	set_title(title.utf8().get_data());
}

void Interface::_export_dialog_callback(int p_order, void *p_userdata) {
	Gtk::ProgressBar *pb = (Gtk::ProgressBar *)p_userdata;
	int last_order = -1;
	for (int i = 0; i < Song::ORDER_MAX; i++) {
		last_order = i;
		if (singleton->song.order_get(i) == Song::ORDER_EMPTY) {
			break;
		}
	}

	if (last_order <= 0) {
		last_order = 1;
	}
	float progress = float(p_order) / float(last_order);
	pb->set_fraction(progress);
	singleton->export_wav_label.set_text(String(String("Exporting Order ") + String::num(p_order) + "/" + String::num(last_order) + " (" + String::num(int(progress * 100)) + "%)").utf8().get_data());
	while (gtk_events_pending()) {
		gtk_main_iteration_do(false);
	}
}

bool Interface::_export_dialog_key(GdkEvent *p_key) {
	//avoid closing scan with escape key
	if (p_key->type == GDK_KEY_PRESS && ((GdkEventKey *)(p_key))->keyval == GDK_KEY_Escape) {
		return true;
	} else {
		return false;
	}
}

void Interface::_on_action_activated(KeyBindings::KeyBind p_bind) {

	switch (p_bind) {
		case KeyBindings::FILE_NEW: {
			Gtk::MessageDialog error_box("Clear song? (no undo)", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK_CANCEL);
			error_box.set_transient_for(*this);
			error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			if (error_box.run() == Gtk::RESPONSE_OK) {
				song.clear();
				undo_redo.clean();
				save_version = 0;
				_update_editors();
				_update_tracks();
				_update_title();
				_update_colors();
				song_path = String();
			}
			error_box.hide();

		} break;
		case KeyBindings::FILE_OPEN: {

			Gtk::FileChooserDialog dialog("Select a file to open",
					Gtk::FILE_CHOOSER_ACTION_OPEN);
			dialog.set_transient_for(*this);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

			//Add response buttons the the dialog:
			gboolean swap_buttons;
			g_object_get(gtk_settings_get_default(), "gtk-alternative-button-order", &swap_buttons, NULL);
			if (swap_buttons) {
				dialog.add_button("Select", Gtk::RESPONSE_OK);
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
			} else {
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
				dialog.add_button("Select", Gtk::RESPONSE_OK);
			}

			auto filter_zt = Gtk::FileFilter::create();
			filter_zt->set_name((String(VERSION_SOFTWARE_NAME) + " files").utf8().get_data());
			filter_zt->add_pattern("*.zyt");
			dialog.add_filter(filter_zt);

			if (song_path != String()) {
				dialog.set_filename(song_path.utf8().get_data());
			}

			int result = dialog.run();
			dialog.hide();

			if (result == Gtk::RESPONSE_OK) {
				String path;
				path.parse_utf8(dialog.get_filename().c_str());

				List<SongFile::MissingPlugin> missing;

				Error err = song_file.load(path, &missing);
				if (err != OK) {
					String err_str;
					if (err == ERR_FILE_CANT_OPEN) {
						err_str = "Error: Can't open file (no access?).";
					} else if (err == ERR_FILE_TOO_NEW) {
						err_str = "Error: File was saved by a newer version.";
					} else if (err == ERR_FILE_CORRUPT) {
						err_str = "Error: File is corrupted.";
					} else {
						err_str = "Error: Could not load.";
					}

					Gtk::MessageDialog error_box(err_str.utf8().get_data(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
					error_box.set_transient_for(*this);
					error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
					error_box.run();
					error_box.hide();
				} else {
					song_path = path;
					undo_redo.clean();
					save_version = 0;
					_update_editors();
					_update_tracks();
					_update_title();
					_update_colors();
					if (missing.size()) {
						String error_text = "The following plugins were not found:\n\n";
						for (List<SongFile::MissingPlugin>::Element *E = missing.front(); E; E = E->next()) {
							error_text += E->get().provider + ": " + E->get().id + "\n";
						}

						error_text += "\nDummy plugins were used instead.";
						Gtk::MessageDialog error_box(error_text.utf8().get_data(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
						error_box.set_transient_for(*this);
						error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
						error_box.run();
						error_box.hide();
					}
				}
			}

		} break;
		case KeyBindings::FILE_SAVE: {
			if (song_path != String()) {
				//just save
				Error err = song_file.save(song_path);
				if (err != OK) {
					Gtk::MessageDialog error_box("Error saving file", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
					error_box.set_transient_for(*this);
					error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
					error_box.run();
					error_box.hide();
				} else {
					save_version = undo_redo.get_current_version();
					_update_title();
				}
				break;
			}
		}; //fall through, no file
		case KeyBindings::FILE_SAVE_AS: {

			Gtk::FileChooserDialog dialog("Select a file to save the song",
					Gtk::FILE_CHOOSER_ACTION_SAVE);
			dialog.set_transient_for(*this);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

			//Add response buttons the the dialog:
			gboolean swap_buttons;
			g_object_get(gtk_settings_get_default(), "gtk-alternative-button-order", &swap_buttons, NULL);
			if (swap_buttons) {
				dialog.add_button("Select", Gtk::RESPONSE_OK);
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
			} else {
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
				dialog.add_button("Select", Gtk::RESPONSE_OK);
			}

			auto filter_zt = Gtk::FileFilter::create();
			filter_zt->set_name((String(VERSION_SOFTWARE_NAME) + " files").utf8().get_data());
			filter_zt->add_pattern("*.zyt");
			dialog.add_filter(filter_zt);

			if (song_path != String()) {
				dialog.set_filename(song_path.utf8().get_data());
			}

			int result = dialog.run();
			if (result == Gtk::RESPONSE_OK) {
				String path;
				path.parse_utf8(dialog.get_filename().c_str());
				if (path.to_lower().get_extension() != "zyt") {
					path += ".zyt";
				}
				Error err = song_file.save(path);
				if (err != OK) {
					Gtk::MessageDialog error_box("Error saving file", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
					error_box.set_transient_for(*this);
					error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
					error_box.run();
					error_box.hide();
				} else {
					song_path = path;
					save_version = undo_redo.get_current_version();
					_update_title();
				}
			}

			dialog.hide();
		} break;
		case KeyBindings::FILE_QUIT: {

			if (save_version != undo_redo.get_current_version()) {
				Gtk::MessageDialog error_box("There are unsaved changes.\nQuit anyway?", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
				error_box.set_transient_for(*this);
				error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

				int response = error_box.run();
				error_box.hide();
				if (response != Gtk::RESPONSE_OK) {
					break;
				}
			}

			application->quit();

		} break;
		case KeyBindings::FILE_EXPORT_WAV: {

			if (!song.can_play()) {
				Gtk::MessageDialog error_box("Songs without an order list can't be exported.", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
				error_box.set_transient_for(*this);
				error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
				error_box.run();
				error_box.hide();
				break;
			}
			Gtk::FileChooserDialog dialog("Select a Microsoft Waveform(tm) file to export",
					Gtk::FILE_CHOOSER_ACTION_SAVE);
			dialog.set_transient_for(*this);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

			//Add response buttons the the dialog:
			gboolean swap_buttons;
			g_object_get(gtk_settings_get_default(), "gtk-alternative-button-order", &swap_buttons, NULL);
			if (swap_buttons) {
				dialog.add_button("Select", Gtk::RESPONSE_OK);
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
			} else {
				dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
				dialog.add_button("Select", Gtk::RESPONSE_OK);
			}

			auto filter_zt = Gtk::FileFilter::create();
			filter_zt->set_name("Microsoft Waveform");
			filter_zt->add_pattern("*.wav");
			dialog.add_filter(filter_zt);

			if (last_wav_export_path != String()) {
				dialog.set_filename(last_wav_export_path.utf8().get_data());
			}

			int result = dialog.run();
			dialog.hide();

			if (result == Gtk::RESPONSE_OK) {

				String path;
				path.parse_utf8(dialog.get_filename().c_str());
				if (path.to_lower().get_extension() != "wav") {
					path += ".wav";
				}

				Error err;
				{
					Gtk::MessageDialog export_dialog("", false /* use_markup */, Gtk::MESSAGE_OTHER, Gtk::BUTTONS_NONE);
					export_dialog.get_vbox()->get_children()[0]->hide();
					export_dialog.get_vbox()->set_spacing(0);

					Gtk::Button *response_button = export_dialog.add_button("Close", Gtk::RESPONSE_OK);
					response_button->set_sensitive(false);

					Gtk::ProgressBar progress;
					export_dialog.get_vbox()->pack_start(progress, Gtk::PACK_EXPAND_WIDGET);
					export_dialog.get_vbox()->pack_start(export_wav_label, Gtk::PACK_EXPAND_WIDGET);

					Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
					int width = screen->get_width();
					int height = screen->get_height();
					export_dialog.set_default_size(width / 5, 1);

					export_dialog.show_all_children();
					export_dialog.get_vbox()->get_children()[0]->hide();
					export_dialog.set_deletable(false);
					export_dialog.set_transient_for(*this);
					export_dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
					export_dialog.set_title("Exporting Song..");
					export_dialog.signal_event().connect(sigc::mem_fun(*this, &Interface::_export_dialog_key));
					export_dialog.show();

					err = song_file.export_wav(path, 96000, _export_dialog_callback, &progress);

					response_button->set_sensitive(true);
					export_dialog.set_deletable(true);
					export_dialog.set_title("Exporting Done");

					export_wav_label.set_text("All Done!");
					export_dialog.run();
					export_dialog.hide();
				}

				if (err != OK) {
					Gtk::MessageDialog error_box("Error exporting file", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);
					error_box.set_transient_for(*this);
					error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
					error_box.run();
					error_box.hide();
				} else {
					last_wav_export_path = path;
				}
			}

		} break;

		case KeyBindings::PLAYBACK_PLAY: {

			song.play();

		} break;
		case KeyBindings::PLAYBACK_STOP: {
			song.stop();

		} break;
		case KeyBindings::PLAYBACK_NEXT_PATTERN: {
			song.play_next_pattern();
		} break;
		case KeyBindings::PLAYBACK_PREV_PATTERN: {
			song.play_prev_pattern();
		} break;
		case KeyBindings::PLAYBACK_PLAY_PATTERN: {
			song.play_pattern(pattern_editor.get_current_pattern());

		} break;
		case KeyBindings::PLAYBACK_PLAY_FROM_CURSOR: {
			int current_pattern = pattern_editor.get_current_pattern();
			int current_order = -1;
			for (int i = 0; i <= Song::ORDER_MAX; i++) {
				if (song.order_get(i) == current_pattern) {
					current_order = i;
					break;
				}
			}
			if (current_order >= 0) {
				song.play(current_order, pattern_editor.get_cursor_tick());
			} else {
				song.play_pattern(current_pattern, pattern_editor.get_cursor_tick());
			}

		} break;
		case KeyBindings::PLAYBACK_PLAY_FROM_ORDER: {
			song.play(orderlist_editor.get_cursor_order());

		} break;
		case KeyBindings::PLAYBACK_CURSOR_FOLLOW: {
			playback_cursor_follow = !playback_cursor_follow;
			key_bindings->set_action_checked(KeyBindings::PLAYBACK_CURSOR_FOLLOW, playback_cursor_follow);
		} break;

		case KeyBindings::EDIT_UNDO: {
			undo_redo.undo();
		} break;
		case KeyBindings::EDIT_REDO: {
			undo_redo.redo();
		} break;
		case KeyBindings::EDIT_SONG_INFO: {
			Gtk::MessageDialog info_box("", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);

			info_box.set_transient_for(*this);
			info_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			info_box.set_title("Song Information");

			info_box.get_vbox()->set_spacing(4);

			Gtk::Label label_name;
			label_name.set_text("Song Name:");
			info_box.get_vbox()->pack_start(label_name, Gtk::PACK_SHRINK);

			Gtk::Entry name;
			info_box.get_vbox()->pack_start(name, Gtk::PACK_SHRINK);
			name.set_text(song.get_name().utf8().get_data());

			Gtk::Label label_author;
			label_author.set_text("Author:");
			info_box.get_vbox()->pack_start(label_author, Gtk::PACK_SHRINK);

			Gtk::Entry author;
			info_box.get_vbox()->pack_start(author, Gtk::PACK_SHRINK);
			author.set_text(song.get_author().utf8().get_data());

			Gtk::Label label_description;
			label_description.set_text("Description:");
			info_box.get_vbox()->pack_start(label_description, Gtk::PACK_SHRINK);

			Gtk::ScrolledWindow window;
			Gtk::TextView description;
			info_box.get_vbox()->pack_start(window, Gtk::PACK_EXPAND_WIDGET);
			window.add(description);
			Glib::RefPtr<Gtk::TextBuffer> buffer = Gtk::TextBuffer::create();
			description.set_buffer(buffer);

			buffer->set_text(song.get_description().utf8().get_data());

			Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
			int width = screen->get_width();
			int height = screen->get_height();

			info_box.set_default_size(width / 5, height / 3);

			info_box.show_all_children();
			info_box.get_vbox()->get_children()[0]->hide();
			info_box.run();
			info_box.hide();

			String new_name;
			String new_author;
			String new_description;
			new_name.parse_utf8(name.get_text().c_str());
			new_author.parse_utf8(author.get_text().c_str());
			new_description.parse_utf8(buffer->get_text().c_str());

			if (new_name != song.get_name() || new_author != song.get_author() || new_description != song.get_description()) {
				undo_redo.begin_action("Change Song Description");
				undo_redo.do_method(&song, &Song::set_name, new_name);
				undo_redo.undo_method(&song, &Song::set_name, song.get_name());
				undo_redo.do_method(&song, &Song::set_author, new_author);
				undo_redo.undo_method(&song, &Song::set_author, song.get_author());
				undo_redo.do_method(&song, &Song::set_description, new_description);
				undo_redo.undo_method(&song, &Song::set_description, song.get_description());
				undo_redo.commit_action();
			}

		} break;
		case KeyBindings::EDIT_FOCUS_PATTERN: {
			pattern_editor.grab_focus();
		} break;
		case KeyBindings::EDIT_FOCUS_ORDERLIST: {
			orderlist_editor.grab_focus();
		} break;
		case KeyBindings::EDIT_FOCUS_LAST_EDITED_EFFECT: {
		} break;

		case KeyBindings::SETTINGS_OPEN: {
			settings_dialog.run();
			settings_dialog.hide();
		} break;
		case KeyBindings::SETTINGS_PATTERN_INPUT_KEYS: {
			String text;
			//text = "::Pattern Editing Cheatsheet::\n\n";

			text += "Navigation:\n\n";

			text += "Row Up / Down: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_MOVE_UP) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_MOVE_DOWN) + "\n";
			text += "Field Left / Right: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_MOVE_LEFT) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_MOVE_RIGHT) + "\n";
			text += "Next/Prev Column: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_TAB) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_BACKTAB) + "\n";
			text += "Page Up / Down: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_PAGE_UP) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_PAGE_DOWN) + "\n";
			text += "First/Last Track: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_HOME) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_END) + "\n";
			text += "Scroll Up / Down: " + key_bindings->get_keybind_text(KeyBindings::PATTERN_PAN_WINDOW_UP) + ", " + key_bindings->get_keybind_text(KeyBindings::PATTERN_PAN_WINDOW_DOWN) + "\n";

			text += "\n\nEditing:\n\n";

			text += "Insert/Delete Row: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_INSERT) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_DELETE) + "\n";
			text += "Clear Field: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_INSERT) + ", " + key_bindings->get_keybind_text(KeyBindings::CURSOR_DELETE) + "\n";
			text += "Raise/Lower Octave: " + key_bindings->get_keybind_text(KeyBindings::PATTERN_OCTAVE_RAISE) + ", " + key_bindings->get_keybind_text(KeyBindings::PATTERN_OCTAVE_LOWER) + "\n";
			text += "Copy Volume Mask: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_COPY_VOLUME_MASK) + "\n";
			text += "Toggle Volume Mask: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_TOGGLE_VOLUME_MASK) + "\n";
			text += "Play Note: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_PLAY_NOTE) + "\n";
			text += "Play Row: " + key_bindings->get_keybind_text(KeyBindings::CURSOR_PLAY_ROW) + "\n";
			text += "Insert Note Off: " + key_bindings->get_keybind_text(KeyBindings::PATTERN_CURSOR_NOTE_OFF) + "\n";

			text += "\n\nVirtual Piano Lower Row (C0 -> B0):\n\n";
			for (int i = 0; i < 12; i++) {
				if (i > 0) {
					text += ", ";
				}
				text += key_bindings->get_keybind_text(KeyBindings::KeyBind(KeyBindings::PIANO_C0 + i));
			}

			text += "\n\nVirtual Piano Upper Row (C1 -> E2):\n\n";
			for (int i = 0; i < 17; i++) {
				if (i > 0) {
					text += ", ";
				}
				text += key_bindings->get_keybind_text(KeyBindings::KeyBind(KeyBindings::PIANO_C1 + i));
			}

			Gtk::MessageDialog about_box(text.ascii().get_data(), false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);

			about_box.set_transient_for(*this);
			about_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

			about_box.set_title("Pattern Editing Cheatsheet");
			about_box.run();
			about_box.hide();

		} break;
		case KeyBindings::SETTINGS_ABOUT: {
			Gtk::MessageDialog about_box(Glib::ustring(VERSION_WITH_COPYRIGHT) + "\nhttp://zytrax.org", false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_CLOSE);

			about_box.set_transient_for(*this);
			about_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			about_box.set_title("About");
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
}
void Interface::_update_tracks() {

	double previous_scroll = track_scroll.get_hadjustment()->get_value();
	int current_track = pattern_editor.get_current_track();

	Map<Track *, int> offsets;

	for (int i = 0; i < racks.size(); i++) {

		offsets[racks[i].rack->get_track()] = racks[i].rack->get_v_offset();
		delete racks[i].rack;
		delete racks[i].volume;
	}

	if (rack_filler) {
		delete rack_filler;
		rack_filler = NULL;
	}
	racks.clear();
	for (int i = 0; i < song.get_track_count(); i++) {
		TrackRacks rack;
		rack.volume = new TrackRackVolume(i, &song, &undo_redo, theme, key_bindings);
		rack.v_scroll = new Gtk::VScrollbar;
		rack.rack = new TrackRackEditor(i, &song, &undo_redo, theme, key_bindings, rack.v_scroll);
		rack.rack->add_effect.connect(sigc::mem_fun(*this, &Interface::_on_add_effect));
		rack.rack->toggle_effect_skip.connect(sigc::mem_fun(*this, &Interface::_on_toggle_effect_skip));
		rack.rack->toggle_send_mute.connect(sigc::mem_fun(*this, &Interface::_on_toggle_send_mute));
		rack.rack->remove_effect.connect(sigc::mem_fun(*this, &Interface::_on_remove_effect));
		rack.rack->remove_send.connect(sigc::mem_fun(*this, &Interface::_on_remove_send));
		rack.rack->insert_send_to_track.connect(sigc::mem_fun(*this, &Interface::_on_track_insert_send));
		rack.rack->send_amount_changed.connect(sigc::mem_fun(*this, &Interface::_on_track_send_amount_changed));
		rack.rack->track_swap_effects.connect(sigc::mem_fun(*this, &Interface::_on_track_swap_effects));
		rack.rack->track_swap_sends.connect(sigc::mem_fun(*this, &Interface::_on_track_swap_sends));
		rack.rack->effect_request_editor.connect(sigc::mem_fun(*this, &Interface::_on_effect_request_editor));
		rack.volume->volume_db_changed.connect(sigc::mem_fun(*this, &Interface::_on_track_volume_changed));

		if (offsets.has(song.get_track(i))) {
			rack.rack->set_v_offset(offsets[song.get_track(i)]);
		}
		track_hbox.pack_start(*rack.volume, Gtk::PACK_SHRINK);
		track_hbox.pack_start(*rack.rack, Gtk::PACK_SHRINK);
		track_hbox.pack_start(*rack.v_scroll, Gtk::PACK_SHRINK);
		rack.rack->set_selected(i == current_track);
		rack.volume->set_selected(i == current_track);
		rack.volume->show();
		rack.rack->show();
		racks.push_back(rack);
	}

	rack_filler = new TrackRackFiller(theme);
	rack_filler->show();
	track_hbox.pack_start(*rack_filler, Gtk::PACK_EXPAND_WIDGET);

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

void Interface::_on_add_effect(int p_track) {

	add_effect_dialog.set_transient_for(*this);
	add_effect_dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	add_effect_dialog.update_effect_list();
	if (add_effect_dialog.run() == Gtk::RESPONSE_OK) {

		int idx = add_effect_dialog.get_selected_effect_index();
		ERR_FAIL_COND(idx == -1);
		AudioEffect *effect = fx_factory->instantiate_effect(idx);
		ERR_FAIL_COND(!effect); //cant create

		{
			//configure default commands if they exist
			for (int j = 0; j < effect->get_control_port_count(); j++) {
				ControlPort *cp = effect->get_control_port(j);

				for (int i = 0; i < SettingsDialog::MAX_DEFAULT_COMMANDS; i++) {
					if (SettingsDialog::get_default_command_name(i) == cp->get_identifier()) {
						cp->set_command(SettingsDialog::get_default_command_command(i));
						break;
					}
				}
			}
		}

		undo_redo.begin_action("Create Effect: " + effect->get_name());
		Track *track = song.get_track(p_track);
		undo_redo.do_method(track, &Track::add_audio_effect, effect, -1);
		undo_redo.undo_method(track, &Track::remove_audio_effect, track->get_audio_effect_count());
		undo_redo.do_data(effect);
		undo_redo.do_method(this, &Interface::_update_tracks);
		undo_redo.undo_method(this, &Interface::_update_tracks);
		undo_redo.commit_action();
	}
	add_effect_dialog.hide();
}

void Interface::_redraw_track_edits() {

	for (int i = 0; i < racks.size(); i++) {
		racks[i].rack->queue_draw();
		racks[i].volume->queue_draw();
	}

	main_vu.queue_draw();
}
void Interface::_on_toggle_effect_skip(int p_track, int p_effect) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	ERR_FAIL_INDEX(p_effect, song.get_track(p_track)->get_audio_effect_count());

	undo_redo.begin_action("Toggle Effect Skip");
	bool skip = song.get_track(p_track)->get_audio_effect(p_effect)->is_skipped();
	undo_redo.do_method(song.get_track(p_track)->get_audio_effect(p_effect), &AudioEffect::set_skip, !skip);
	undo_redo.undo_method(song.get_track(p_track)->get_audio_effect(p_effect), &AudioEffect::set_skip, skip);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}
void Interface::_on_toggle_send_mute(int p_track, int p_send) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	ERR_FAIL_INDEX(p_send, song.get_track(p_track)->get_send_count());

	undo_redo.begin_action("Toggle Send Mute");
	bool mute = song.get_track(p_track)->is_send_muted(p_send);
	undo_redo.do_method(song.get_track(p_track), &Track::set_send_mute, p_send, !mute);
	undo_redo.undo_method(song.get_track(p_track), &Track::set_send_mute, p_send, mute);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}
void Interface::_on_remove_effect(int p_track, int p_effect) {
	ERR_FAIL_INDEX(p_track, song.get_track_count());
	ERR_FAIL_INDEX(p_effect, song.get_track(p_track)->get_audio_effect_count());

	undo_redo.begin_action("Remove Effect");
	undo_redo.do_method(song.get_track(p_track), &Track::remove_audio_effect, p_effect);
	AudioEffect *effect = song.get_track(p_track)->get_audio_effect(p_effect);
	undo_redo.undo_method(song.get_track(p_track), &Track::add_audio_effect, effect, p_effect);
	undo_redo.undo_data(effect);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}
void Interface::_on_remove_send(int p_track, int p_send) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	ERR_FAIL_INDEX(p_send, song.get_track(p_track)->get_send_count());

	undo_redo.begin_action("Remove Send");
	Track *track = song.get_track(p_track);
	undo_redo.do_method(track, &Track::remove_send, p_send);
	undo_redo.undo_method(track, &Track::add_send, track->get_send_track(p_send), p_send);
	undo_redo.undo_method(track, &Track::set_send_amount, p_send, track->get_send_amount(p_send));
	undo_redo.undo_method(track, &Track::set_send_mute, p_send, track->is_send_muted(p_send));
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_update_song_process_order() {

	song.update_process_order();
}

void Interface::_on_track_insert_send(int p_track, int p_to_track) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	ERR_FAIL_COND(p_to_track != Track::SEND_SPEAKERS && (p_to_track < 0 || p_to_track >= song.get_track_count()));
	ERR_FAIL_COND(p_to_track == p_track);

	//first try this
	Track *track = song.get_track(p_track);
	SoundDriverManager::lock_driver();
	track->add_send(p_to_track);
	bool valid = song.update_process_order();
	track->remove_send(track->get_send_count() - 1);
	SoundDriverManager::unlock_driver();
	if (!valid) {
		Gtk::MessageDialog error_box("Unable to add send, cyclic reference?", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
		error_box.set_transient_for(*this);
		error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
		error_box.run();
		error_box.hide();

		return;
	}

	undo_redo.begin_action("Add Send");
	undo_redo.do_method(track, &Track::add_send, p_to_track, -1);
	undo_redo.undo_method(track, &Track::remove_send, track->get_send_count());
	undo_redo.do_method(this, &Interface::_update_song_process_order);
	undo_redo.undo_method(this, &Interface::_update_song_process_order);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_on_track_send_amount_changed(int p_track, int p_send, float p_amount) {
	ERR_FAIL_INDEX(p_track, song.get_track_count());
	Track *track = song.get_track(p_track);
	ERR_FAIL_INDEX(p_send, track->get_send_count());

	undo_redo.begin_action("Set Send Amount", true);
	undo_redo.do_method(track, &Track::set_send_amount, p_send, p_amount);
	undo_redo.undo_method(track, &Track::set_send_amount, p_send, track->get_send_amount(p_send));
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_on_track_swap_effects(int p_track, int p_effect, int p_with_effect) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	Track *track = song.get_track(p_track);
	undo_redo.begin_action("Swap Effects");
	undo_redo.do_method(track, &Track::swap_audio_effects, p_effect, p_with_effect);
	undo_redo.undo_method(track, &Track::swap_audio_effects, p_effect, p_with_effect);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}
void Interface::_on_track_swap_sends(int p_track, int p_send, int p_with_send) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	Track *track = song.get_track(p_track);

	undo_redo.begin_action("Swap Sends");
	undo_redo.do_method(track, &Track::swap_sends, p_send, p_with_send);
	undo_redo.undo_method(track, &Track::swap_sends, p_send, p_with_send);
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_on_track_volume_changed(int p_track, float p_volume_db) {

	ERR_FAIL_INDEX(p_track, song.get_track_count());
	Track *track = song.get_track(p_track);

	undo_redo.begin_action("Change Track #" + String::num(p_track) + " volume.", true);
	undo_redo.do_method(track, &Track::set_mix_volume_db, p_volume_db);
	undo_redo.undo_method(track, &Track::set_mix_volume_db, track->get_mix_volume_db());
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_on_effect_request_editor(int p_track, int p_effect) {
	ERR_FAIL_INDEX(p_track, song.get_track_count());
	Track *track = song.get_track(p_track);
	ERR_FAIL_INDEX(p_effect, track->get_audio_effect_count());
	AudioEffect *effect = track->get_audio_effect(p_effect);

	if (effect->get_name() == "DummyPlugin") {
		//do not attempt editing dummy plugins
		Gtk::MessageDialog error_box("This is a placeholder for a missing plugin.\nIt can't be edited.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
		error_box.set_transient_for(*this);
		error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
		error_box.run();
		error_box.hide();
		return;
	}

	if (!active_effect_editors.has(effect)) {
		//create a new editor
		EffectEditor *effect_editor = new EffectEditor;

		Gtk::Widget *editor = NULL;
		for (int i = plugin_editor_function_count - 1; i >= 0; i--) {
			editor = plugin_editor_create_functions[i](effect, effect_editor);
			if (editor) {
				break;
			}
		}

		if (!editor) {
			delete editor;
			ERR_FAIL_COND(!editor);
		}

		effect_editor->edit(effect, track, editor);
		effect_editor->toggle_automation_visibility.connect(sigc::mem_fun(*this, &Interface::_on_toggle_automation_visibility));
		effect_editor->select_automation_command.connect(sigc::mem_fun(*this, &Interface::_on_select_automation_command));

		active_effect_editors[effect] = effect_editor;
	}

	active_effect_editors[effect]->show();
}

void Interface::_update_editor_automations_for_effect(AudioEffect *p_effect) {
	if (active_effect_editors.has(p_effect)) {
		active_effect_editors[p_effect]->update_automations();
	}
	pattern_editor.redraw_and_validate_cursor();
}

void Interface::_on_select_automation_command(Track *p_track, AudioEffect *p_effect, int p_automation, int p_command) {

	ControlPort *port = p_effect->get_control_port(p_automation);

	undo_redo.begin_action("Change Automation Command");
	undo_redo.do_method(port, &ControlPort::set_command, (char)p_command);
	undo_redo.undo_method(port, &ControlPort::set_command, port->get_command());
	undo_redo.do_method(this, &Interface::_update_editor_automations_for_effect, p_effect);
	undo_redo.undo_method(this, &Interface::_update_editor_automations_for_effect, p_effect);
	undo_redo.commit_action();
}

void Interface::_on_toggle_automation_visibility(Track *p_track, AudioEffect *p_effect, int p_automation, bool p_visible) {

	if (p_visible) {

		undo_redo.begin_action("Create Automation");

		int disabled_index = -1;
		for (int i = 0; i < p_track->get_disabled_automation_count(); i++) {
			Automation *a = p_track->get_disabled_automation(i);
			if (a->get_control_port() == p_effect->get_control_port(p_automation)) {
				disabled_index = i;
				break;
			}
		}

		if (disabled_index >= 0) { //move from disabled to enabled
			Automation *a = p_track->get_disabled_automation(disabled_index);
			undo_redo.do_method(p_track, &Track::remove_disabled_automation, disabled_index);
			undo_redo.do_method(p_track, &Track::add_automation, a, -1);

			undo_redo.undo_method(p_track, &Track::remove_automation, p_track->get_automation_count());
			undo_redo.undo_method(p_track, &Track::add_disabled_automation, a, disabled_index);
		} else {
			Automation *a = new Automation(p_effect->get_control_port(p_automation), p_effect);
			undo_redo.do_method(p_track, &Track::add_automation, a, -1);
			undo_redo.undo_method(p_track, &Track::remove_automation, p_track->get_automation_count());
			undo_redo.do_data(a);
		}

		undo_redo.do_method(this, &Interface::_update_editor_automations_for_effect, p_effect);
		undo_redo.undo_method(this, &Interface::_update_editor_automations_for_effect, p_effect);

		undo_redo.commit_action();

	} else {

		int index = -1;
		for (int i = 0; i < p_track->get_automation_count(); i++) {
			Automation *a = p_track->get_automation(i);
			if (a->get_control_port() == p_effect->get_control_port(p_automation)) {
				index = i;
				break;
			}
		}

		ERR_FAIL_COND(index == -1);

		Automation *a = p_track->get_automation(index);

		undo_redo.begin_action("Remove Automation");
		undo_redo.do_method(p_track, &Track::remove_automation, index);

		if (!a->is_empty()) {
			undo_redo.do_method(p_track, &Track::add_disabled_automation, a, -1);
			undo_redo.undo_method(p_track, &Track::remove_disabled_automation, p_track->get_disabled_automation_count());
		}
		undo_redo.undo_method(p_track, &Track::add_automation, a, index);
		undo_redo.do_method(this, &Interface::_update_editor_automations_for_effect, p_effect);
		undo_redo.undo_method(this, &Interface::_update_editor_automations_for_effect, p_effect);

		undo_redo.commit_action();
	}
}

void Interface::_on_application_startup() {
	key_bindings->initialize(application, this);

	menu = Gio::Menu::create();

	file_menu = Gio::Menu::create();
	file_menu_file = Gio::Menu::create();
	file_menu->append_section(file_menu_file);
	file_menu_file->append("New", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_NEW).ascii().get_data());
	file_menu_file->append("Open", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_OPEN).ascii().get_data());
	file_menu_file->append("Save", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_SAVE).ascii().get_data());
	file_menu_file->append("Save As", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_SAVE_AS).ascii().get_data());
	file_menu_export = Gio::Menu::create();
	file_menu->append_section(file_menu_export);
	file_menu_export->append("Export to WAV", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_EXPORT_WAV).ascii().get_data());
	file_menu_exit = Gio::Menu::create();
	file_menu->append_section(file_menu_exit);
	file_menu_exit->append("Quit", key_bindings->get_keybind_detailed_name(KeyBindings::FILE_QUIT).ascii().get_data());
	menu->append_submenu("File", file_menu);

	play_menu = Gio::Menu::create();
	play_menu_play = Gio::Menu::create();
	play_menu->append_section(play_menu_play);
	play_menu_play->append("Play Song", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY).ascii().get_data());
	play_menu_play->append("Stop", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_STOP).ascii().get_data());
	play_menu_seek = Gio::Menu::create();
	play_menu->append_section(play_menu_seek);
	play_menu_seek->append("Skip to Next Pattern", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_NEXT_PATTERN).ascii().get_data());
	play_menu_seek->append("Skip to Prev Pattern", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_PREV_PATTERN).ascii().get_data());
	play_menu_pattern = Gio::Menu::create();
	play_menu->append_section(play_menu_pattern);
	play_menu_pattern->append("Play Current Pattern", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_PATTERN).ascii().get_data());
	play_menu_pattern->append("Play From Cursor", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_FROM_CURSOR).ascii().get_data());
	play_menu_pattern->append("Play From Current Order", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_PLAY_FROM_ORDER).ascii().get_data());
	play_menu_extra = Gio::Menu::create();
	play_menu->append_section(play_menu_extra);
	play_menu_extra->append("Toggle Follow Song", key_bindings->get_keybind_detailed_name(KeyBindings::PLAYBACK_CURSOR_FOLLOW).ascii().get_data());

	menu->append_submenu("Play", play_menu);

	edit_menu = Gio::Menu::create();
	edit_menu_undo = Gio::Menu::create();
	edit_menu->append_section(edit_menu_undo);
	edit_menu_undo->append("Undo", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_UNDO).ascii().get_data());
	edit_menu_undo->append("Redo", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_REDO).ascii().get_data());
	edit_menu_info = Gio::Menu::create();
	edit_menu->append_section(edit_menu_info);
	edit_menu_info->append("Song Information", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_SONG_INFO).ascii().get_data());
	edit_menu_focus = Gio::Menu::create();
	edit_menu->append_section(edit_menu_focus);
	edit_menu_focus->append("Focus on Pattern", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_PATTERN).ascii().get_data());
	edit_menu_focus->append("Focus on Orderlist", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_ORDERLIST).ascii().get_data());
	edit_menu_focus->append("Open Last Edited Effect", key_bindings->get_keybind_detailed_name(KeyBindings::EDIT_FOCUS_LAST_EDITED_EFFECT).ascii().get_data());
	menu->append_submenu("Edit", edit_menu);

	select_menu = Gio::Menu::create();
	select_menu_select = Gio::Menu::create();
	select_menu->append_section(select_menu_select);
	select_menu_select->append("Begin", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_BEGIN).ascii().get_data());
	select_menu_select->append("End", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_END).ascii().get_data());
	select_menu_select->append("Column/Track/Pattern", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECT_COLUMN_TRACK_ALL).ascii().get_data());
	select_menu_select->append("Clear", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_DISABLE).ascii().get_data());
	select_menu_clipboard = Gio::Menu::create();
	select_menu->append_section(select_menu_clipboard);
	select_menu_clipboard->append("Cut", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_CUT).ascii().get_data());
	select_menu_clipboard->append("Copy", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_COPY).ascii().get_data());
	select_menu_clipboard->append("Paste Insert", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_INSERT).ascii().get_data());
	select_menu_clipboard->append("Paste Overwrite", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_OVERWRITE).ascii().get_data());
	select_menu_clipboard->append("Paste Mix", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_PASTE_MIX).ascii().get_data());
	select_menu_transpose = Gio::Menu::create();
	select_menu->append_section(select_menu_transpose);
	select_menu_transpose->append("Raise Note(s) a Semitone", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_SEMITONE).ascii().get_data());
	select_menu_transpose->append("Raise Note(s) an Octave", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_RAISE_NOTES_OCTAVE).ascii().get_data());
	select_menu_transpose->append("Lower Note(s) a Semitone", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_SEMITONE).ascii().get_data());
	select_menu_transpose->append("Lower Note(s) an Octave", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_LOWER_NOTES_OCTAVE).ascii().get_data());
	select_menu_operations = Gio::Menu::create();
	select_menu->append_section(select_menu_operations);
	select_menu_operations->append("Set Volume Mask", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_SET_VOLUME).ascii().get_data());
	select_menu_operations->append("Interpolate Volume or Automation", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION).ascii().get_data());
	select_menu_operations->append("Amplify Volume or Automation", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION).ascii().get_data());
	select_menu_length = Gio::Menu::create();
	select_menu->append_section(select_menu_length);
	select_menu_length->append("Double Length", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_DOUBLE_LENGTH).ascii().get_data());
	select_menu_length->append("Halve Length", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_HALVE_LENGTH).ascii().get_data());
	select_menu_length->append("Scale Length", key_bindings->get_keybind_detailed_name(KeyBindings::PATTERN_SELECTION_SCALE_LENGTH).ascii().get_data());
	menu->append_submenu("Selection", select_menu);

	settings_menu = Gio::Menu::create();
	settings_menu_preferences = Gio::Menu::create();
	settings_menu->append_section(settings_menu_preferences);
	settings_menu_preferences->append("Preferences", key_bindings->get_keybind_detailed_name(KeyBindings::SETTINGS_OPEN).ascii().get_data());
	settings_menu_cheat = Gio::Menu::create();
	settings_menu->append_section(settings_menu_cheat);
	settings_menu_cheat->append("Pattern Editing Cheat-Sheet", key_bindings->get_keybind_detailed_name(KeyBindings::SETTINGS_PATTERN_INPUT_KEYS).ascii().get_data());
	settings_menu_about = Gio::Menu::create();
	settings_menu->append_section(settings_menu_about);
	settings_menu_about->append("About", key_bindings->get_keybind_detailed_name(KeyBindings::SETTINGS_ABOUT).ascii().get_data());
	menu->append_submenu("Settings", settings_menu);

	application->set_menubar(menu);

	key_bindings->action_activated.connect(sigc::mem_fun(*this, &Interface::_on_action_activated));

	settings_dialog.initialize_bindings();
	pattern_editor.initialize_menus();
}

void Interface::add_editor_plugin_function(EffectEditorPluginFunc p_plugin) {
	ERR_FAIL_COND(plugin_editor_function_count == MAX_EFFECT_EDITOR_PLUGINS);
	plugin_editor_create_functions[plugin_editor_function_count++] = p_plugin;
}

void Interface::_on_pattern_settings_open() {
	pattern_settings_popover.popup();
	pattern_settings_length.get_adjustment()->set_value(song.pattern_get_beats(pattern_editor.get_current_pattern()));
	bar_length.get_adjustment()->set_value(song.pattern_get_beats_per_bar(pattern_editor.get_current_pattern()));
	change_swing.set_active(song.pattern_get_swing_beat_divisor(pattern_editor.get_current_pattern()));
	change_next.get_adjustment()->set_value(1.0);
}
void Interface::_on_pattern_settings_change() {

	undo_redo.begin_action("Change Pattern(s)");
	int patterns = change_next.get_adjustment()->get_value();
	int beats = pattern_settings_length.get_adjustment()->get_value();
	int beats_per_bar = bar_length.get_adjustment()->get_value();
	Song::SwingBeatDivisor swing_divisor = Song::SWING_BEAT_DIVISOR_1;

	Gtk::TreeModel::iterator iter = change_swing.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[zoom_model_columns.index];

			swing_divisor = Song::SwingBeatDivisor(id);
		}
	}

	for (int i = 0; i < patterns; i++) {
		int pattern = pattern_editor.get_current_pattern() + i;
		undo_redo.do_method(&song, &Song::pattern_set_beats, pattern, beats);
		undo_redo.undo_method(&song, &Song::pattern_set_beats, pattern, song.pattern_get_beats(pattern));
		undo_redo.do_method(&song, &Song::pattern_set_beats_per_bar, pattern, beats_per_bar);
		undo_redo.undo_method(&song, &Song::pattern_set_beats_per_bar, pattern, song.pattern_get_beats_per_bar(pattern));
		undo_redo.do_method(&song, &Song::pattern_set_swing_beat_divisor, pattern, swing_divisor);
		undo_redo.undo_method(&song, &Song::pattern_set_swing_beat_divisor, pattern, song.pattern_get_swing_beat_divisor(pattern));

		undo_redo.do_method(&pattern_editor, &PatternEditor::redraw_and_validate_cursor);
		undo_redo.undo_method(&pattern_editor, &PatternEditor::redraw_and_validate_cursor);
	}
	undo_redo.commit_action();
	pattern_settings_popover.popdown();
}

void Interface::_update_colors() {
	pattern_editor.queue_draw();
	orderlist_editor.queue_draw();
	if (rack_filler) {
		rack_filler->queue_draw();
	}
	for (int i = 0; i < racks.size(); i++) {
		racks[i].rack->queue_draw();
		racks[i].volume->queue_draw();
	}
}

void Interface::_undo_redo_action(const String &p_name, void *p_userdata) {
	Interface *interface = (Interface *)p_userdata;
	interface->_update_title();
}

bool Interface::_close_request(GdkEventAny *event) {

	_on_action_activated(KeyBindings::FILE_QUIT);
	return true;
}

void Interface::_process_audio(AudioFrame *p_frames, int p_amount) {

	singleton->song.process_audio(p_frames, p_amount);
}

Interface *Interface::singleton = NULL;

bool Interface::_playback_timer_callback() {
	if (!song.is_playing()) {
		pattern_editor.set_playback_pos(-1, -1);
		orderlist_editor.set_playback_order(-1);

	} else {
		pattern_editor.set_playback_pos(song.get_playing_pattern(), song.get_playing_tick());

		orderlist_editor.set_playback_order(song.get_playing_order());
		if (playback_cursor_follow) {
			pattern_editor.set_playback_cursor(song.get_playing_pattern(), song.get_playing_tick());
		}
	}

	for (int i = 0; i < racks.size(); i++) {
		racks[i].volume->update_peak();
	}

	main_vu.update_peak();

	return true;
}

void Interface::_on_main_volume_db_changed(float p_db) {

	undo_redo.begin_action("Change Master Volume", true);
	undo_redo.do_method(&song, &Song::set_main_volume_db, p_db);
	undo_redo.undo_method(&song, &Song::set_main_volume_db, song.get_main_volume_db());
	undo_redo.do_method(this, &Interface::_redraw_track_edits);
	undo_redo.undo_method(this, &Interface::_redraw_track_edits);
	undo_redo.commit_action();
}

void Interface::_on_song_step_buffer_changed() {
	int step_size = SoundDriverManager::get_buffer_size_frames(SoundDriverManager::get_step_buffer_size());
	song.set_process_buffer_size(step_size);
}

void Interface::_on_song_mix_rate_changed() {

	int mix_rate = SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::get_mix_frequency());
	song.set_sampling_rate(mix_rate);
}
void Interface::_update_song_mixing_parameters() {
	song.set_sampling_rate(SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::get_mix_frequency()));
	song.set_process_buffer_size(SoundDriverManager::get_buffer_size_frames(SoundDriverManager::get_step_buffer_size()));
}
Interface::Interface(Gtk::Application *p_application, AudioEffectFactory *p_fx_factory, Theme *p_theme, KeyBindings *p_key_bindings) :
		add_effect_dialog(p_fx_factory),
		song_file(&song, p_fx_factory),
		pattern_editor(&song, &undo_redo, p_theme, p_key_bindings),
		orderlist_editor(&song, &undo_redo, p_theme, p_key_bindings),
		settings_dialog(p_theme, p_key_bindings, p_fx_factory),
		main_vu(&song, &undo_redo, p_theme) {

	theme = p_theme;
	key_bindings = p_key_bindings;
	plugin_editor_function_count = 0;
	application = p_application;
	application->signal_startup().connect(sigc::mem_fun(*this, &Interface::_on_application_startup));

	updating_editors = true;

	fx_factory = p_fx_factory;
	add(main_vbox);

	main_vbox.pack_start(grid, Gtk::PACK_SHRINK);

	grid.set_column_spacing(4);
	grid.set_row_spacing(4);
	grid.attach(play_hbox, 0, 0, 1, 1);

	prev_pattern_icon = create_image_from_icon("PrevPattern");
	prev_pattern.set_image(prev_pattern_icon);
	play_hbox.pack_start(prev_pattern, Gtk::PACK_SHRINK);
	prev_pattern.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_PREV_PATTERN));

	play_icon = create_image_from_icon("Play");
	play.set_image(play_icon);
	play_hbox.pack_start(play, Gtk::PACK_SHRINK);
	play.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_PLAY));

	stop_icon = create_image_from_icon("Stop");
	stop.set_image(stop_icon);
	play_hbox.pack_start(stop, Gtk::PACK_SHRINK);
	stop.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_STOP));

	next_pattern_icon = create_image_from_icon("NextPattern");
	next_pattern.set_image(next_pattern_icon);
	play_hbox.pack_start(next_pattern, Gtk::PACK_SHRINK);
	next_pattern.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_NEXT_PATTERN));

	sep1.set_text("    ");
	play_hbox.pack_start(sep1, Gtk::PACK_SHRINK);

	play_pattern_icon = create_image_from_icon("PlayPattern");
	play_pattern.set_image(play_pattern_icon);
	play_hbox.pack_start(play_pattern, Gtk::PACK_SHRINK);
	play_pattern.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_PLAY_PATTERN));

	play_cursor_icon = create_image_from_icon("PlayFromCursor");
	play_cursor.set_image(play_cursor_icon);
	play_hbox.pack_start(play_cursor, Gtk::PACK_SHRINK);
	play_cursor.signal_clicked().connect(sigc::bind<KeyBindings::KeyBind>(sigc::mem_fun(*this, &Interface::_on_action_activated), KeyBindings::PLAYBACK_PLAY_FROM_CURSOR));

	sep2.set_text("    ");
	play_hbox.pack_start(sep2, Gtk::PACK_SHRINK);

	add_track_icon = create_image_from_icon("AddTrack");
	add_track.set_image(add_track_icon);
	add_track.set_label(" New Track");
	add_track.set_always_show_image(true);
	play_hbox.pack_start(add_track, Gtk::PACK_SHRINK);
	add_track.signal_clicked().connect(sigc::mem_fun(*this, &Interface::_add_track));

	//play_hbox.pack_start(spacer1, Gtk::PACK_EXPAND_WIDGET);

	grid.attach(main_vu, 1, 0, 3, 1);
	main_vu.main_volume_db_changed.connect(sigc::mem_fun(*this, &Interface::_on_main_volume_db_changed));

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
	pattern_settings.signal_clicked().connect(sigc::mem_fun(*this, &Interface::_on_pattern_settings_open));

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
	main_hbox.pack_start(pattern_vscroll, Gtk::PACK_SHRINK);
	main_hbox.pack_start(orderlist_editor, Gtk::PACK_SHRINK);
	main_hbox.pack_start(orderlist_vscroll, Gtk::PACK_SHRINK);

	pattern_vbox.pack_start(pattern_hscroll, Gtk::PACK_SHRINK);

	main_split.pack2(track_scroll, false, false);
	track_scroll.add(track_hbox);
	track_scroll.set_propagate_natural_height(true);
	track_scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_NEVER);

	//pattern_editor.track_edited.connect(sigc::mem_fun(this, &Interface::_track_edited));
	pattern_editor.track_layout_changed.connect(sigc::mem_fun(this, &Interface::_update_tracks));
	pattern_editor.current_track_changed.connect(sigc::mem_fun(this, &Interface::_update_selected_track));
	pattern_editor.volume_mask_changed.connect(sigc::mem_fun(this, &Interface::_update_volume_mask));
	pattern_editor.octave_changed.connect(sigc::mem_fun(this, &Interface::_update_octave));
	pattern_editor.pattern_changed.connect(sigc::mem_fun(this, &Interface::_update_pattern));
	pattern_editor.step_changed.connect(sigc::mem_fun(this, &Interface::_update_step));
	pattern_editor.zoom_changed.connect(sigc::mem_fun(this, &Interface::_update_zoom));

	pattern_editor.set_hscroll(pattern_hscroll.get_adjustment());
	pattern_editor.set_vscroll(pattern_vscroll.get_adjustment());

	orderlist_editor.set_vscroll(orderlist_vscroll.get_adjustment());

	{

		change_swing_store = Gtk::ListStore::create(zoom_model_columns);
		change_swing.set_model(change_swing_store);

		const char *swing_divisor[Song::SWING_BEAT_DIVISOR_MAX] = {
			"1 Beat",
			"1/2 Beat",
			"1/3 Beat",
			"1/4 Beat",
			"1/6 Beat",
			"1/8 Beat"
		};

		for (int i = 0; i < Song::SWING_BEAT_DIVISOR_MAX; i++) {
			Gtk::TreeModel::Row row = *(change_swing_store->append());
			row[zoom_model_columns.name] = swing_divisor[i];
			row[zoom_model_columns.index] = i;
			swing_rows.push_back(row);
		}

		change_swing.pack_start(zoom_model_columns.name);
	}
	pattern_settings_popover.add(pattern_settings_grid);

	pattern_settings_length_label.set_text("  Pattern Length (beats):");
	pattern_settings_grid.attach(pattern_settings_length_label, 0, 0, 1, 1);
	pattern_settings_length.set_adjustment(Gtk::Adjustment::create(16, 4, 128));
	pattern_settings_grid.attach(pattern_settings_length, 1, 0, 1, 1);
	bar_length_label.set_text("Bar Length (beats):");
	pattern_settings_grid.attach(bar_length_label, 0, 1, 1, 1);
	bar_length.set_adjustment(Gtk::Adjustment::create(4, 2, 16));
	pattern_settings_grid.attach(bar_length, 1, 1, 1, 1);

	change_swing_label.set_text("Swing:");
	pattern_settings_grid.attach(change_swing_label, 0, 2, 1, 1);
	pattern_settings_grid.attach(change_swing, 1, 2, 1, 1);

	//pattern_settings_vsep.set_text("sas  ");
	pattern_settings_vsep.set_size_request(20, 2);
	pattern_settings_grid.attach(pattern_settings_vsep, 0, 3, 2, 1);
	change_next_label.set_text("Patterns to Apply:");
	pattern_settings_grid.attach(change_next_label, 0, 4, 1, 1);
	change_next.set_adjustment(Gtk::Adjustment::create(1, 1, 999));
	pattern_settings_grid.attach(change_next, 1, 4, 1, 1);
	pattern_settings_grid.attach(pattern_settings_change_button, 0, 5, 2, 1);
	pattern_settings_change_button.set_label("Change");
	pattern_settings_change_button.signal_clicked().connect(sigc::mem_fun(*this, &Interface::_on_pattern_settings_change));
	pattern_settings_popover.set_relative_to(pattern_settings);
	pattern_settings_popover.set_position(Gtk::POS_BOTTOM);
	pattern_settings_popover.show_all_children();

	settings_dialog.update_colors.connect(sigc::mem_fun(*this, &Interface::_update_colors));
	settings_dialog.set_transient_for(*this);
	settings_dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	settings_dialog.update_song_step_buffer.connect(sigc::mem_fun(*this, &Interface::_on_song_step_buffer_changed));
	settings_dialog.update_mix_rate.connect(sigc::mem_fun(*this, &Interface::_on_song_mix_rate_changed));

	show_all_children();
	rack_filler = NULL;

	updating_editors = false;

	save_version = 0;

	//pattern_editor.init();
	_update_editors();

	undo_redo.set_action_callback(_undo_redo_action, this);
	_update_title();

	signal_delete_event().connect(sigc::mem_fun(*this, &Interface::_close_request));

	singleton = this;

	playback_cursor_follow = false;

	//setup song
	_update_song_mixing_parameters();
	SoundDriverManager::set_mix_callback(_process_audio);

	playback_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Interface::_playback_timer_callback),
			10, Glib::PRIORITY_DEFAULT);
}

Interface::~Interface() {

	for (int i = 0; i < menu_items.size(); i++) {
		delete menu_items[i];
	}
	for (int i = 0; i < racks.size(); i++) {
		delete racks[i].rack;
		delete racks[i].volume;
	}
	if (rack_filler) {
		delete rack_filler;
	}
}
