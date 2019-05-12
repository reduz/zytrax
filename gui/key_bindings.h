#ifndef KEY_BINDINGS_H
#define KEY_BINDINGS_H

#include "rstring.h"
#include <gtkmm.h>
class KeyBindings {
public:
	enum KeyBind {

		FILE_NEW,
		FILE_OPEN,
		FILE_SAVE,
		FILE_SAVE_AS,
		FILE_QUIT,

		PLAYBACK_PLAY,
		PLAYBACK_STOP,
		PLAYBACK_NEXT_PATTERN,
		PLAYBACK_PREV_PATTERN,
		PLAYBACK_PLAY_PATTERN,
		PLAYBACK_PLAY_FROM_CURSOR,
		PLAYBACK_PLAY_FROM_ORDER,
		PLAYBACK_CURSOR_FOLLOW,

		EDIT_UNDO,
		EDIT_REDO,
		EDIT_SONG_INFO,
		EDIT_FOCUS_PATTERN,
		EDIT_FOCUS_ORDERLIST,
		EDIT_FOCUS_LAST_EDITED_EFFECT,

		TRACK_ADD_TRACK,
		TRACK_ADD_COLUMN,
		TRACK_REMOVE_COLUMN,
		TRACK_MOVE_LEFT,
		TRACK_MOVE_RIGHT,
		TRACK_MUTE,
		TRACK_SOLO,
		TRACK_RENAME,
		TRACK_REMOVE,

		AUTOMATION_TOGGLE_VISIBLE,
		AUTOMATION_RADIO_ENVELOPE_NUMBERS,
		AUTOMATION_RADIO_ENVELOPE_SMALL,
		AUTOMATION_RADIO_ENVELOPE_LARGE,
		AUTOMATION_MOVE_LEFT,
		AUTOMATION_MOVE_RIGHT,
		AUTOMATION_REMOVE,

		SETTINGS_OPEN,
		SETTINGS_PATTERN_INPUT_KEYS,
		SETTINGS_ABOUT,

		CURSOR_MOVE_UP,
		CURSOR_MOVE_DOWN,
		CURSOR_MOVE_UP_1_ROW,
		CURSOR_MOVE_DOWN_1_ROW,
		CURSOR_PAGE_UP,
		CURSOR_PAGE_DOWN,
		CURSOR_MOVE_LEFT,
		CURSOR_MOVE_RIGHT,
		CURSOR_TAB,
		CURSOR_BACKTAB,
		CURSOR_HOME,
		CURSOR_END,
		CURSOR_FIELD_CLEAR,
		CURSOR_INSERT,
		CURSOR_DELETE,
		CURSOR_COPY_VOLUME_MASK,
		CURSOR_TOGGLE_VOLUME_MASK,
		CURSOR_PLAY_NOTE,
		CURSOR_PLAY_ROW,

		CURSOR_ADVANCE_1,
		CURSOR_ADVANCE_2,
		CURSOR_ADVANCE_3,
		CURSOR_ADVANCE_4,
		CURSOR_ADVANCE_5,
		CURSOR_ADVANCE_6,
		CURSOR_ADVANCE_7,
		CURSOR_ADVANCE_8,
		CURSOR_ADVANCE_9,
		CURSOR_ADVANCE_10,

		CURSOR_ZOOM_1,
		CURSOR_ZOOM_2,
		CURSOR_ZOOM_3,
		CURSOR_ZOOM_4,
		CURSOR_ZOOM_6,
		CURSOR_ZOOM_8,
		CURSOR_ZOOM_12,
		CURSOR_ZOOM_16,
		CURSOR_ZOOM_24,
		CURSOR_ZOOM_32,

		PATTERN_PAN_WINDOW_UP,
		PATTERN_PAN_WINDOW_DOWN,
		PATTERN_CURSOR_NOTE_OFF,
		PATTERN_OCTAVE_RAISE,
		PATTERN_OCTAVE_LOWER,
		PATTERN_PREV_PATTERN,
		PATTERN_NEXT_PATTERN,

