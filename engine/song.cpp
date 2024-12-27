#include "song.h"
#include "dsp/db.h"



void Song::_dispatch_routed_event(const MIDIEventRouted & p_event, void *p_self) {

	Song *self = (Song*)p_self;
	if (self->midi_buffer_pos >= MAX_MIDI_EVENTS_PER_MIX) {
		return;
	}

	MIDIEventRouted ev = p_event;
	self->midi_event_buffer[self->midi_buffer_pos++]=ev;;
}

void Song::_dispatch_main_thread_event(const MIDIEventRouted & p_event, void *p_self) {
	Song *self = (Song*)p_self;
	if (self->main_thread_midi_buffer_pos >= MAX_MIDI_EVENTS_PER_MIX) {
		return;
	}

	MIDIEventRouted ev = p_event;
	self->main_thread_midi_events[self->main_thread_midi_buffer_pos++]=ev;
}


void Song::_process_audio_step() {

	int buffer_len = buffer.size();
	AudioFrame *buffer_ptr = &buffer[0];
	midi_buffer_pos = 0;

	//clear
	for (int i = 0; i < buffer_len; i++) {
		buffer_ptr[i] = AudioFrame(0, 0);
	}

	//clear track buffers
	for (int j = 0; j < tracks.size(); j++) {
		Track *t = tracks[j];
		ERR_FAIL_COND(t->input_buffer.size() != buffer_len);
		ERR_FAIL_COND(t->process_buffer.size() != buffer_len);
		AudioFrame *input_buf_ptr = &t->input_buffer[0];
		AudioFrame *process_buf_ptr = &t->process_buffer[0];

		//clear
		for (int i = 0; i < buffer_len; i++) {
			input_buf_ptr[i] = AudioFrame(0, 0);
			process_buf_ptr[i] = AudioFrame(0, 0);
		}
	}

	//process events and playback
	if (playback.playing) {
		double tick_to_frame = ((60.0 / double(playback.bpm)) / double(TICKS_PER_BEAT)) * double(sampling_rate);
		double block_ticks = buffer_len / tick_to_frame;
		double pattern_ticks = pattern_get_beats(playback.pattern) * TICKS_PER_BEAT;
		double block_offset = 0.0;
		SwingBeatDivisor swing_divisor = pattern_get_swing_beat_divisor(playback.pattern);

		if (playback.pos + block_ticks >= pattern_ticks) {
			//went past end of pattern
			Tick tick_from = Tick(playback.pos);
			Tick tick_to = Tick(pattern_ticks);
			//play to end of pattern
			for (int i = 0; i < tracks.size(); i++) {
				tracks[i]->process_events(playback.pattern, 0, tick_from, tick_to, playback.bpm, swing_divisor, swing);
			}
			//remainder
			block_ticks = playback.pos + block_ticks - pattern_ticks;
			block_offset = pattern_ticks - playback.pos;
			playback.pos = 0; //restart on next pattern

			//guess next pattern
			if (!playback.loop_pattern) {
				int attempts = Song::ORDER_MAX + 2; //add two, first and loop.
				playback.order++;
				//validate order
				while (attempts) {
					attempts--;
					if (playback.order > ORDER_MAX) {
						if (!playback.can_loop) {
							playback.playing = false;
							break;
						}
						playback.order = 0;
					}

					int next_pattern = order_get(playback.order);
					if (next_pattern == ORDER_EMPTY) {
						if (!playback.can_loop) {
							playback.playing = false;
							break;
						}
						playback.order = 0;
					} else if (next_pattern == ORDER_SKIP) {
						playback.order++;
					} else {
						playback.pattern = next_pattern;
						break;
					}
				}

				if (attempts == 0) {
					playback.playing = false;
				}
			}
		}

		if (playback.playing && playback.playing && block_ticks > 0) {
			//went past end of pattern
			Tick tick_from = Tick(playback.pos);
			Tick tick_to = Tick(playback.pos + block_ticks);
			Tick offset = Tick(block_offset);
			playback.pos += block_ticks;
			//play remaining region
			for (int i = 0; i < tracks.size(); i++) {
				tracks[i]->process_events(playback.pattern, offset, tick_from, tick_to, playback.bpm, swing_divisor, swing);
			}
		}
	}

	if (playback.range.active) {

		int from_track = get_event_column_track(playback.range.from_column);
		int to_track = get_event_column_track(playback.range.to_column);

		for (int i = from_track; i <= to_track; i++) {
			ERR_CONTINUE(i < 0 || i >= tracks.size());
			int from = (i == from_track) ? get_event_column_event(playback.range.from_column) : 0;
			int to = (i == to_track) ? get_event_column_event(playback.range.to_column) : tracks[i]->get_event_column_count() - 1;

			tracks[i]->process_events(playback.range.pattern, 0, playback.range.from_tick, playback.range.to_tick, playback.playing ? playback.bpm : bpm, playback.range.pattern, 0, from, to);
		}
		playback.range.active = false;
	}

	for (int i = 0; i < playback.single_event_count; i++) {
		ERR_CONTINUE(playback.single_event[i].track < 0 || playback.single_event[i].track >= tracks.size());
		tracks[playback.single_event[i].track]->add_single_event(playback.single_event[i].event);
	}
	playback.single_event_count = 0;

	//process audio in track-order
	ERR_FAIL_COND(track_process_order.size() != tracks.size());

	for (int i = 0; i < track_process_order.size(); i++) {

		Track *t = tracks[track_process_order[i]];
		//process tracks
		const AudioFrame *src_audio = t->process_audio_step(_dispatch_routed_event,this);
		//do sends
		int send_count = t->sends.size();
		Track::Send *sends = send_count ? &t->sends[0] : NULL;
		for (int j = 0; j < send_count; j++) {
			if (sends[j].mute) {
				continue;
			}
			AudioFrame *dst_audio = sends[j].track == Track::SEND_SPEAKERS ? buffer_ptr : &tracks[sends[j].track]->process_buffer[0];
			//accumulate
			for (int k = 0; k < buffer_len; k++) {
				dst_audio[k] += src_audio[k] * sends[j].amount;
			}
		}
	}

	//apply volume
	{
		float volume = db2linear(main_volume_db);
		for (int i = 0; i < buffer_len; i++) {
			buffer_ptr[i] *= volume;
			float energy_l = ABS(buffer_ptr[i].l);
			if (energy_l > peak_volume_l) {
				peak_volume_l = energy_l;
			}
			float energy_r = ABS(buffer_ptr[i].r);
			if (energy_r > peak_volume_r) {
				peak_volume_r = energy_r;
			}
		}
	}
}

