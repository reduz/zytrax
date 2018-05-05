#ifndef KEY_BINDINGS_H
#define KEY_BINDINGS_H

#include <gdkmm.h>

class KeyBindings {
public:
	struct KeyState {
		guint keyval;
		guint state;

		KeyState(guint p_keyval = 0, guint p_state = 0) {
			keyval = p_keyval;
			state = p_state;
		}
	};

	enum KeyBind {
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
		PATTERN_PAN_WINDOW_UP,
		PATTERN_PAN_WINDOW_DOWN,
		PATTERN_CURSOR_NOTE_OFF,

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

	KeyState binds[BIND_MAX];

	bool is_keybind(GdkEventKey *ev, KeyBind p_bind) const;
	KeyBindings();
};

#endif // KEY_BINDINGS_H
