#include "color_theme.h"

static Gdk::RGBA make_rgba(uint8_t p_red, uint8_t p_green, uint8_t p_blue, uint8_t p_alpha = 255) {

	Gdk::RGBA rgba;
	rgba.set_red(float(p_red) / 255.0);
	rgba.set_green(float(p_green) / 255.0);
	rgba.set_blue(float(p_blue) / 255.0);
	rgba.set_alpha(float(p_alpha) / 255.0);
	return rgba;
}

Theme::Theme() {

	colors[COLOR_BACKGROUND] = make_rgba(0, 0, 0);
	colors[COLOR_FOCUS] = make_rgba(255, 255, 255);
	colors[COLOR_PATTERN_EDITOR_TRACK_SEPARATOR] = make_rgba(128, 128, 128);
	colors[COLOR_PATTERN_EDITOR_CURSOR] = make_rgba(255, 255, 255);
	colors[COLOR_PATTERN_EDITOR_ROW_BAR] = make_rgba(220, 220, 255);
	colors[COLOR_PATTERN_EDITOR_ROW_BEAT] = make_rgba(110, 110, 140);
	colors[COLOR_PATTERN_EDITOR_ROW_SUB_BEAT] = make_rgba(60, 60, 80);
	colors[COLOR_PATTERN_EDITOR_BG] = make_rgba(26, 26, 42);
	colors[COLOR_PATTERN_EDITOR_NOTE] = make_rgba(181, 181, 234);
	colors[COLOR_PATTERN_EDITOR_HL_BAR] = make_rgba(61, 61, 95);
	colors[COLOR_PATTERN_EDITOR_HL_BEAT] = make_rgba(41, 41, 61);
	colors[COLOR_PATTERN_EDITOR_NOTE_NOFIT] = make_rgba(144, 144, 169);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_VALUE] = make_rgba(217, 217, 180);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_HL_BAR] = make_rgba(87, 87, 65);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_HL_BEAT] = make_rgba(63, 63, 49);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_VALUE_NOFIT] = make_rgba(205, 205, 190);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_POINT] = make_rgba(251, 246, 220);
	colors[COLOR_PATTERN_EDITOR_TRACK_NAME] = make_rgba(181, 181, 234);
	colors[COLOR_PATTERN_EDITOR_AUTOMATION_NAME] = make_rgba(217, 217, 180);

	fonts[FONT_PATTERN].face = "FreeMono";
	fonts[FONT_PATTERN].size = 15;
	fonts[FONT_PATTERN].bold = true;

	constants[CONSTANT_PATTERN_EDITOR_TRACK_SEPARATION] = 5;
	constants[CONSTANT_PATTERN_EDITOR_COLUMN_SEPARATION] = 10;
}