		PATTERN_SELECT_BEGIN,
		PATTERN_SELECT_END,
		PATTERN_SELECT_COLUMN_TRACK_ALL,
		PATTERN_SELECTION_RAISE_NOTES_SEMITONE,
		PATTERN_SELECTION_RAISE_NOTES_OCTAVE,
		PATTERN_SELECTION_LOWER_NOTES_SEMITONE,
		PATTERN_SELECTION_LOWER_NOTES_OCTAVE,
		PATTERN_SELECTION_SET_VOLUME,
		PATTERN_SELECTION_INTERPOLATE_VOLUME_AUTOMATION,
		PATTERN_SELECTION_AMPLIFY_VOLUME_AUTOMATION,
		PATTERN_SELECTION_CUT,
		PATTERN_SELECTION_COPY,
		PATTERN_SELECTION_PASTE_INSERT,
		PATTERN_SELECTION_PASTE_OVERWRITE,
		PATTERN_SELECTION_PASTE_MIX,
		PATTERN_SELECTION_DISABLE,
		PATTERN_SELECTION_DOUBLE_LENGTH,
		PATTERN_SELECTION_HALVE_LENGTH,
		PATTERN_SELECTION_SCALE_LENGTH,

		PIANO_C0,
		PIANO_Cs0,
		PIANO_D0,
		PIANO_Ds0,
		PIANO_E0,
		PIANO_F0,
		PIANO_Fs0,
		PIANO_G0,
		PIANO_Gs0,
		PIANO_A0,
		PIANO_As0,
		PIANO_B0,
		PIANO_C1,
		PIANO_Cs1,
		PIANO_D1,
		PIANO_Ds1,
		PIANO_E1,
		PIANO_F1,
		PIANO_Fs1,
		PIANO_G1,
		PIANO_Gs1,
		PIANO_A1,
		PIANO_As1,
		PIANO_B1,
		PIANO_C2,
		PIANO_Cs2,
		PIANO_D2,
		PIANO_Ds2,
		PIANO_E2,

		BIND_MAX
	};

private:
	Gtk::Application *application;
	Gtk::ApplicationWindow *window;

	struct KeyState {

		enum Mode {
			MODE_NORMAL,
			MODE_TOGGLE,
			MODE_RADIO,
		};

		guint keyval;
		guint state;
		bool shortcut;
		Mode mode;
		int radio_base;
		String detailed_name; //cache

		guint initial_keyval;
		guint initial_state;

		KeyState(guint p_keyval = 0, guint p_state = 0, bool p_shortcut = false, Mode p_mode = MODE_NORMAL, int p_radio_base = 0) {
			keyval = p_keyval;
			state = p_state;
			shortcut = p_shortcut;
			mode = p_mode;
			radio_base = p_radio_base;
			initial_keyval = p_keyval;
			initial_state = p_state;
		}
	};

	static const char *bind_names[BIND_MAX];
	KeyState binds[BIND_MAX];
	Glib::RefPtr<Gio::SimpleAction> actions[BIND_MAX];

	void _add_keybind(KeyBind p_bind, KeyState p_state);
	void _on_action(KeyBind p_bind);
	void _on_action_string(Glib::ustring p_string);

	bool initialized;

public:
	//done this way so each UI can capture whathever action it wants
	sigc::signal1<void, KeyBind> action_activated;

	String get_keybind_detailed_name(KeyBind p_bind);
	String get_keybind_action_name(KeyBind p_bind);
	String get_keybind_text(KeyBind p_bind);
	const char *get_keybind_name(KeyBind p_bind);
	int get_keybind_key(KeyBind p_bind);
	int get_keybind_mod(KeyBind p_bind);

	Glib::RefPtr<Gio::SimpleAction> get_keybind_action(KeyBind p_bind);
	void set_action_enabled(KeyBind p_bind, bool p_enabled);
	void set_action_checked(KeyBind p_bind, bool p_checked);
	void set_action_state(KeyBind p_bind, const String &p_state);

	bool is_keybind(GdkEventKey *ev, KeyBind p_bind) const;
	bool is_keybind_noshift(GdkEventKey *ev, KeyBind p_bind) const;

	void set_keybind(KeyBind p_bind, guint p_keyval, guint p_state);
	void reset_keybind(KeyBind p_bind);
	void clear_keybind(KeyBind p_bind);

	void initialize(Gtk::Application *p_application, Gtk::ApplicationWindow *p_window);
	KeyBindings();
};

#endif // KEY_BINDINGS_H
