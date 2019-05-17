#ifndef SONG_H
#define SONG_H

#include "track.h"

class Song {
public:
	enum {
		DEFAULT_BEATS_PER_BAR = 4,
		DEFAULT_PATTERN_BEATS = 16,
		DEFAULT_BPM = 125,
		DEFAULT_PROCESS_BUFFER_SIZE = 256,
		ORDER_MAX = 999,
		ORDER_EMPTY = 0xFFFFF,
		ORDER_SKIP = 0xFFFFE,
		MAX_PATTERN = 999
	};

	enum SwingBeatDivisor {
		SWING_BEAT_DIVISOR_1,
		SWING_BEAT_DIVISOR_2,
		SWING_BEAT_DIVISOR_3,
		SWING_BEAT_DIVISOR_4,
		SWING_BEAT_DIVISOR_6,
		SWING_BEAT_DIVISOR_8,
		SWING_BEAT_DIVISOR_MAX
	};

private:
	struct PatternConfig {

		int beats_per_bar;
		int beats;
		SwingBeatDivisor swing_beat_divisor;
		PatternConfig() {
			beats_per_bar = DEFAULT_BEATS_PER_BAR;
			beats = DEFAULT_PATTERN_BEATS;
			swing_beat_divisor = SWING_BEAT_DIVISOR_1;
		}
	};

	Map<int, PatternConfig> pattern_config;
	Map<int, int> order_list;

	Vector<Track *> tracks;

	void _check_delete_pattern_config(int p_pattern);

	float bpm;
	float swing;
	float main_volume_db;
	mutable float peak_volume_l;
	mutable float peak_volume_r;

	String name;
	String author;
	String description;

	Vector<AudioFrame> buffer;
	int buffer_pos;

	void _process_audio_step();

	Vector<int> track_process_order;
	struct SortSend {
		int from;
		int to;
	};

	int sampling_rate;

	struct Playback {
		bool playing;
		bool loop_pattern;
		int pattern;
		int order; //number if playing song, else -1
		double pos; //tick (needs sub-tick accuracy)
		int bpm;
		float volume;
		float prev_volume;
		bool can_loop;

		struct Range {
			bool active;
			int pattern;
			int from_column;
			int to_column;
			Tick from_tick;
			Tick to_tick;
		} range;

	} playback;

	void _pre_capture_automations();
	void _restore_automations();

public:
	bool update_process_order();

	void set_process_buffer_size(int p_frames);
	void set_sampling_rate(int p_hz);

	void process_audio(AudioFrame *p_output, int p_frames);

	void pattern_set_beats_per_bar(int p_pattern, int p_beats_per_bar);
	int pattern_get_beats_per_bar(int p_pattern) const;
	void pattern_set_beats(int p_pattern, int p_beats);
	int pattern_get_beats(int p_pattern) const;
	void pattern_set_swing_beat_divisor(int p_pattern, SwingBeatDivisor p_divisor);
	SwingBeatDivisor pattern_get_swing_beat_divisor(int p_pattern) const;

	void order_set(int p_order, int p_pattern);
	int order_get(int p_order) const;

	int get_track_count() const;

	void add_track(Track *p_track);
	void remove_track(int p_idx);
	Track *get_track(int p_idx);
	void add_track_at_pos(Track *p_track, int p_pos);
	void swap_tracks(int p_which, int p_by_which);

	int get_event_column_count() const;
	void set_event(int p_pattern, int p_column, Tick p_pos, Track::Event p_event);
	Track::Event::Type get_event_column_type(int p_column) const;

	int get_event_column_track(int p_column) const;
	int get_event_column_event(int p_column) const;
	int get_event_column_note_column(int p_column) const;
	int get_event_column_command_column(int p_column) const;
	int get_event_column_automation(int p_column) const;

	Track::Event get_event(int p_pattern, int p_column, Tick p_pos) const;
	void get_events_in_range(int p_pattern, const Track::Pos &p_from, const Track::Pos &p_to, List<Track::PosEvent> *r_events) const;

	void set_bpm(float p_value);
	float get_bpm() const;

	void set_swing(float p_value);
	float get_swing() const;

	void set_name(String p_name);
	String get_name() const;

	void set_author(String p_author);
	String get_author() const;

	void set_description(String p_description);
	String get_description() const;

	void clear();

	bool can_play() const;
	bool play(int p_from_order = 0, Tick p_from_tick = 0, bool p_can_loop = true);
	void play_pattern(int p_pattern, Tick p_from_tick = 0);
	void play_event_range(int p_pattern, int p_from_column, int p_to_column, Tick p_from_tick, Tick p_to_tick);
	void play_next_pattern();
	void play_prev_pattern();

	void stop();
	bool is_playing() const;
	int get_playing_order() const;
	int get_playing_pattern() const;
	Tick get_playing_tick() const;

	void set_main_volume_db(float p_db);
	float get_main_volume_db() const;

	float get_peak_volume_db_l() const;
	float get_peak_volume_db_r() const;

	~Song();
	Song();
};

#endif // SONG_H