void Song::_flush_midi_events(int p_events_from_pos,int p_events_to_pos,int &event_write_ofs,MIDIEventRouted *p_event_buffer, int p_event_buffer_max_size) {

	for(uint32_t i=0;i<midi_buffer_pos;i++) {
		if (event_write_ofs >= p_event_buffer_max_size) {
			break;
		}

		MIDIEventRouted ev = midi_event_buffer[i];
		ev.frame += p_events_from_pos;

		if (ev.frame >= 0 && ev.frame < p_events_to_pos) { //only contemplate what covers this mix.
			p_event_buffer[event_write_ofs++]=ev;
		}
	}
}

void Song::process_audio(AudioFrame *p_output, int p_frames, MIDIEventRouted *p_event_buffer, int p_event_buffer_max_size, int &r_events_written) {

	int buffer_len = buffer.size();
	ERR_FAIL_COND(buffer_len == 0);
	const AudioFrame *buffer_ptr = &buffer[0];

	main_thread_midi_buffer_pos = MIN(main_thread_midi_buffer_pos,p_event_buffer_max_size);

	r_events_written = 0;

	for(int i=0;i<main_thread_midi_buffer_pos;i++) {
		p_event_buffer[i]=main_thread_midi_events[i];
		r_events_written++;
	}

	p_event_buffer+=main_thread_midi_buffer_pos;
	p_event_buffer_max_size-=main_thread_midi_buffer_pos;
	main_thread_midi_buffer_pos=0;


	int event_write_ofs = 0;
	int event_read_ofs = -buffer_pos;

	for (int i = 0; i < p_frames; i++) {

		if (buffer_pos >= buffer_len) {
			_flush_midi_events(event_read_ofs,event_read_ofs + buffer_len,event_write_ofs,p_event_buffer, p_event_buffer_max_size);
			event_read_ofs += buffer_len;
			_process_audio_step();
			buffer_pos = 0;
		}

		p_output[i] = buffer_ptr[buffer_pos];
		buffer_pos++;
	}

	_flush_midi_events(event_read_ofs,event_read_ofs + buffer_pos,event_write_ofs,p_event_buffer, p_event_buffer_max_size);

	/*for(int i=0;i<event_write_ofs;i++) {
		p_event_buffer[i]=midi_event_buffer[i];
	}*/

	r_events_written += event_write_ofs;
}

