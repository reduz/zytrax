#include "key_bindings.h"

bool KeyBindings::is_keybind(GdkEventKey *ev, KeyBind p_bind) const {

	guint state = ev->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK | GDK_META_MASK);

	return (binds[p_bind].state == state && binds[p_bind].keyval == ev->keyval);
}

KeyBindings::KeyBindings() {

	binds[CURSOR_MOVE_UP] = KeyState(GDK_KEY_Up);
	binds[CURSOR_MOVE_DOWN] = KeyState(GDK_KEY_Down);
	binds[CURSOR_MOVE_UP_1_ROW] = KeyState(GDK_KEY_Up, GDK_CONTROL_MASK);
	binds[CURSOR_MOVE_DOWN_1_ROW] = KeyState(GDK_KEY_Down, GDK_CONTROL_MASK);
	binds[CURSOR_PAGE_UP] = KeyState(GDK_KEY_Page_Up);
	binds[CURSOR_PAGE_DOWN] = KeyState(GDK_KEY_Page_Down);
	binds[CURSOR_MOVE_LEFT] = KeyState(GDK_KEY_Left);
	binds[CURSOR_MOVE_RIGHT] = KeyState(GDK_KEY_Right);
	binds[CURSOR_TAB] = KeyState(GDK_KEY_Tab);
	binds[CURSOR_BACKTAB] = KeyState(GDK_KEY_Tab, GDK_SHIFT_MASK);
	binds[CURSOR_HOME] = KeyState(GDK_KEY_Home);
	binds[CURSOR_END] = KeyState(GDK_KEY_End);
	binds[CURSOR_FIELD_CLEAR] = KeyState(GDK_KEY_period);
	binds[CURSOR_INSERT] = KeyState(GDK_KEY_Insert);
	binds[CURSOR_DELETE] = KeyState(GDK_KEY_Delete);
	binds[PATTERN_PAN_WINDOW_UP] = KeyState(GDK_KEY_Up, GDK_MOD1_MASK);
	binds[PATTERN_PAN_WINDOW_DOWN] = KeyState(GDK_KEY_Left, GDK_MOD1_MASK);
	binds[PATTERN_CURSOR_NOTE_OFF] = KeyState(GDK_KEY_equal);

	binds[PIANO_C0] = KeyState(GDK_KEY_z);
	binds[PIANO_Cs0] = KeyState(GDK_KEY_s);
	binds[PIANO_D0] = KeyState(GDK_KEY_x);
	binds[PIANO_Ds0] = KeyState(GDK_KEY_d);
	binds[PIANO_E0] = KeyState(GDK_KEY_c);
	binds[PIANO_F0] = KeyState(GDK_KEY_v);
	binds[PIANO_Fs0] = KeyState(GDK_KEY_g);
	binds[PIANO_G0] = KeyState(GDK_KEY_b);
	binds[PIANO_Gs0] = KeyState(GDK_KEY_h);
	binds[PIANO_A0] = KeyState(GDK_KEY_n);
	binds[PIANO_As0] = KeyState(GDK_KEY_j);
	binds[PIANO_B0] = KeyState(GDK_KEY_m);
	binds[PIANO_C1] = KeyState(GDK_KEY_q);
	binds[PIANO_Cs1] = KeyState(GDK_KEY_2);
	binds[PIANO_D1] = KeyState(GDK_KEY_w);
	binds[PIANO_Ds1] = KeyState(GDK_KEY_3);
	binds[PIANO_E1] = KeyState(GDK_KEY_e);
	binds[PIANO_F1] = KeyState(GDK_KEY_r);
	binds[PIANO_Fs1] = KeyState(GDK_KEY_5);
	binds[PIANO_G1] = KeyState(GDK_KEY_t);
	binds[PIANO_Gs1] = KeyState(GDK_KEY_6);
	binds[PIANO_A1] = KeyState(GDK_KEY_y);
	binds[PIANO_As1] = KeyState(GDK_KEY_7);
	binds[PIANO_B1] = KeyState(GDK_KEY_u);
	binds[PIANO_C2] = KeyState(GDK_KEY_i);
	binds[PIANO_Cs2] = KeyState(GDK_KEY_9);
	binds[PIANO_D2] = KeyState(GDK_KEY_o);
	binds[PIANO_Ds2] = KeyState(GDK_KEY_0);
	binds[PIANO_E2] = KeyState(GDK_KEY_p);
}
