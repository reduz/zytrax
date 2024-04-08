#include "key_bindings.h"
#include "error_macros.h"

bool KeyBindings::is_keybind(GdkEventKey *ev, KeyBind p_bind) const {

	guint state = ev->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK | GDK_META_MASK);

	return (binds[p_bind].state == state && binds[p_bind].keyval == ev->keyval);
}

bool KeyBindings::is_keybind_noshift(GdkEventKey *ev, KeyBind p_bind) const {

	guint state = ev->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_META_MASK);

	return (binds[p_bind].state == state && binds[p_bind].keyval == ev->keyval);
}

const char *KeyBindings::bind_names[BIND_MAX] = {

	"FileNew",
	"FileOpen",
	"ImportIT",
	"FileSave",
	"FileSaveAs",
	"FileExportWav",
	"FileQuit",

	"PlaybackPlay",
	"PlaybackStop",
	"PlaybackNextPattern",
	"PlaybackPrevPattern",
	"PlaybackPlayPattern",
	"PlaybackPlayFromCursor",
	"PlaybackPlayFromOrder",
	"PlaybackCursorFollow",

	"EditUndo",
	"EditRedo",
	"EditSongSettings",
	"EditFocusPattern",
	"EditFocusOrderlist",
	"EditFocusLastEffect",

	"TrackAddTrack",
	"TrackAddColumn",
	"TrackRemoveColumn",
	"TrackAddCommandColumn",
	"TrackRemoveCommandColumn",
	"TrackMergeNext",
	"TrackMoveLeft",
	"TrackMoveRight",
	"TrackMute",
	"TrackSolo",
	"TrackRename",
	"TrackRemove",

	"AutomationRadioDiscreteRows",
	"AutomationRadioEnvelopeSmall",
	"AutomationRadioEnvelopeLarge",
	"AutomationMoveLeft",
	"AutomationMoveRight",
	"AutomationRemove",

	"SettingsOpen",
	"SettingsPatternInputKeys",
	"SettingsAbout",

	"CursorMoveUp",
	"CursorMoveDown",
	"CursorMoveUp1Row",
	"CursorMoveDown1Row",
	"CursorPageUp",
	"CursorPageDown",
	"CursorMoveLeft",
	"CursorMoveRight",
	"CursorTab",
	"CursorBacktab",
	"CursorHome",
	"CursorEnd",
	"CursorFieldClear",
	"CursorInsert",
	"CursorDelete",
	"CursorTrackInsert",
	"CursorTrackDelete",
	"CursorCopyVolumeMask",
	"CursorToggleVolumeMask",
	"CursorPlayNote",
	"CursorPlayRow",

	"CursorAdvance1",
	"CursorAdvance2",
	"CursorAdvance3",
	"CursorAdvance4",
	"CursorAdvance5",
	"CursorAdvance6",
	"CursorAdvance7",
	"CursorAdvance8",
	"CursorAdvance9",
	"CursorAdvance10",

	"CursorZoom1",
	"CursorZoom2",
	"CursorZoom3",
	"CursorZoom4",
	"CursorZoom6",
	"CursorZoom8",
	"CursorZoom12",
	"CursorZoom16",
	"CursorZoom24",
	"CursorZoom32",

	"PatternPanWindowUp",
	"PatternPanWindowDown",
	"PatternCursorNoteOff",
	"PatternOctaveLower",
	"PatternOctaveRaise",
	"PatternPrevPattern",
	"PatternNextPattern",

	"PatternOctaveLowerAlt",
	"PatternOctaveRaiseAlt",
	"PatternPrevPatternAlt",
	"PatternNextPatternAlt",

	"PatternSelectBegin",
	"PatternSelectEnd",
	"PatternSelectColumnTrackAll",
	"PatternSelectionRaiseNotesSemitone",
	"PatternSelectionRaiseNotesOctave",
	"PatternSelectionLowerNotesSemitone",
	"PatternSelectionLowerNotesOctave",
	"PatternSelectionSetVolume",
	"PatternSelectionInterpolateVolumeAutomation",
	"PatternSelectionAmplifyVolumeAutomation",
	"PatternSelectionCut",
	"PatternSelectionCopy",
	"PatternSelectionPasteInsert",
	"PatternSelectionPasteOverwrite",
	"PatternSelectionPasteMix",
	"PatternSelectionDisable",
	"PatternSelectionDoubleLength",
	"PatternSelectionHalveLength",
	"PatternSelectionScaleLength",

	"PianoC0",
	"PianoCs0",
	"PianoD0",
	"PianoDs0",
	"PianoE0",
	"PianoF0",
	"PianoFs0",
	"PianoG0",
	"PianoGs0",
	"PianoA0",
	"PianoAs0",
	"PianoB0",
	"PianoC1",
	"PianoCs1",
	"PianoD1",
	"PianoDs1",
	"PianoE1",
	"PianoF1",
	"PianoFs1",
	"PianoG1",
	"PianoGs1",
	"PianoA1",
	"PianoAs1",
	"PianoB1",
	"PianoC2",
	"PianoCs2",
	"PianoD2",
	"PianoDs2",
	"PianoE2",
};

