#ifndef TRACK_H
#define TRACK_H

#include "audio_effect.h"
#include "audio_lock.h"
#include "list.h"
#include "map.h"
#include "value_stream.h"
#include "vector.h"

#define TICKS_PER_BEAT 192
typedef uint64_t Tick;

class Automation {
public:
	enum DisplayMode {
		DISPLAY_ROWS,
		DISPLAY_SMALL,
		DISPLAY_LARGE
	};

	enum {
		VALUE_MAX = 99,
		EMPTY = 255
	};

private:
	AudioEffect *owner;
	ControlPort *port;
	DisplayMode display_mode;
	bool visible;

	Map<int, ValueStream<Tick, uint8_t> > data;

public:
	void set_point(int p_pattern, Tick p_offset, uint8_t p_value);
	bool has_point(int p_pattern, Tick p_offset) const;
	uint8_t get_point(int p_pattern, Tick p_offset) const;
	void remove_point(int p_pattern, Tick p_offset);

	Tick get_point_tick_by_index(int p_pattern, int p_index) const;
	uint8_t get_point_by_index(int p_pattern, int p_index) const;
	int get_point_count(int p_pattern) const;
	void get_points_in_range(int p_pattern, Tick p_from, Tick p_to, int &r_first, int &r_count) const;
	float interpolate_offset(int p_pattern, Tick p_offset) const;

	ControlPort *get_control_port();
	AudioEffect *get_owner();

	void set_visible(bool p_visible);
	bool is_visible() const;
	void set_display_mode(DisplayMode p_mode);
	DisplayMode get_display_mode() const;

	Automation(ControlPort *p_port, AudioEffect *p_owner = NULL);
};

class Track {
public:
	struct Note {

		enum {
			EMPTY = 0xFF,
			OFF = 0xFE,
			MAX_VOLUME = 99,
			MAX_NOTE = 119 // 10 octaves
		};

		uint8_t note;
		uint8_t volume;
		inline bool is_empty() const { return (note == EMPTY && volume == EMPTY); }
		bool operator==(Note p_note) const { return note == p_note.note && volume == p_note.volume; }

		Note(uint8_t p_note = EMPTY, uint8_t p_volume = EMPTY) {
			note = p_note;
			volume = p_volume;
		}
	};

	struct Pos {

		Tick tick;
		int column;

		bool operator<(const Pos &p_rval) const { return (tick == p_rval.tick) ? (column < p_rval.column) : (tick < p_rval.tick); }
		bool operator>(const Pos &p_rval) const { return (tick == p_rval.tick) ? (column > p_rval.column) : (tick > p_rval.tick); }
		bool operator==(const Pos &p_rval) const { return (tick == p_rval.tick) && (column == p_rval.column); }

		Pos(Tick p_tick = 0, int p_column = 0) {
			tick = p_tick;
			column = p_column;
		}
	};

	//generic event?
	struct Event {

		enum Type {
			TYPE_NOTE,
			TYPE_AUTOMATION
		};

		Type type;
		uint8_t a;
		uint8_t b;

		operator uint8_t() const {
			// to automation
			if (type != TYPE_AUTOMATION)
				return Automation::EMPTY;
			else
				return a;
		}

		operator Note() const {

			if (type != TYPE_NOTE)
				return Note();
			else {

				Note n;
				n.note = a;
				n.volume = b;
				return n;
			}
		}

		Event(const Note &p_note) {
			type = TYPE_NOTE;
			a = p_note.note;
			b = p_note.volume;
		}

		Event(const uint8_t p_autoval) {

			type = TYPE_AUTOMATION;
			a = p_autoval;
			b = 0;
		}

		Event() {
			type = TYPE_NOTE;
			a = Note::EMPTY;
			b = Note::EMPTY;
		}
	};

	struct PosNote {
		Pos pos;
		Note note;
	};

	struct PosEvent {
		Pos pos;
		Event event;
	};

private:
	Map<int, ValueStream<Pos, Note> > note_data;
	int note_columns;

	int swing_step;
	bool muted;

	Vector<AudioEffect *> effects;
	Vector<Automation *> automations;

	ControlPortDefault swing;
	ControlPortDefault volume;
	ControlPortDefault pan;

	float mix_volume;

	String name;

public:
	void set_name(String p_name);
	String get_name() const;

	/* audio effects */

	int get_audio_effect_count() const;
	void add_audio_effect(AudioEffect *p_effect, int p_pos = -1);
	void remove_audio_effect(int p_pos);
	AudioEffect *get_audio_effect(int p_pos);

	/* automations */

	int get_automation_count() const;
	void add_automation(Automation *p_automation, int p_pos = -1);
	void remove_automation(int p_pos);
	Automation *get_automation(int p_pos) const;
	void swap_automations(int p_which, int p_by_which);

	/* notes */

	void set_columns(int p_columns);
	int get_column_count() const;

	void set_note(int p_pattern, Pos p_pos, Note p_note);
	Note get_note(int p_pattern, Pos p_pos) const;

	int get_note_count(int p_pattern) const;
	Note get_note_by_index(int p_pattern, int p_index) const;
	Pos get_note_pos_by_index(int p_pattern, int p_index) const;

	void get_notes_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, int &r_first, int &r_count) const;
	void get_notes_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosNote> *r_notes) const;

	/* swingie */

	void set_swing_step(int p_swing_step);
	int get_swing_step() const;

	void set_muted(bool p_mute);
	bool is_muted() const;

	int get_event_column_count() const;
	Track::Event::Type get_event_column_type(int p_column) const;
	void set_event(int p_pattern, int p_column, Tick p_pos, const Event &p_event);
	Event get_event(int p_pattern, int p_column, Tick p_pos) const;
	void get_events_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosEvent> *r_events) const;

	ControlPort *get_volume_port();
	ControlPort *get_pan_port();
	ControlPort *get_swing_port();

	void set_mix_volume_db(float p_db);
	float get_mix_volume_db() const;

	float get_peak_volume_db() const;

	Track();
};

#endif // TRACK_H
