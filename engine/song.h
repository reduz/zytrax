#ifndef SONG_H
#define SONG_H

#include "track.h"

class Song {
public:

	enum {
		DEFAULT_BEATS_PER_BAR=4,
		DEFAULT_PATTERN_BEATS=16,
		ORDER_MAX=(1<<16),
		ORDER_EMPTY=0xFFFFF,
		ORDER_SKIP=0xFFFFE,
	};

private:


	struct PatternConfig {

		int beats_per_bar;
		int beats;
		PatternConfig() { beats_per_bar=DEFAULT_BEATS_PER_BAR; beats=DEFAULT_PATTERN_BEATS; }
	};

	Map<int,PatternConfig> pattern_config;
	Map<int,int> order_list;

	Vector<Track*> tracks;

	void _check_delete_pattern_config(int p_pattern);

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
	void add_track_at_pos(Track *p_track,int p_pos);
	void swap_tracks(int p_which,int p_by_which);

	int get_event_column_count() const;
	void set_event(int p_pattern, int p_column, Tick p_pos, Track::Event p_event);
	Track::Event::Type get_event_column_type(int p_column) const;
	Track::Event get_event(int p_pattern,int p_column, Tick p_pos) const;
	void get_events_in_range(int p_pattern,const Track::Pos& p_from,const Track::Pos& p_to,List<Track::PosEvent> *r_events ) const;

	~Song();
	Song();
};

#endif // SONG_H