String KeyBindings::get_keybind_detailed_name(KeyBind p_bind) {
	return binds[p_bind].detailed_name;
}

String KeyBindings::get_keybind_text(KeyBind p_bind) {
	Gtk::AccelKey accel(binds[p_bind].keyval, Gdk::ModifierType(binds[p_bind].state));
	return accel.get_abbrev().c_str();
}
String KeyBindings::get_keybind_action_name(KeyBind p_bind) {
	if (binds[p_bind].mode == KeyState::MODE_RADIO) {
		return bind_names[binds[p_bind].radio_base];
	} else {
		return bind_names[p_bind];
	}
}

const char *KeyBindings::get_keybind_name(KeyBind p_bind) {
	return bind_names[p_bind];
}

int KeyBindings::get_keybind_key(KeyBind p_bind) {

	return binds[p_bind].keyval;
}
int KeyBindings::get_keybind_mod(KeyBind p_bind) {
	return binds[p_bind].state;
}

void KeyBindings::_add_keybind(KeyBind p_bind, KeyState p_state) {

	binds[p_bind] = p_state;
}

void KeyBindings::initialize(Gtk::Application *p_application, Gtk::ApplicationWindow *p_window) {

	initialized = true;
	application = p_application;
	window = p_window;

	for (int i = 0; i < BIND_MAX; i++) {
		if (!binds[i].shortcut) {
			continue;
		}

		KeyState &state = binds[i];
		//create action

		if (state.mode == KeyState::MODE_TOGGLE) {
			actions[i] = window->add_action_bool(bind_names[i], sigc::bind<KeyBind>(sigc::mem_fun(*this, &KeyBindings::_on_action), KeyBind(i)));
		} else if (state.mode == KeyState::MODE_RADIO) {
			if (i == state.radio_base) {
				//only add if it's the base one

				actions[i] = window->add_action_radio_string(bind_names[i], sigc::mem_fun(*this, &KeyBindings::_on_action_string), bind_names[i]);
			} else {
				actions[i] = actions[state.radio_base];
			}
		} else {
			actions[i] = window->add_action(bind_names[i], sigc::bind<KeyBind>(sigc::mem_fun(*this, &KeyBindings::_on_action), KeyBind(i)));
		}

		//create accelerator

		if (state.mode == KeyState::MODE_RADIO) {

			gchar *text = gtk_accelerator_name(state.keyval, GdkModifierType(state.state));

			GVariant *v = g_variant_new_string(bind_names[i]);
			gchar *detailed_name = g_action_print_detailed_name(bind_names[state.radio_base], v);
			g_variant_unref(v);

			String dname = String("win.") + detailed_name;

			free(detailed_name);

			if (state.keyval > 0) {
				application->set_accel_for_action(dname.ascii().get_data(), text);
			}
			binds[i].detailed_name = dname;

			free(text);

		} else {

			String dname = "win." + String(bind_names[i]);
			gchar *text = gtk_accelerator_name(state.keyval, GdkModifierType(state.state));

			if (state.keyval > 0) {
				application->set_accel_for_action(dname.ascii().get_data(), text);
			}
			binds[i].detailed_name = dname;
			free(text);
		}
	}
}

