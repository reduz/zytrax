#include "song.h"

void Song::_check_delete_pattern_config(int p_pattern) {

	if (!pattern_config.has(p_pattern))
		return;
	if (pattern_config[p_pattern].beats_per_bar == DEFAULT_BEATS_PER_BAR && pattern_config[p_pattern].beats == DEFAULT_PATTERN_BEATS) {

		pattern_config.erase(p_pattern);
	}
}

void Song::pattern_set_beats_per_bar(int p_pattern, int p_beats_per_bar) {

	_AUDIO_LOCK_

	if (!pattern_config.has(p_pattern))
		pattern_config[p_pattern] = PatternConfig();

	pattern_config[p_pattern].beats_per_bar = p_beats_per_bar;

	_check_delete_pattern_config(p_pattern);
}

int Song::pattern_get_beats_per_bar(int p_pattern) const {

	if (!pattern_config.has(p_pattern))
		return DEFAULT_BEATS_PER_BAR;

	return pattern_config[p_pattern].beats_per_bar;
}
void Song::pattern_set_beats(int p_pattern, int p_beats) {

	_AUDIO_LOCK_

	if (!pattern_config.has(p_pattern))
		pattern_config[p_pattern] = PatternConfig();

	pattern_config[p_pattern].beats = p_beats;

	_check_delete_pattern_config(p_pattern);
}
int Song::pattern_get_beats(int p_pattern) const {

	if (!pattern_config.has(p_pattern))
		return DEFAULT_PATTERN_BEATS;

	return pattern_config[p_pattern].beats;
}

void Song::order_set(int p_order, int p_pattern) {

	_AUDIO_LOCK_

	ERR_FAIL_COND(p_order < 0 || (p_order > ORDER_MAX && p_order != ORDER_EMPTY && p_order != ORDER_SKIP));

	if (p_order == ORDER_EMPTY)
		order_list.erase(p_order);
	else
		order_list.insert(p_order, p_pattern);
}
int Song::order_get(int p_order) const {

	if (order_list.has(p_order))
		return order_list[p_order];
	else
		return ORDER_EMPTY;
}

int Song::get_track_count() const {

	return tracks.size();
}

void Song::add_track(Track *p_track) {

	_AUDIO_LOCK_
	//audio kill
	tracks.push_back(p_track);
}

void Song::add_track_at_pos(Track *p_track, int p_pos) {

	_AUDIO_LOCK_
	tracks.insert(p_pos, p_track);
}

void Song::remove_track(int p_idx) {

	_AUDIO_LOCK_
	//audio kill...
	ERR_FAIL_INDEX(p_idx, tracks.size());

	tracks.remove(p_idx);
}

void Song::swap_tracks(int p_which, int p_by_which) {
	_AUDIO_LOCK_;
	SWAP(tracks[p_which], tracks[p_by_which]);
}

Track *Song::get_track(int p_idx) {

	ERR_FAIL_INDEX_V(p_idx, tracks.size(), NULL);

	return tracks[p_idx];
}

int Song::get_event_column_count() const {

	int cc = 0;
	for (int i = 0; i < tracks.size(); i++)
		cc += tracks[i]->get_event_column_count();

	return cc;
}

int Song::get_event_column_track(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			return i;
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	return -1;
}
int Song::get_event_column_note_column(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			if (p_column < tracks[i]->get_column_count()) {
				return p_column;
			} else {
				return -1;
			}
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	return -1;
}
int Song::get_event_column_automation(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			if (p_column < tracks[i]->get_column_count()) {
				return -1;
			} else {
				return p_column - tracks[i]->get_column_count();
			}
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	return -1;
}

void Song::set_event(int p_pattern, int p_column, Tick p_pos, Track::Event p_event) {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			tracks[i]->set_event(p_pattern, p_column, p_pos, p_event);
			return;
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	ERR_FAIL_COND(true);
}

Track::Event::Type Song::get_event_column_type(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			return tracks[i]->get_event_column_type(p_column);
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	ERR_FAIL_COND_V(true, Track::Event::TYPE_NOTE);
}

Track::Event Song::get_event(int p_pattern, int p_column, Tick p_pos) const {

	for (int i = 0; i < tracks.size(); i++) {
		printf("col: %i\n", p_column);
		if (p_column < tracks[i]->get_event_column_count()) {

			return tracks[i]->get_event(p_pattern, p_column, p_pos);
		}

		p_column -= tracks[i]->get_event_column_count();
	}

	ERR_FAIL_COND_V(true, Track::Event());
}
void Song::get_events_in_range(int p_pattern, const Track::Pos &p_from, const Track::Pos &p_to, List<Track::PosEvent> *r_events) const {

	int col = 0;
	for (int i = 0; i < tracks.size(); i++) {

		int tc = tracks[i]->get_event_column_count();

		for (int j = 0; j < tc; j++) {

			if (col >= p_from.column) {

				Track::Pos from = p_from;
				from.column = j;
				Track::Pos to = p_to;
				to.column = j;

				List<Track::PosEvent> pevents;
				tracks[i]->get_events_in_range(p_pattern, from, to, &pevents);

				for (List<Track::PosEvent>::Element *E = pevents.front(); E; E = E->next()) {
					Track::PosEvent pe = E->get();
					pe.pos.column = col;
					r_events->push_back(pe);
				}
			}

			col++;
			if (col > p_to.column)
				break;
		}
	}
}

void Song::set_bpm(float p_value) {
	bpm = p_value;
}
float Song::get_bpm() const {
	return bpm;
}

void Song::set_swing(float p_value) {
	swing = p_value;
}
float Song::get_swing() const {
	return swing;
}

Song::~Song() {

	for (int i = 0; i < tracks.size(); i++)
		delete tracks[i];
}

Song::Song() {
	bpm = 125;
	swing = 0;
}