///////////////

void Song::_check_delete_pattern_config(int p_pattern) {

	if (!pattern_config.has(p_pattern))
		return;
	if (pattern_config[p_pattern].beats_per_bar == DEFAULT_BEATS_PER_BAR && pattern_config[p_pattern].beats == DEFAULT_PATTERN_BEATS && pattern_config[p_pattern].swing_beat_divisor == SWING_BEAT_DIVISOR_1) {

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

void Song::pattern_set_swing_beat_divisor(int p_pattern, SwingBeatDivisor p_divisor) {

	ERR_FAIL_INDEX(p_divisor, SWING_BEAT_DIVISOR_MAX);

	_AUDIO_LOCK_

			if (!pattern_config.has(p_pattern))
			pattern_config[p_pattern] = PatternConfig();

	pattern_config[p_pattern].swing_beat_divisor = p_divisor;

	_check_delete_pattern_config(p_pattern);
}
Song::SwingBeatDivisor Song::pattern_get_swing_beat_divisor(int p_pattern) const {
	if (!pattern_config.has(p_pattern))
		return SWING_BEAT_DIVISOR_1;

	return pattern_config[p_pattern].swing_beat_divisor;
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

bool Song::update_process_order() {

	Vector<int> references;
	references.resize(tracks.size());
	for (int i = 0; i < tracks.size(); i++) {
		references[i] = i;
	}

	// add connections

	Vector<SortSend> sort_sends;
	for (int i = 0; i < tracks.size(); i++) {
		for (int j = 0; j < tracks[i]->get_send_count(); j++) {
			int send_to = tracks[i]->get_send_track(j);
			if (send_to != Track::SEND_SPEAKERS) {
				SortSend ss;
				ss.from = i;
				ss.to = send_to;
				sort_sends.push_back(ss);
			}
		}
	}

	//sort

	int max_passes = sort_sends.size() * sort_sends.size();
	int pass_count = 0;
	bool success = true;
	while (pass_count < max_passes) {
		//using bubblesort because of simplicity,
		success = true;
		for (int i = 0; i < sort_sends.size(); i++) {
			int from_idx = sort_sends[i].from;
			int to_idx = sort_sends[i].to;
			if (references[from_idx] > references[to_idx]) {
				SWAP(references[from_idx], references[to_idx]);
				success = false;
				break;
			}
		}

		if (success) {
			break;
		}

		pass_count++;
	}

	track_process_order.resize(tracks.size());
	for (int i = 0; i < references.size(); i++) {
		track_process_order[references[i]] = i;
	}

	return success;
}
void Song::add_track(Track *p_track) {

	_AUDIO_LOCK_
			//audio kill
			p_track->set_process_buffer_size(buffer.size());
	p_track->set_sampling_rate(sampling_rate);
	tracks.push_back(p_track);
	update_process_order();
}

void Song::add_track_at_pos(Track *p_track, int p_pos) {

	_AUDIO_LOCK_
			tracks.insert(p_pos, p_track);
	update_process_order();
}

void Song::remove_track(int p_idx) {

	_AUDIO_LOCK_
			//audio kill...
			ERR_FAIL_INDEX(p_idx, tracks.size());

	tracks.remove(p_idx);
	update_process_order();
}

void Song::swap_tracks(int p_which, int p_by_which) {
	_AUDIO_LOCK_;
	SWAP(tracks[p_which], tracks[p_by_which]);
	for (int i = 0; i < tracks.size(); i++) {
		Track *t = tracks[i];
		for (int j = 0; j < t->get_send_count(); j++) {
			int send = t->get_send_track(j);
			if (send == p_which) {
				t->set_send_track(j, p_by_which);
			} else if (send == p_by_which) {
				t->set_send_track(j, p_which);
			}
		}
	}
	update_process_order();
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

int Song::get_event_column_event(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			return p_column;
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

int Song::get_event_column_command_column(int p_column) const {

	for (int i = 0; i < tracks.size(); i++) {

		if (p_column < tracks[i]->get_event_column_count()) {

			if (p_column < tracks[i]->get_column_count()) {
				return -1;
			}

			p_column -= tracks[i]->get_column_count();

			if (p_column < tracks[i]->get_command_column_count()) {
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

			if (p_column < tracks[i]->get_column_count() + tracks[i]->get_command_column_count()) {
				return -1;
			} else {

				p_column -= tracks[i]->get_column_count() + tracks[i]->get_command_column_count();
				return p_column;
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
		if (col > p_to.column)
			break;
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

void Song::set_name(String p_name) {
	name = p_name;
}
String Song::get_name() const {

	return name;
}

void Song::set_author(String p_author) {
	author = p_author;
}
String Song::get_author() const {
	return author;
}

void Song::set_description(String p_description) {
	description = p_description;
}
String Song::get_description() const {
	return description;
}

void Song::set_process_buffer_size(int p_frames) {
	_AUDIO_LOCK_;
	buffer.resize(p_frames);
	for (int i = 0; i < p_frames; i++) {
		buffer[i] = AudioFrame(0, 0);
	}
	buffer_pos = p_frames;
	for (int i = 0; i < tracks.size(); i++) {
		tracks[i]->set_process_buffer_size(p_frames);
	}
}

void Song::set_sampling_rate(int p_hz) {
	_AUDIO_LOCK_
			sampling_rate = p_hz;
	for (int i = 0; i < tracks.size(); i++) {
		tracks[i]->set_sampling_rate(p_hz);
	}
}

void Song::_pre_capture_automations() {

	for (int i = 0; i < tracks.size(); i++) {
		tracks[i]->automations_pre_play_capture();
	}
}
void Song::_restore_automations() {

	for (int i = 0; i < tracks.size(); i++) {
		tracks[i]->automations_pre_play_restore();
	}
}

bool Song::can_play() const {

	int from_order = 0;
	while (true) {
		int order = order_get(from_order);

		if (order == ORDER_EMPTY) {
			return false; //nothing to play
		}

		if (from_order > ORDER_MAX) {
			return false;
		}

		if (order != ORDER_SKIP) {
			break;
		}

		from_order++;
	}

	return true;
}
bool Song::play(int p_from_order, Tick p_from_tick, bool p_can_loop) {
	stop();

	_AUDIO_LOCK_

			int order;
	while (true) {
		order = order_get(p_from_order);

		if (order == ORDER_EMPTY) {
			return false; //nothing to play
		}

		if (p_from_order > ORDER_MAX) {
			return false;
		}

		if (order != ORDER_SKIP) {
			break;
		}

		p_from_order++;
	}

	_pre_capture_automations();

	playback.playing = true;
	playback.loop_pattern = false;
	playback.pattern = order;
	playback.order = p_from_order;
	playback.bpm = bpm;
	playback.volume = 1.0;
	playback.prev_volume = 1.0;
	playback.pos = p_from_tick;
	playback.can_loop = p_can_loop;
	return true;
}
void Song::play_pattern(int p_pattern, Tick p_from_tick) {
	stop();

	_AUDIO_LOCK_

			_pre_capture_automations();

	playback.playing = true;
	playback.loop_pattern = true;
	playback.pattern = p_pattern;
	playback.order = -1;
	playback.bpm = bpm;
	playback.volume = 1.0;
	playback.prev_volume = 1.0;
	playback.pos = p_from_tick;
	playback.can_loop = true;
}
void Song::play_event_range(int p_pattern, int p_from_column, int p_to_column, Tick p_from_tick, Tick p_to_tick) {

	_AUDIO_LOCK_

			_pre_capture_automations();

	playback.range.active = true;
	playback.range.pattern = p_pattern;
	playback.range.from_column = p_from_column;
	playback.range.from_tick = p_from_tick;
	playback.range.to_column = p_to_column;
	playback.range.to_tick = p_to_tick;
}
void Song::play_single_event(int p_track, const AudioEffect::Event &p_event) {
	_AUDIO_LOCK_
			if (playback.single_event_count == SINGLE_EVENT_MAX) {
		return;
	}
	if (p_track < 0 || p_track >= tracks.size()) {
		return;
	}
	playback.single_event[playback.single_event_count].event = p_event;
	playback.single_event[playback.single_event_count].track = p_track;
	playback.single_event_count++;
}
void Song::stop() {
	_AUDIO_LOCK_

	playback.playing = false;
	playback.can_loop = true;
	//important, restore before stopping because stopping may call reset, which may still
	//restore to an own value/
	_restore_automations();

	for (int i = 0; i < tracks.size(); i++) {
		tracks[i]->stop();
	}

	midi_buffer_pos = 0;
	main_thread_midi_buffer_pos=0;
}

void Song::play_next_pattern() {
	_AUDIO_LOCK_

			if (!playback.playing || playback.loop_pattern) {
		return;
	}

	int order = playback.order;
	int pattern;
	while (true) {
		order++;
		if (order > ORDER_MAX) {
			return;
		}
		pattern = order_get(order);
		if (pattern == ORDER_EMPTY) {
			return;
		} else if (pattern != ORDER_SKIP) {
			break;
		}
	}

	playback.order = order;
	playback.pattern = pattern;
	playback.pos = 0;
}
void Song::play_prev_pattern() {
	_AUDIO_LOCK_

			if (!playback.playing || playback.loop_pattern) {
		return;
	}

	int order = playback.order;
	int pattern;
	while (true) {
		order--;
		if (order < 0) {
			return;
		}
		pattern = order_get(order);
		if (pattern == ORDER_EMPTY) {
			return;
		} else if (pattern != ORDER_SKIP) {
			break;
		}
	}

	playback.order = order;
	playback.pattern = pattern;
	playback.pos = 0;
}

bool Song::is_playing() const {
	return playback.playing;
}
int Song::get_playing_order() const {
	if (playback.playing && !playback.loop_pattern) {
		return playback.order;
	} else {
		return -1;
	}
}
int Song::get_playing_pattern() const {
	if (playback.playing) {
		return playback.pattern;
	} else {
		return -1;
	}
}
Tick Song::get_playing_tick() const {
	if (playback.playing) {
		return playback.pos;
	} else {
		return 0;
	}
}


void Song::set_mute_track(int p_track,bool p_muted) {
	ERR_FAIL_INDEX(p_track,tracks.size());
	tracks[p_track]->set_muted(_dispatch_main_thread_event,this,p_muted);
}

void Song::clear() {
	for (int i = 0; i < tracks.size(); i++)
		delete tracks[i];

	tracks.clear();
	bpm = DEFAULT_BPM;
	swing = 0;
	name = "";
	author = "";
	description = "";
	order_list.clear();
	pattern_config.clear();
	track_process_order.clear();
	midi_buffer_pos = 0;
	main_thread_midi_buffer_pos=0;
}

void Song::set_main_volume_db(float p_db) {
	main_volume_db = p_db;
}
float Song::get_main_volume_db() const {
	return main_volume_db;
}

float Song::get_peak_volume_db_l() const {
	float peak_db = linear2db(peak_volume_l);
	peak_volume_l = 0;
	return peak_db;
}

float Song::get_peak_volume_db_r() const {
	float peak_db = linear2db(peak_volume_r);
	peak_volume_r = 0;
	return peak_db;
}

Song::~Song() {

	clear();
}

Song::Song() {
	bpm = DEFAULT_BPM;
	swing = 0;
	set_process_buffer_size(DEFAULT_PROCESS_BUFFER_SIZE);
	buffer_pos = DEFAULT_PROCESS_BUFFER_SIZE;
	sampling_rate = 44100;
	playback.playing = false;
	playback.pattern = -1;
	playback.order = -1;
	playback.pos = 0;
	playback.bpm = bpm;
	playback.volume = 1.0;
	playback.prev_volume = 1.0;
	playback.range.active = false;
	playback.can_loop = true;
	playback.single_event_count = 0;
	main_volume_db = -12;
	peak_volume_l = 0;
	peak_volume_r = 0;
	playback.single_event_count = 0;
}