void KeyBindings::set_action_enabled(KeyBind p_bind, bool p_enabled) {
	if (actions[p_bind].operator->()) {
		actions[p_bind]->set_enabled(p_enabled);
	}
}

void KeyBindings::set_action_checked(KeyBind p_bind, bool p_checked) {

	actions[p_bind]->change_state(Glib::Variant<bool>::create(p_checked));
}
void KeyBindings::set_action_state(KeyBind p_bind, const String &p_state) {

	int idx;
	if (binds[p_bind].mode == KeyState::MODE_RADIO) {
		idx = binds[p_bind].radio_base;
	} else {
		idx = p_bind;
	}

	actions[idx]->change_state(Glib::Variant<Glib::ustring>::create(p_state.ascii().get_data()));
}

Glib::RefPtr<Gio::SimpleAction> KeyBindings::get_keybind_action(KeyBind p_bind) {
	return actions[p_bind];
}

void KeyBindings::set_keybind(KeyBind p_bind, guint p_keyval, guint p_state) {
	ERR_FAIL_INDEX(p_bind, BIND_MAX);

	binds[p_bind].keyval = p_keyval;
	binds[p_bind].state = p_state;

	if (!initialized || !binds[p_bind].shortcut) {
		return;
	}

	//unset existing

	application->unset_accels_for_action(binds[p_bind].detailed_name.ascii().get_data());

	if (p_keyval == 0) {
		return;
	}
	//set new

	gchar *text = gtk_accelerator_name(p_keyval, GdkModifierType(p_state));
	application->set_accel_for_action(binds[p_bind].detailed_name.ascii().get_data(), text);
	free(text);
}

void KeyBindings::reset_keybind(KeyBind p_bind) {
	ERR_FAIL_INDEX(p_bind, BIND_MAX);
	set_keybind(p_bind, binds[p_bind].initial_keyval, binds[p_bind].initial_state);
}
void KeyBindings::clear_keybind(KeyBind p_bind) {
	ERR_FAIL_INDEX(p_bind, BIND_MAX);
	set_keybind(p_bind, 0, 0);
}

void KeyBindings::_on_action(KeyBind p_bind) {

	action_activated.emit(p_bind);
}

void KeyBindings::_on_action_string(Glib::ustring p_string) {
	for (int i = 0; i < BIND_MAX; i++) {
		if (p_string == bind_names[i]) {
			_on_action(KeyBind(i));
		}
	}
}

