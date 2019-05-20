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
	enum EditMode {
		EDIT_ROWS_DISCRETE,
		EDIT_ENVELOPE_SMALL,
		EDIT_ENVELOPE_LARGE
	};

	enum {
		VALUE_MAX = 99,
		EMPTY = 255
	};

private:
	AudioEffect *owner;
	ControlPort *port;
	EditMode display_mode;

	Map<int, ValueStream<Tick, uint8_t> > data;

	bool has_pre_play_capture;
	float pre_play_capture_value;

	static void _ui_changed_callbacks(void *p_ud);
	void _ui_changed_callback();

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

	void set_edit_mode(EditMode p_mode);
	EditMode get_edit_mode() const;

	void pre_play_capture();
	void pre_play_restore();

	bool is_empty() const;

	void add_notify();
	void remove_notify();

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

	struct Command {

		enum {
			EMPTY = 0xFF,
			MAX_PARAM = 99,
		};

		uint8_t command;
		uint8_t parameter;
		inline bool is_empty() const { return (command == EMPTY && parameter == 0); }
		bool operator==(Command p_command) const { return command == p_command.command && parameter == p_command.parameter; }

		Command(uint8_t p_command = EMPTY, uint8_t p_parameter = 0) {
			command = p_command;
			parameter = p_parameter;
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
			TYPE_COMMAND,
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

		operator Command() const {

			if (type != TYPE_COMMAND)
				return Command();
			else {

				Command c;
				c.command = a;
				c.parameter = b;
				return c;
			}
		}

		Event(const Note &p_note) {
			type = TYPE_NOTE;
			a = p_note.note;
			b = p_note.volume;
		}
		Event(const Command &p_command) {
			type = TYPE_COMMAND;
			a = p_command.command;
			b = p_command.parameter;
		}

		Event(const uint8_t p_autoval) {

			type = TYPE_AUTOMATION;
			a = p_autoval;
			b = 0;
		}

		static Event make_empty(Type p_type) {
			Event ev;
			ev.type = p_type;
			ev.a = Note::EMPTY;
			ev.b = ev.type == TYPE_COMMAND ? 0 : Note::EMPTY;
			return ev;
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

	struct PosCommand {
		Pos pos;
		Command command;
	};

	struct PosEvent {
		Pos pos;
		Event event;
	};

	enum {
		SEND_SPEAKERS = -1,
		EVENT_BUFFER_MAX = 8192
	};

private:
	Map<int, ValueStream<Pos, Note> > note_data;
	int note_columns;
	Vector<uint8_t> column_state;

	Map<int, ValueStream<Pos, Command> > command_data;
	int command_columns;

	bool muted;

	Vector<AudioEffect *> effects;
	Vector<Automation *> automations;
	Vector<Automation *> disabled_automations;

	struct Send {
		int track;
		float amount;
		bool mute;
	};

	Vector<Send> sends;

	float mix_volume;

	String name;

	int sampling_rate;

	friend class Song;
	Vector<AudioFrame> input_buffer;
	Vector<AudioFrame> process_buffer;
	Vector<AudioFrame> process_buffer2;

	AudioEffect::Event event_buffer[EVENT_BUFFER_MAX];
	int event_buffer_size;

	void process_events(int p_pattern, Tick p_offset, Tick p_from_tick, Tick p_to_tick, int p_bpm, int p_swing_divisor, float p_swing, int p_from = -1, int p_to = -1);
	void add_single_event(const AudioEffect::Event &p_event);
	const AudioFrame *process_audio_step();

	bool first_mix;

	mutable float peak_volume_l;
	mutable float peak_volume_r;

	_FORCE_INLINE_ Tick _get_swinged_tick(Tick p_tick, int p_swing_divisor, float p_swing);

public:
	void set_name(String p_name);
	String get_name() const;

	/* audio effects */

	int get_audio_effect_count() const;
	void add_audio_effect(AudioEffect *p_effect, int p_pos = -1);
	void remove_audio_effect(int p_pos);
	AudioEffect *get_audio_effect(int p_pos);
	void swap_audio_effects(int p_effect, int p_with_effect);

	/* automations */

	int get_automation_count() const;
	void add_automation(Automation *p_automation, int p_pos = -1);
	void remove_automation(int p_pos);
	Automation *get_automation(int p_pos) const;
	void swap_automations(int p_which, int p_by_which);

	/* disabled automations (user may want to keep it, even if it was disabled)*/

	int get_disabled_automation_count() const;
	void add_disabled_automation(Automation *p_automation, int p_pos = -1);
	void remove_disabled_automation(int p_pos);
	Automation *get_disabled_automation(int p_pos) const;

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

	void set_command_columns(int p_columns);
	int get_command_column_count() const;

	void set_command(int p_pattern, Pos p_pos, Command p_command);
	Command get_command(int p_pattern, Pos p_pos) const;

	int get_command_count(int p_pattern) const;
	Command get_command_by_index(int p_pattern, int p_index) const;
	Pos get_command_pos_by_index(int p_pattern, int p_index) const;

	void get_commands_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, int &r_first, int &r_count) const;
	void get_commands_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosCommand> *r_commands) const;

	void set_muted(bool p_mute);
	bool is_muted() const;

	int get_event_column_count() const;
	Track::Event::Type get_event_column_type(int p_column) const;
	void set_event(int p_pattern, int p_column, Tick p_pos, const Event &p_event);
	Event get_event(int p_pattern, int p_column, Tick p_pos) const;
	void get_events_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosEvent> *r_events) const;

	void set_mix_volume_db(float p_db);
	float get_mix_volume_db() const;

	float get_peak_volume_db_l() const;
	float get_peak_volume_db_r() const;

	void add_send(int p_track, int p_pos = -1);
	void set_send_amount(int p_send, float p_amount);
	void set_send_track(int p_send, int p_track);
	void set_send_mute(int p_send, bool p_mute);
	int get_send_track(int p_send) const;
	float get_send_amount(int p_send) const;
	bool is_send_muted(int p_send) const;
	int get_send_count();
	void remove_send(int p_send);
	void swap_sends(int p_send, int p_with_send);
	bool has_send(int p_send) const;

	void set_process_buffer_size(int p_frames);
	void set_sampling_rate(int p_hz);

	void stop();

	void automations_pre_play_capture();
	void automations_pre_play_restore();

	Track();
	~Track();
};

#endif // TRACK_H
