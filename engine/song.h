#ifndef SONG_H
#define SONG_H

#include "track.h"

class Song {
public:
	enum {
		DEFAULT_BEATS_PER_BAR = 4,
		DEFAULT_PATTERN_BEATS = 16,
		DEFAULT_BPM = 125,
		ORDER_MAX = 999,
		ORDER_EMPTY = 0xFFFFF,
		ORDER_SKIP = 0xFFFFE,
		MAX_PATTERN = 999
	};

private:
	struct PatternConfig {

		int beats_per_bar;
		int beats;
		PatternConfig() {
			beats_per_bar = DEFAULT_BEATS_PER_BAR;
			beats = DEFAULT_PATTERN_BEATS;
		}
	};

	Map<int, PatternConfig> pattern_config;
	Map<int, int> order_list;

	Vector<Track *> tracks;

	void _check_delete_pattern_config(int p_pattern);

	float bpm;
	float swing;

	String name;
	String author;
	String description;

public:
	void pattern_set_beats_per_bar(int p_pattern, int p_beats_per_bar);
	int pattern_get_beats_per_bar(int p_pattern) const;
	void pattern_set_beats(int p_pattern, int p_beats);
	int pattern_get_beats(int p_pattern) const;

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
	int get_event_column_note_column(int p_column) const;
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

	~Song();
	Song();
};

#endif // SONG_H