KeyBindings::KeyBindings() {

	initialized = false;

	_add_keybind(FILE_NEW, KeyState(GDK_KEY_n, GDK_CONTROL_MASK, true));
	_add_keybind(FILE_OPEN, KeyState(GDK_KEY_o, GDK_CONTROL_MASK, true));
	_add_keybind(FILE_IMPORT_IT, KeyState(0, 0, true));
	_add_keybind(FILE_SAVE, KeyState(GDK_KEY_s, GDK_CONTROL_MASK, true));
	_add_keybind(FILE_SAVE_AS, KeyState(0, 0, true));
	_add_keybind(FILE_EXPORT_WAV, KeyState(0, 0, true));
	_add_keybind(FILE_QUIT, KeyState(GDK_KEY_q, GDK_CONTROL_MASK, true));

	_add_keybind(PLAYBACK_PLAY, KeyState(GDK_KEY_F5, 0, true));
	_add_keybind(PLAYBACK_STOP, KeyState(GDK_KEY_F8, 0, true));
	_add_keybind(PLAYBACK_NEXT_PATTERN, KeyState(GDK_KEY_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK, true));
	_add_keybind(PLAYBACK_PREV_PATTERN, KeyState(GDK_KEY_Up, GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK, true));
	_add_keybind(PLAYBACK_PLAY_PATTERN, KeyState(GDK_KEY_F6, 0, true));
	_add_keybind(PLAYBACK_PLAY_FROM_CURSOR, KeyState(GDK_KEY_F7, 0, true));
	_add_keybind(PLAYBACK_PLAY_FROM_ORDER, KeyState(GDK_KEY_F6, GDK_SHIFT_MASK, true));
	_add_keybind(PLAYBACK_CURSOR_FOLLOW, KeyState(GDK_KEY_F4, 0, true, KeyState::MODE_TOGGLE));

	_add_keybind(EDIT_UNDO, KeyState(GDK_KEY_z, GDK_CONTROL_MASK, true));
	_add_keybind(EDIT_REDO, KeyState(GDK_KEY_z, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(EDIT_SONG_INFO, KeyState(GDK_KEY_i, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(EDIT_FOCUS_PATTERN, KeyState(GDK_KEY_F2, 0, true));
	_add_keybind(EDIT_FOCUS_ORDERLIST, KeyState(GDK_KEY_F11, 0, true));
	_add_keybind(EDIT_FOCUS_LAST_EDITED_EFFECT, KeyState(GDK_KEY_F3, 0, true));

	_add_keybind(TRACK_ADD_TRACK, KeyState(GDK_KEY_a, GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_ADD_COLUMN, KeyState(GDK_KEY_bracketright, GDK_MOD1_MASK, true));
	_add_keybind(TRACK_REMOVE_COLUMN, KeyState(GDK_KEY_bracketleft, GDK_MOD1_MASK, true));
	_add_keybind(TRACK_ADD_COMMAND_COLUMN, KeyState(GDK_KEY_bracketright, GDK_MOD1_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_REMOVE_COMMAND_COLUMN, KeyState(GDK_KEY_bracketleft, GDK_MOD1_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_MERGE_NEXT, KeyState(0, 0, true));
	_add_keybind(TRACK_MOVE_LEFT, KeyState(GDK_KEY_Left, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_MOVE_RIGHT, KeyState(GDK_KEY_Right, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_MUTE, KeyState(GDK_KEY_F9, 0, true, KeyState::MODE_TOGGLE));
	_add_keybind(TRACK_SOLO, KeyState(GDK_KEY_F10, 0, true));
	_add_keybind(TRACK_RENAME, KeyState(GDK_KEY_r, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));
	_add_keybind(TRACK_REMOVE, KeyState(GDK_KEY_x, GDK_SHIFT_MASK | GDK_CONTROL_MASK, true));

	_add_keybind(AUTOMATION_RADIO_DISCRETE_ROWS, KeyState(GDK_KEY_n, GDK_CONTROL_MASK | GDK_SHIFT_MASK, true, KeyState::MODE_RADIO, AUTOMATION_RADIO_DISCRETE_ROWS));
	_add_keybind(AUTOMATION_RADIO_ENVELOPE_SMALL, KeyState(GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, true, KeyState::MODE_RADIO, AUTOMATION_RADIO_DISCRETE_ROWS));
	_add_keybind(AUTOMATION_RADIO_ENVELOPE_LARGE, KeyState(GDK_KEY_l, GDK_CONTROL_MASK | GDK_SHIFT_MASK, true, KeyState::MODE_RADIO, AUTOMATION_RADIO_DISCRETE_ROWS));
	_add_keybind(AUTOMATION_MOVE_LEFT, KeyState(GDK_KEY_Left, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(AUTOMATION_MOVE_RIGHT, KeyState(GDK_KEY_Right, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(AUTOMATION_REMOVE, KeyState(GDK_KEY_x, GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SHIFT_MASK, true));

	_add_keybind(SETTINGS_OPEN, KeyState(GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(SETTINGS_PATTERN_INPUT_KEYS, KeyState(GDK_KEY_p, GDK_CONTROL_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(SETTINGS_ABOUT, KeyState(0, 0, true));

	_add_keybind(CURSOR_MOVE_UP, KeyState(GDK_KEY_Up));
	_add_keybind(CURSOR_MOVE_DOWN, KeyState(GDK_KEY_Down));
	_add_keybind(CURSOR_MOVE_UP_1_ROW, KeyState(GDK_KEY_Up, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_MOVE_DOWN_1_ROW, KeyState(GDK_KEY_Down, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_PAGE_UP, KeyState(GDK_KEY_Page_Up));
	_add_keybind(CURSOR_PAGE_DOWN, KeyState(GDK_KEY_Page_Down));
	_add_keybind(CURSOR_MOVE_LEFT, KeyState(GDK_KEY_Left));
	_add_keybind(CURSOR_MOVE_RIGHT, KeyState(GDK_KEY_Right));
	_add_keybind(CURSOR_TAB, KeyState(GDK_KEY_Tab));
	_add_keybind(CURSOR_BACKTAB, KeyState(GDK_KEY_ISO_Left_Tab, GDK_SHIFT_MASK));
	_add_keybind(CURSOR_HOME, KeyState(GDK_KEY_Home));
	_add_keybind(CURSOR_END, KeyState(GDK_KEY_End));
	_add_keybind(CURSOR_FIELD_CLEAR, KeyState(GDK_KEY_period));
	_add_keybind(CURSOR_INSERT, KeyState(GDK_KEY_Insert));
	_add_keybind(CURSOR_DELETE, KeyState(GDK_KEY_Delete));
	_add_keybind(CURSOR_TRACK_INSERT, KeyState(GDK_KEY_Insert, GDK_SHIFT_MASK));
	_add_keybind(CURSOR_TRACK_DELETE, KeyState(GDK_KEY_Delete, GDK_SHIFT_MASK));
	_add_keybind(CURSOR_COPY_VOLUME_MASK, KeyState(GDK_KEY_Return));
	_add_keybind(CURSOR_TOGGLE_VOLUME_MASK, KeyState(GDK_KEY_comma));
	_add_keybind(CURSOR_PLAY_NOTE, KeyState(GDK_KEY_4));
	_add_keybind(CURSOR_PLAY_ROW, KeyState(GDK_KEY_8));

	_add_keybind(CURSOR_ADVANCE_1, KeyState(GDK_KEY_1, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_2, KeyState(GDK_KEY_2, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_3, KeyState(GDK_KEY_3, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_4, KeyState(GDK_KEY_4, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_5, KeyState(GDK_KEY_5, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_6, KeyState(GDK_KEY_6, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_7, KeyState(GDK_KEY_7, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_8, KeyState(GDK_KEY_8, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_9, KeyState(GDK_KEY_9, GDK_MOD1_MASK));
	_add_keybind(CURSOR_ADVANCE_10, KeyState(GDK_KEY_0, GDK_MOD1_MASK));

	_add_keybind(CURSOR_ZOOM_1, KeyState(GDK_KEY_1, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_2, KeyState(GDK_KEY_2, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_3, KeyState(GDK_KEY_3, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_4, KeyState(GDK_KEY_4, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_6, KeyState(GDK_KEY_5, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_8, KeyState(GDK_KEY_6, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_12, KeyState(GDK_KEY_7, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_16, KeyState(GDK_KEY_8, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_24, KeyState(GDK_KEY_9, GDK_CONTROL_MASK));
	_add_keybind(CURSOR_ZOOM_32, KeyState(GDK_KEY_0, GDK_CONTROL_MASK));

	_add_keybind(PATTERN_PAN_WINDOW_UP, KeyState(GDK_KEY_Up, GDK_MOD1_MASK));
	_add_keybind(PATTERN_PAN_WINDOW_DOWN, KeyState(GDK_KEY_Left, GDK_MOD1_MASK));
	_add_keybind(PATTERN_CURSOR_NOTE_OFF, KeyState(GDK_KEY_grave));

	_add_keybind(PATTERN_OCTAVE_LOWER, KeyState(GDK_KEY_minus));
	_add_keybind(PATTERN_OCTAVE_RAISE, KeyState(GDK_KEY_equal));
	_add_keybind(PATTERN_PREV_PATTERN, KeyState(GDK_KEY_bracketleft));
	_add_keybind(PATTERN_NEXT_PATTERN, KeyState(GDK_KEY_bracketright));

	_add_keybind(PATTERN_OCTAVE_LOWER_ALT, KeyState(GDK_KEY_KP_Divide));
	_add_keybind(PATTERN_OCTAVE_RAISE_ALT, KeyState(GDK_KEY_KP_Multiply));
	_add_keybind(PATTERN_PREV_PATTERN_ALT, KeyState(GDK_KEY_KP_Subtract));
	_add_keybind(PATTERN_NEXT_PATTERN_ALT, KeyState(GDK_KEY_KP_Add));

	_add_keybind(PATTERN_SELECT_BEGIN, KeyState(GDK_KEY_b, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECT_END, KeyState(GDK_KEY_e, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECT_COLUMN_TRACK_ALL, KeyState(GDK_KEY_l, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_RAISE_NOTES_SEMITONE, KeyState(GDK_KEY_q, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_RAISE_NOTES_OCTAVE, KeyState(GDK_KEY_q, GDK_MOD1_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(PATTERN_SELECTION_LOWER_NOTES_SEMITONE, KeyState(GDK_KEY_a, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_LOWER_NOTES_OCTAVE, KeyState(GDK_KEY_a, GDK_MOD1_MASK | GDK_SHIFT_MASK, true));
	_add_keybind(PATTERN_SELECTION_SET_VOLUME, KeyState(GDK_KEY_v, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION, KeyState(GDK_KEY_k, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION, KeyState(GDK_KEY_j, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_CUT, KeyState(GDK_KEY_z, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_COPY, KeyState(GDK_KEY_c, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_PASTE_INSERT, KeyState(GDK_KEY_p, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_PASTE_OVERWRITE, KeyState(GDK_KEY_o, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_PASTE_MIX, KeyState(GDK_KEY_m, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_DISABLE, KeyState(GDK_KEY_u, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_DOUBLE_LENGTH, KeyState(GDK_KEY_f, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_HALVE_LENGTH, KeyState(GDK_KEY_g, GDK_MOD1_MASK, true));
	_add_keybind(PATTERN_SELECTION_SCALE_LENGTH, KeyState(GDK_KEY_t, GDK_MOD1_MASK, true));

	_add_keybind(PIANO_C0, KeyState(GDK_KEY_z));
	_add_keybind(PIANO_Cs0, KeyState(GDK_KEY_s));
	_add_keybind(PIANO_D0, KeyState(GDK_KEY_x));
	_add_keybind(PIANO_Ds0, KeyState(GDK_KEY_d));
	_add_keybind(PIANO_E0, KeyState(GDK_KEY_c));
	_add_keybind(PIANO_F0, KeyState(GDK_KEY_v));
	_add_keybind(PIANO_Fs0, KeyState(GDK_KEY_g));
	_add_keybind(PIANO_G0, KeyState(GDK_KEY_b));
	_add_keybind(PIANO_Gs0, KeyState(GDK_KEY_h));
	_add_keybind(PIANO_A0, KeyState(GDK_KEY_n));
	_add_keybind(PIANO_As0, KeyState(GDK_KEY_j));
	_add_keybind(PIANO_B0, KeyState(GDK_KEY_m));
	_add_keybind(PIANO_C1, KeyState(GDK_KEY_q));
	_add_keybind(PIANO_Cs1, KeyState(GDK_KEY_2));
	_add_keybind(PIANO_D1, KeyState(GDK_KEY_w));
	_add_keybind(PIANO_Ds1, KeyState(GDK_KEY_3));
	_add_keybind(PIANO_E1, KeyState(GDK_KEY_e));
	_add_keybind(PIANO_F1, KeyState(GDK_KEY_r));
	_add_keybind(PIANO_Fs1, KeyState(GDK_KEY_5));
	_add_keybind(PIANO_G1, KeyState(GDK_KEY_t));
	_add_keybind(PIANO_Gs1, KeyState(GDK_KEY_6));
	_add_keybind(PIANO_A1, KeyState(GDK_KEY_y));
	_add_keybind(PIANO_As1, KeyState(GDK_KEY_7));
	_add_keybind(PIANO_B1, KeyState(GDK_KEY_u));
	_add_keybind(PIANO_C2, KeyState(GDK_KEY_i));
	_add_keybind(PIANO_Cs2, KeyState(GDK_KEY_9));
	_add_keybind(PIANO_D2, KeyState(GDK_KEY_o));
	_add_keybind(PIANO_Ds2, KeyState(GDK_KEY_0));
	_add_keybind(PIANO_E2, KeyState(GDK_KEY_p));
}
