#ifndef COLOR_THEME_H
#define COLOR_THEME_H

#include "globals/rstring.h"
#include <gtkmm.h>
struct Theme {

	enum {
		COLOR_BACKGROUND,
		COLOR_FOCUS,
		COLOR_PATTERN_EDITOR_TRACK_SEPARATOR,
		COLOR_PATTERN_EDITOR_CURSOR,
		COLOR_PATTERN_EDITOR_ROW_BAR,
		COLOR_PATTERN_EDITOR_ROW_BEAT,
		COLOR_PATTERN_EDITOR_ROW_SUB_BEAT,
		COLOR_PATTERN_EDITOR_BG,
		COLOR_PATTERN_EDITOR_NOTE,
		COLOR_PATTERN_EDITOR_HL_BAR,
		COLOR_PATTERN_EDITOR_HL_BEAT,
		COLOR_PATTERN_EDITOR_NOTE_NOFIT,
		COLOR_PATTERN_EDITOR_AUTOMATION_VALUE,
		COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR,
		COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT,
		COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_NOFIT,
		COLOR_PATTERN_EDITOR_AUTOMATION_POINT,
		COLOR_PATTERN_EDITOR_TRACK_NAME,
		COLOR_PATTERN_EDITOR_AUTOMATION_NAME,
		COLOR_MAX
	};

	Gdk::RGBA colors[COLOR_MAX];

	enum {
		FONT_PATTERN,
		FONT_MAX
	};

	struct Font {
		String face;
		float size;
		bool bold;
	};

	Font fonts[FONT_MAX];

	enum {

		CONSTANT_PATTERN_EDITOR_TRACK_SEPARATION,
		CONSTANT_PATTERN_EDITOR_COLUMN_SEPARATION,
		CONSTANT_MAX
	};

	int constants[CONSTANT_MAX];

	Theme();
};

#endif // COLOR_THEME_H
