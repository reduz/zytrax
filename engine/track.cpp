#include "track.h"
#include "dsp/db.h"
#include "song.h"
#include <stdio.h>

void Automation::_ui_changed_callbacks(void *p_ud) {
	Automation *automation = (Automation *)p_ud;
	automation->_ui_changed_callback();
}
void Automation::_ui_changed_callback() {
	if (has_pre_play_capture) {
		pre_play_capture_value = port->get(); //update pre play capture because UI modified it
	}
}
void Automation::set_point(int p_pattern, Tick p_offset, uint8_t p_value) {

	_AUDIO_LOCK_

	if (p_value == EMPTY) {
		remove_point(p_pattern, p_offset);
		return;
	}

	if (!data.has(p_pattern)) {

		data[p_pattern] = ValueStream<Tick, uint8_t>();
	}

	data[p_pattern].insert(p_offset, p_value);
}

bool Automation::has_point(int p_pattern, Tick p_offset) const {

	if (!data.has(p_pattern))
		return false;
	return data[p_pattern].find_exact(p_offset) >= 0;
}

uint8_t Automation::get_point(int p_pattern, Tick p_offset) const {

	if (!data.has(p_pattern))
		return EMPTY;

	int idx = data[p_pattern].find_exact(p_offset);
	if (idx < 0)
		return EMPTY;
	return data[p_pattern][idx];
}

void Automation::remove_point(int p_pattern, Tick p_offset) {

	if (!data.has(p_pattern))
		return;

	_AUDIO_LOCK_

	int idx = data[p_pattern].find_exact(p_offset);
	if (idx < 0)
		return;
	data[p_pattern].erase(idx);
	if (data[p_pattern].size() == 0)
		data.erase(p_pattern);
}

Tick Automation::get_point_tick_by_index(int p_pattern, int p_index) const {

	// this is used super often when playing, so it should be more optimized

	const Map<int, ValueStream<Tick, uint8_t> >::Element *E = data.find(p_pattern);
	ERR_FAIL_COND_V(!E, 0);

	ERR_FAIL_INDEX_V(p_index, E->get().size(), 0);
	return E->get().get_pos(p_index);
}

uint8_t Automation::get_point_by_index(int p_pattern, int p_index) const {

	// this is used super often when playing, so it should be more optimized

	const Map<int, ValueStream<Tick, uint8_t> >::Element *E = data.find(p_pattern);
	ERR_FAIL_COND_V(!E, 0);
	ERR_FAIL_INDEX_V(p_index, E->get().size(), 0);
	return E->get()[p_index];
}

int Automation::get_point_count(int p_pattern) const {

	const Map<int, ValueStream<Tick, uint8_t> >::Element *E = data.find(p_pattern);
	if (!E)
		return 0;

	return E->get().size();
}

float Automation::interpolate_offset(int p_pattern, Tick p_offset) const {

	const Map<int, ValueStream<Tick, uint8_t> >::Element *E = data.find(p_pattern);
	if (!E) {
		return -1;
	}

	const ValueStream<Tick, uint8_t> &vs = E->get();

	int pos = vs.find(p_offset);
	int total = vs.size();

	if (pos < 0 || pos >= total)
		return -1;
	int n = pos + 1;
	if (n >= total)
		return -1;
	float c = float(p_offset - vs.get_pos(pos)) / float(vs.get_pos(n) - vs.get_pos(pos));
	float a = (vs[pos] / float(VALUE_MAX));
	float b = (vs[n] / float(VALUE_MAX));

	return b * c + a * (1.0 - c);
}

void Automation::get_points_in_range(int p_pattern, Tick p_from, Tick p_to, int &r_first, int &r_count) const {

	const Map<int, ValueStream<Tick, uint8_t> >::Element *E = data.find(p_pattern);
	if (!E) {
		r_count = 0;
		return;
	}

	const ValueStream<Tick, uint8_t> &vs = E->get();

	if (vs.size() == 0) {
		r_count = 0;
		return;
	}

	int pos_beg = vs.find(p_from);
	int pos_end = vs.find(p_to);

	if (pos_end >= 0 && p_to == vs.get_pos(pos_end)) {
		pos_end--;
	}

	if (pos_end < 0) {
		r_count = 0;
		return;
	}

	if (pos_beg < 0 || vs.get_pos(pos_beg) < p_from)
		pos_beg++;

	if (pos_beg > pos_end) {
		r_count = 0;
		return;
	}

	r_first = pos_beg;
	r_count = pos_end - pos_beg + 1;
}

ControlPort *Automation::get_control_port() {

	return port;
}

AudioEffect *Automation::get_owner() {

	return owner;
}

void Automation::set_edit_mode(EditMode p_mode) {

	display_mode = p_mode;
}

Automation::EditMode Automation::get_edit_mode() const {

	return display_mode;
}

bool Automation::is_empty() const {
	return (display_mode == EDIT_ROWS_DISCRETE && data.empty());
}

void Automation::pre_play_capture() {

	if (has_pre_play_capture) {
		return; //do not re-capture
	}

	pre_play_capture_value = port->get();
	has_pre_play_capture = true;
}
void Automation::pre_play_restore() {

	if (has_pre_play_capture) {
		port->set(pre_play_capture_value);
		has_pre_play_capture = false;
	};
}

void Automation::add_notify() {
	has_pre_play_capture = false; //should be false but just in case
	port->set_ui_changed_callback(&_ui_changed_callbacks, this);
}
void Automation::remove_notify() {
	pre_play_restore();
	port->set_ui_changed_callback(NULL, NULL);
}

Automation::Automation(ControlPort *p_port, AudioEffect *p_owner) {

	port = p_port;
	owner = p_owner;
	display_mode = EDIT_ROWS_DISCRETE;
	has_pre_play_capture = false;
	pre_play_capture_value = 0;
}

/* audio effects */

void Track::set_name(String p_name) {

	name = p_name;
}

String Track::get_name() const {

	return name;
}

int Track::get_audio_effect_count() const {

	return effects.size();
}

void Track::add_audio_effect(AudioEffect *p_effect, int p_pos) {

	_AUDIO_LOCK_

	if (p_pos < 0)
		p_pos = effects.size();
	ERR_FAIL_COND(p_pos > effects.size());
	effects.insert(p_pos, p_effect);
	p_effect->set_process_block_size(process_buffer.size());
	p_effect->set_sampling_rate(sampling_rate);
}

void Track::remove_audio_effect(int p_pos) {

	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_pos, effects.size());
	AudioEffect *fx = effects[p_pos];

	for (int i = 0; i < automations.size(); i++) {

		if (automations[i]->get_owner() == fx) {

			automations.remove(i);
			i--;
		}
	}

	effects.remove(p_pos);
}

void Track::swap_audio_effects(int p_effect, int p_with_effect) {
	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_effect, effects.size());
	ERR_FAIL_INDEX(p_with_effect, effects.size());
	SWAP(effects[p_effect], effects[p_with_effect]);
}
AudioEffect *Track::get_audio_effect(int p_pos) {

	ERR_FAIL_INDEX_V(p_pos, effects.size(), NULL);
	return effects[p_pos];
}

/* automations */

int Track::get_automation_count() const {

	return automations.size();
}
void Track::add_automation(Automation *p_automation, int p_pos) {

	_AUDIO_LOCK_
	if (p_pos < 0)
		p_pos = automations.size();
	ERR_FAIL_COND(p_pos > automations.size());
	p_automation->add_notify();
	automations.insert(p_pos, p_automation);
}
void Track::remove_automation(int p_pos) {

	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_pos, automations.size());
	automations[p_pos]->remove_notify();
	automations.remove(p_pos);
}
Automation *Track::get_automation(int p_pos) const {

	ERR_FAIL_INDEX_V(p_pos, automations.size(), NULL);
	return automations[p_pos];
}

void Track::swap_automations(int p_which, int p_by_which) {
	_AUDIO_LOCK_

	SWAP(automations[p_which], automations[p_by_which]);
}
// disabled automations

int Track::get_disabled_automation_count() const {

	return disabled_automations.size();
}
void Track::add_disabled_automation(Automation *p_automation, int p_pos) {

	_AUDIO_LOCK_
	if (p_pos < 0) {
		p_pos = disabled_automations.size();
	}
	ERR_FAIL_COND(p_pos > disabled_automations.size());
	disabled_automations.insert(p_pos, p_automation);
}
void Track::remove_disabled_automation(int p_pos) {

	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_pos, disabled_automations.size());
	disabled_automations.remove(p_pos);
}
Automation *Track::get_disabled_automation(int p_pos) const {

	ERR_FAIL_INDEX_V(p_pos, disabled_automations.size(), NULL);
	return disabled_automations[p_pos];
}

////
void Track::set_columns(int p_columns) {

	_AUDIO_LOCK_

	ERR_FAIL_COND(p_columns < 1);

	note_columns = p_columns;
	column_state.resize(note_columns);
	for (int i = 0; i < column_state.size(); i++) {
		column_state[i] = Note::EMPTY;
	}
}

int Track::get_column_count() const {

	return note_columns;
}

void Track::set_note(int p_pattern, Pos p_pos, Note p_note) {

	_AUDIO_LOCK_

	if (!note_data.has(p_pattern))
		note_data[p_pattern] = ValueStream<Pos, Note>();

	if (p_note.is_empty()) {
		int idx = note_data[p_pattern].find_exact(p_pos);
		if (idx < 0)
			return;

		note_data[p_pattern].erase(idx);
	} else {
		note_data[p_pattern].insert(p_pos, p_note);
	}
}
Track::Note Track::get_note(int p_pattern, Pos p_pos) const {

	const Map<int, ValueStream<Pos, Note> >::Element *E = note_data.find(p_pattern);

	if (!E)
		return Note();

	int idx = E->get().find_exact(p_pos);
	if (idx < 0)
		return Note();
	else
		return E->get()[idx];
}

void Track::get_notes_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, int &r_first, int &r_count) const {

	const Map<int, ValueStream<Pos, Note> >::Element *E = note_data.find(p_pattern);
	if (!E) {
		r_count = 0;
		return;
	}

	const ValueStream<Pos, Note> &vs = E->get();

	if (vs.size() == 0) {
		r_count = 0;
		return;
	}

	int pos_beg = vs.find(p_from);
	int pos_end = vs.find(p_to);

	if (pos_end >= 0 && p_to == vs.get_pos(pos_end)) {
		pos_end--;
	}

	if (pos_end < 0) {
		r_count = 0;
		return;
	}

	if (pos_beg < 0 || vs.get_pos(pos_beg) < p_from)
		pos_beg++;

	if (pos_beg > pos_end) {
		r_count = 0;
		return;
	}

	r_first = pos_beg;
	r_count = pos_end - pos_beg + 1;
}

int Track::get_note_count(int p_pattern) const {

	const Map<int, ValueStream<Pos, Note> >::Element *E = note_data.find(p_pattern);
	if (!E)
		return 0;

	return E->get().size();
}

Track::Note Track::get_note_by_index(int p_pattern, int p_index) const {

	const Map<int, ValueStream<Pos, Note> >::Element *E = note_data.find(p_pattern);
	if (!E)
		return Note();

	const ValueStream<Pos, Note> &vs = E->get();
	ERR_FAIL_INDEX_V(p_index, vs.size(), Note());
	return vs[p_index];
}
Track::Pos Track::get_note_pos_by_index(int p_pattern, int p_index) const {

	const Map<int, ValueStream<Pos, Note> >::Element *E = note_data.find(p_pattern);
	if (!E)
		return Pos();

	const ValueStream<Pos, Note> &vs = E->get();
	ERR_FAIL_INDEX_V(p_index, vs.size(), Pos());
	return vs.get_pos(p_index);
}

void Track::get_notes_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosNote> *r_notes) const {

	Pos from = p_from;
	Pos to = p_to;
	if (from.column > to.column) {
		SWAP(from.column, to.column);
	}
	if (from.tick > to.tick) {
		SWAP(from.tick, to.tick);
	}

	int fromidx;
	int count;
	get_notes_in_range(p_pattern, from, to, fromidx, count);

	for (int i = 0; i < count; i++) {
		PosNote pn;
		pn.pos = get_note_pos_by_index(p_pattern, i + fromidx);
		if (pn.pos.column < from.column || pn.pos.column > to.column)
			continue;
		pn.note = get_note_by_index(p_pattern, i + fromidx);
		r_notes->push_back(pn);
	}
}
////
void Track::set_command_columns(int p_columns) {

	_AUDIO_LOCK_

	ERR_FAIL_COND(p_columns < 0);

	command_columns = p_columns;
	for (int i = 0; i < column_state.size(); i++) {
		column_state[i] = Command::EMPTY;
	}
}

int Track::get_command_column_count() const {

	return command_columns;
}

void Track::set_command(int p_pattern, Pos p_pos, Command p_command) {

	_AUDIO_LOCK_

	if (!command_data.has(p_pattern))
		command_data[p_pattern] = ValueStream<Pos, Command>();

	if (p_command.is_empty()) {
		int idx = command_data[p_pattern].find_exact(p_pos);
		if (idx < 0)
			return;

		command_data[p_pattern].erase(idx);
	} else {
		command_data[p_pattern].insert(p_pos, p_command);
	}
}
Track::Command Track::get_command(int p_pattern, Pos p_pos) const {

	const Map<int, ValueStream<Pos, Command> >::Element *E = command_data.find(p_pattern);

	if (!E)
		return Command();

	int idx = E->get().find_exact(p_pos);
	if (idx < 0)
		return Command();
	else
		return E->get()[idx];
}

void Track::get_commands_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, int &r_first, int &r_count) const {

	const Map<int, ValueStream<Pos, Command> >::Element *E = command_data.find(p_pattern);
	if (!E) {
		r_count = 0;
		return;
	}

	const ValueStream<Pos, Command> &vs = E->get();

	if (vs.size() == 0) {
		r_count = 0;
		return;
	}

	int pos_beg = vs.find(p_from);
	int pos_end = vs.find(p_to);

	if (pos_end >= 0 && p_to == vs.get_pos(pos_end)) {
		pos_end--;
	}

	if (pos_end < 0) {
		r_count = 0;
		return;
	}

	if (pos_beg < 0 || vs.get_pos(pos_beg) < p_from)
		pos_beg++;

	if (pos_beg > pos_end) {
		r_count = 0;
		return;
	}

	r_first = pos_beg;
	r_count = pos_end - pos_beg + 1;
}

int Track::get_command_count(int p_pattern) const {

	const Map<int, ValueStream<Pos, Command> >::Element *E = command_data.find(p_pattern);
	if (!E)
		return 0;

	return E->get().size();
}

Track::Command Track::get_command_by_index(int p_pattern, int p_index) const {

	const Map<int, ValueStream<Pos, Command> >::Element *E = command_data.find(p_pattern);
	if (!E)
		return Command();

	const ValueStream<Pos, Command> &vs = E->get();
	ERR_FAIL_INDEX_V(p_index, vs.size(), Command());
	return vs[p_index];
}
Track::Pos Track::get_command_pos_by_index(int p_pattern, int p_index) const {

	const Map<int, ValueStream<Pos, Command> >::Element *E = command_data.find(p_pattern);
	if (!E)
		return Pos();

	const ValueStream<Pos, Command> &vs = E->get();
	ERR_FAIL_INDEX_V(p_index, vs.size(), Pos());
	return vs.get_pos(p_index);
}

void Track::get_commands_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosCommand> *r_commands) const {

	Pos from = p_from;
	Pos to = p_to;
	if (from.column > to.column) {
		SWAP(from.column, to.column);
	}
	if (from.tick > to.tick) {
		SWAP(from.tick, to.tick);
	}

	int fromidx;
	int count;
	get_commands_in_range(p_pattern, from, to, fromidx, count);

	for (int i = 0; i < count; i++) {
		PosCommand pn;
		pn.pos = get_command_pos_by_index(p_pattern, i + fromidx);
		if (pn.pos.column < from.column || pn.pos.column > to.column)
			continue;
		pn.command = get_command_by_index(p_pattern, i + fromidx);
		r_commands->push_back(pn);
	}
}

///
int Track::get_event_column_count() const {

	return note_columns + command_columns + automations.size();
}

void Track::set_event(int p_pattern, int p_column, Tick p_pos, const Event &p_event) {

	_AUDIO_LOCK_

	ERR_FAIL_INDEX(p_column, get_event_column_count());

	if (p_column < note_columns) {
		//note
		ERR_FAIL_COND(p_event.type != Event::TYPE_NOTE);
		Pos p;
		p.column = p_column;
		p.tick = p_pos;
		set_note(p_pattern, p, p_event);

		return;
	}

	p_column -= note_columns;

	if (p_column < command_columns) {
		//command
		ERR_FAIL_COND(p_event.type != Event::TYPE_COMMAND);
		Pos p;
		p.column = p_column;
		p.tick = p_pos;
		set_command(p_pattern, p, p_event);

		return;
	}

	p_column -= command_columns;

	{

		ERR_FAIL_COND(p_event.type != Event::TYPE_AUTOMATION);
		for (int i = 0; i < automations.size(); i++) {

			if (p_column == 0) {
				get_automation(i)->set_point(p_pattern, p_pos, p_event.a);
				return;
			}
			p_column--;
		}
	}
}

Track::Event::Type Track::get_event_column_type(int p_column) const {

	if (p_column < note_columns)
		return Event::TYPE_NOTE;
	else if (p_column < note_columns + command_columns)
		return Event::TYPE_COMMAND;
	else
		return Event::TYPE_AUTOMATION;
}

Track::Event Track::get_event(int p_pattern, int p_column, Tick p_pos) const {

	ERR_FAIL_INDEX_V(p_column, get_event_column_count(), Event());

	if (p_column < note_columns) {
		//note
		Pos p;
		p.column = p_column;
		p.tick = p_pos;
		return get_note(p_pattern, p);
	}

	p_column -= note_columns;

	if (p_column < command_columns) {
		//command
		Pos p;
		p.column = p_column;
		p.tick = p_pos;
		return get_command(p_pattern, p);
	}

	p_column -= command_columns;
	{
		for (int i = 0; i < automations.size(); i++) {

			if (p_column == 0) {
				return get_automation(i)->get_point(p_pattern, p_pos);
			}
			p_column--;
		}
	}

	ERR_FAIL_COND_V(true, Event());
}

void Track::get_events_in_range(int p_pattern, const Pos &p_from, const Pos &p_to, List<PosEvent> *r_events) const {

	Map<Pos, Event> events;

	Pos events_from = p_from;
	Pos events_to = p_to;

	if (events_from.column < note_columns) {
		//has notes
		List<PosNote> pn;
		Pos end = events_to;
		if (end.column >= note_columns)
			end.column = note_columns - 1;

		get_notes_in_range(p_pattern, events_from, end, &pn);

		for (const List<PosNote>::Element *E = pn.front(); E; E = E->next()) {

			events.insert(E->get().pos, E->get().note);
		}
	}

	events_from.column -= note_columns;
	events_to.column -= note_columns;

	if (!(events_to.column < 0 || events_from.column >= command_columns)) {
		//has commands
		List<PosCommand> pn;
		Pos from = events_from;
		from.column = MAX(0, from.column);
		Pos end = events_to;
		end.column = MIN(command_columns - 1, end.column);

		get_commands_in_range(p_pattern, from, end, &pn);

		for (const List<PosCommand>::Element *E = pn.front(); E; E = E->next()) {

			Pos evpos;
			evpos = E->get().pos;
			evpos.column += note_columns;
			events.insert(evpos, E->get().command);
		}
	}

	events_from.column -= command_columns;
	events_to.column -= command_columns;

	if (!(events_to.column < 0 || events_from.column >= automations.size())) {
		//has commands

		int begin = MAX(0, events_from.column);
		int end = MIN(automations.size() - 1, events_to.column);

		for (int i = begin; i <= end; i++) {

			int f, c;
			automations[i]->get_points_in_range(p_pattern, events_from.tick, events_to.tick, f, c);
			for (int j = 0; j < c; j++) {

				uint8_t v = automations[i]->get_point_by_index(p_pattern, j + f);
				Tick t = automations[i]->get_point_tick_by_index(p_pattern, j + f);
				Pos p;
				p.column = i + note_columns + command_columns;
				p.tick = t;
				events.insert(p, v);
			}
		}
	}

	//add everything beautifully ordered
	for (Map<Pos, Event>::Element *E = events.front(); E; E = E->next()) {

		PosEvent pe;
		pe.pos = E->key();
		pe.event = E->get();
		r_events->push_back(pe);
	}
}

void Track::set_muted(bool p_mute) {

	_AUDIO_LOCK_

	if (muted == p_mute) {
		return;
	}
	muted = p_mute;
	first_mix = true; //clear memories when unmuted
}

bool Track::is_muted() const {
	return muted;
}

void Track::set_mix_volume_db(float p_db) {
	mix_volume = p_db;
}
float Track::get_mix_volume_db() const {
	return mix_volume;
}

float Track::get_peak_volume_db_l() const {
	float pvolume = linear2db(peak_volume_l);
	peak_volume_l = 0;
	return pvolume;
}

float Track::get_peak_volume_db_r() const {
	float pvolume = linear2db(peak_volume_r);
	peak_volume_r = 0;
	return pvolume;
}

void Track::add_send(int p_track, int p_pos) {

	_AUDIO_LOCK_

	for (int i = 0; i < sends.size(); i++) {
		ERR_FAIL_COND(sends[i].track == p_track);
	}

	Send send;
	send.amount = 1.0;
	send.track = p_track;
	send.mute = false;
	if (p_pos < 0 || p_pos >= sends.size()) {
		sends.push_back(send);
	} else {
		sends.insert(p_pos, send);
	}
}
void Track::set_send_amount(int p_send, float p_amount) {

	ERR_FAIL_INDEX(p_send, sends.size());
	sends[p_send].amount = p_amount;
}

void Track::set_send_track(int p_send, int p_track) {
	_AUDIO_LOCK_

	ERR_FAIL_INDEX(p_send, sends.size());
//cant validate, unfortunately
#if 0
	for (int i = 0; i < sends.size(); i++) {
		if (sends[i].track == p_send) {
			continue;
		}
		ERR_FAIL_COND(sends[i].track == p_track);
	}
#endif
	sends[p_send].track = p_track;
}
void Track::set_send_mute(int p_send, bool p_mute) {

	ERR_FAIL_INDEX(p_send, sends.size());
	sends[p_send].mute = p_mute;
}

int Track::get_send_track(int p_send) const {
	ERR_FAIL_INDEX_V(p_send, sends.size(), SEND_SPEAKERS);
	return sends[p_send].track;
}
float Track::get_send_amount(int p_send) const {
	ERR_FAIL_INDEX_V(p_send, sends.size(), SEND_SPEAKERS);
	return sends[p_send].amount;
}

bool Track::is_send_muted(int p_send) const {
	ERR_FAIL_INDEX_V(p_send, sends.size(), false);
	return sends[p_send].mute;
}

int Track::get_send_count() {
	return sends.size();
}
void Track::remove_send(int p_send) {
	_AUDIO_LOCK_

	ERR_FAIL_INDEX(p_send, sends.size());
	sends.remove(p_send);
}

void Track::swap_sends(int p_send, int p_with_send) {
	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_send, sends.size());
	ERR_FAIL_INDEX(p_with_send, sends.size());
	SWAP(sends[p_send], sends[p_with_send]);
}

bool Track::has_send(int p_send) const {
	for (int i = 0; i < sends.size(); i++) {
		if (sends[i].track == p_send) {
			return true;
		}
	}
	return false;
}
void Track::set_process_buffer_size(int p_frames) {
	_AUDIO_LOCK_
	if (input_buffer.size() == p_frames) {
		return;
	}
	input_buffer.resize(p_frames);
	process_buffer.resize(p_frames);
	process_buffer2.resize(p_frames);
	for (int i = 0; i < effects.size(); i++) {
		effects[i]->set_process_block_size(p_frames);
	}
}

void Track::set_sampling_rate(int p_hz) {
	if (sampling_rate == p_hz) {
		return;
	}
	sampling_rate = p_hz;
	for (int i = 0; i < effects.size(); i++) {
		effects[i]->set_sampling_rate(p_hz);
	}
}

Tick Track::_get_swinged_tick(Tick p_tick, int p_swing_divisor, float p_swing) {
	if (p_swing_divisor < 0 || p_swing_divisor >= Song::SWING_BEAT_DIVISOR_MAX) {
		return p_tick;
	}
	static const int divisors[Song::SWING_BEAT_DIVISOR_MAX] = { 1, 2, 3, 4, 6, 8 };

	Tick tick_frac_size = TICKS_PER_BEAT / divisors[p_swing_divisor];
	Tick tick_frac = p_tick % tick_frac_size;
	Tick tick_debased = p_tick - tick_frac;

	Tick swing_split = int(((1.0 + p_swing) * (double)tick_frac_size) / 2.0);

	if (tick_frac <= swing_split) {

		tick_frac = tick_frac * (tick_frac_size / 2) / swing_split;

	} else {

		tick_frac = tick_frac_size - tick_frac; //invert

		Tick diff = tick_frac_size - swing_split;
		if (diff == 0)
			tick_frac = tick_frac_size;
		else
			tick_frac = tick_frac * (tick_frac_size / 2) / diff;

		tick_frac = tick_frac_size - tick_frac; //revert
	}

	return tick_frac + tick_debased;
}
void Track::process_events(int p_pattern, Tick p_offset, Tick p_from_tick, Tick p_to_tick, int p_bpm, int p_swing_divisor, float p_swing, int p_from, int p_to) {

	if (event_buffer_size == EVENT_BUFFER_MAX) {
		return;
	}
	if (p_offset == 0) {
		//add the bpm
		event_buffer[event_buffer_size].type = AudioEffect::Event::TYPE_BPM;
		event_buffer[event_buffer_size].param8 = p_bpm;
		event_buffer[event_buffer_size].paramf = 0;
		event_buffer_size++;
	}

	p_from_tick = _get_swinged_tick(p_from_tick, p_swing_divisor, p_swing);
	p_to_tick = _get_swinged_tick(p_to_tick, p_swing_divisor, p_swing);

	int first, count;
	get_notes_in_range(p_pattern, p_from_tick, p_to_tick, first, count);

	double tick_to_frame = ((60.0 / double(p_bpm)) / double(TICKS_PER_BEAT)) * double(sampling_rate);

	for (int i = 0; i < count; i++) {

		if (event_buffer_size == EVENT_BUFFER_MAX) {
			return;
		}

		Note note = get_note_by_index(p_pattern, first + i);

		Pos pos = get_note_pos_by_index(p_pattern, first + i);

		if (pos.column >= note_columns) {
			continue;
		}

		if (p_from != -1 && p_from > pos.column) {
			continue;
		}
		if (p_to != -1 && p_to < pos.column) {
			continue;
		}

		int sample_offset = int(double(p_offset + (pos.tick - p_from_tick)) * tick_to_frame);

		if (note.note != Note::EMPTY && column_state[pos.column] != Note::EMPTY) {
			//note or note off, and note was playing in column: must turn off existing note

			event_buffer[event_buffer_size].type = AudioEffect::Event::TYPE_NOTE_OFF;
			event_buffer[event_buffer_size].param8 = column_state[pos.column];
			if (note.volume == Note::EMPTY || note.note == Note::OFF) { //new note, note-off quickly
				event_buffer[event_buffer_size].paramf = 1.0;
			} else {
				event_buffer[event_buffer_size].paramf = note.volume / 99.0; // map 0 .. 1
			}
			event_buffer[event_buffer_size].offset = sample_offset;

			event_buffer_size++;
			column_state[pos.column] = Note::EMPTY; //clear
		}

		if (note.note <= Note::MAX_NOTE) {
			//note on
			event_buffer[event_buffer_size].type = AudioEffect::Event::TYPE_NOTE;
			event_buffer[event_buffer_size].param8 = note.note;
			if (note.volume == Note::EMPTY) {
				event_buffer[event_buffer_size].paramf = 1.0;
			} else {
				event_buffer[event_buffer_size].paramf = note.volume / 99.0; // map 0 .. 1
			}
			event_buffer[event_buffer_size].offset = sample_offset;
			column_state[pos.column] = note.note; //clear

			event_buffer_size++;
		} else if (note.note == Note::EMPTY && note.volume != Note::EMPTY && column_state[pos.column] != Note::EMPTY) {
			//send volume change alone (may be supported via midi note pressure for regular MIDI)
			event_buffer[event_buffer_size].type = AudioEffect::Event::TYPE_AFTERTOUCH;
			event_buffer[event_buffer_size].param8 = column_state[pos.column];
			event_buffer[event_buffer_size].paramf = note.volume / 99.0; // map 0 .. 1
			event_buffer[event_buffer_size].offset = sample_offset;
			event_buffer_size++;
		}
	}

	int from_ofs = column_state.size();

	//process events
	get_commands_in_range(p_pattern, p_from_tick, p_to_tick, first, count);

	for (int i = 0; i < count; i++) {

		Command command = get_command_by_index(p_pattern, first + i);

		if (command.command == Command::EMPTY) {
			//well, do nothing.
			continue;
		}

		Pos pos = get_command_pos_by_index(p_pattern, first + i);

		if (pos.column >= command_columns) {
			//may be hidden
			continue;
		}
		if (p_from != -1 && p_from > pos.column + from_ofs) {
			continue;
		}
		if (p_to != -1 && p_to < pos.column + from_ofs) {
			continue;
		}

		//go on a quest to find the control ports and send commands, this should be optimized, though not as many commands
		//are processed, so its not too bad.

		float param_value = float(command.parameter) / float(Command::MAX_PARAM);

		for (int j = 0; j < effects.size(); j++) {
			AudioEffect *fx = effects[j];
			for (int k = 0; k < fx->get_control_port_count(); k++) {
				ControlPort *control_port = fx->get_control_port(k);
				if (control_port->get_command() == command.command) {
					control_port->set_normalized(param_value);
				}
			}
		}
	}

	from_ofs += command_columns;

	for (int j = 0; j < automations.size(); j++) {

		if (p_from != -1 && p_from > (j + from_ofs)) {
			continue;
		}
		if (p_to != -1 && p_to < (j + from_ofs)) {
			continue;
		}

		Automation *a = automations[j];
		if (a->get_edit_mode() == Automation::EDIT_ROWS_DISCRETE) {
			//set the row, without interpolation
			a->get_points_in_range(p_pattern, p_from_tick, p_to_tick, first, count);
			for (int i = 0; i < count; i++) {
				int value = a->get_point_by_index(p_pattern, first + i);
				float valuef = float(value) / 99.0;
				a->get_control_port()->set_normalized(valuef);
			}
		} else {
			//interpolate
			float value = a->interpolate_offset(p_pattern, p_to_tick);
			if (value == -1) {
				//if to is empty, use the value in from
				value = a->interpolate_offset(p_pattern, p_from_tick);
			}

			if (value != -1) {
				//must be something in there
				a->get_control_port()->set_normalized(value);
			}
		}
	}
}

const AudioFrame *Track::process_audio_step() {

	//see if any of the effects uses the secondary input
	int effect_count = effects.size();
	AudioEffect **effects_ptr = effect_count ? &effects[0] : NULL;

	bool has_side_input = false;
	for (int i = 0; i < effect_count; i++) {
		if (effects_ptr[i]->has_secondary_input() && !effects_ptr[i]->is_skipped()) {
			has_side_input = true;
		}
	}

	int buffer_len = input_buffer.size();

	const AudioFrame *input_buffer_ptr = &input_buffer[0];
	AudioFrame *process_buffer_src_ptr = &process_buffer[0];
	AudioFrame *process_buffer_dst_ptr = &process_buffer2[0];

	if (!has_side_input || muted) {
		//no side input, just copy input to process
		for (int i = 0; i < buffer_len; i++) {
			process_buffer_src_ptr[i] = input_buffer_ptr[i];
		}
	} else {
		//has side input, which will eventually be used.
		//zero the buffer
		for (int i = 0; i < buffer_len; i++) {
			process_buffer_src_ptr[i] = AudioFrame(0, 0);
		}
	}

	if (!muted) {
		for (int i = 0; i < effect_count; i++) {
			if (effects_ptr[i]->is_skipped()) {
				continue;
			}
			if (effects_ptr[i]->has_secondary_input()) {
				effects_ptr[i]->process_with_secondary(event_buffer, event_buffer_size, process_buffer_src_ptr, input_buffer_ptr, process_buffer_dst_ptr, first_mix);
			} else {
				effects_ptr[i]->process(event_buffer, event_buffer_size, process_buffer_src_ptr, process_buffer_dst_ptr, first_mix);
			}

			SWAP(process_buffer_src_ptr, process_buffer_dst_ptr);
		}
	}

	//apply volume
	{
		float volume = db2linear(mix_volume);
		for (int i = 0; i < buffer_len; i++) {
			process_buffer_src_ptr[i] *= volume;
			float energy_l = ABS(process_buffer_src_ptr[i].l);
			if (energy_l > peak_volume_l) {
				peak_volume_l = energy_l;
			}
			float energy_r = ABS(process_buffer_src_ptr[i].r);
			if (energy_r > peak_volume_r) {
				peak_volume_r = energy_r;
			}
		}
	}

	//clear mix and event count
	first_mix = false;
	event_buffer_size = 0;

	return process_buffer_src_ptr;
}

void Track::stop() {

	for (int i = 0; i < effects.size(); i++) {
		AudioEffect *fx = effects[i];
		fx->reset();
	}
	for (int i = 0; i < column_state.size(); i++) {
		column_state[i] = Note::EMPTY;
	}

	event_buffer_size = 0;
	first_mix = true;
}

void Track::automations_pre_play_capture() {
	for (int i = 0; i < automations.size(); i++) {
		automations[i]->pre_play_capture();
	}
}
void Track::automations_pre_play_restore() {
	for (int i = 0; i < automations.size(); i++) {
		automations[i]->pre_play_restore();
	}
}

Track::Track() {

	name = "New Track";

	note_columns = 1;
	command_columns = 0;
	sampling_rate = 44100;

	mix_volume = 0;

	event_buffer_size = 0;

	muted = false;
	first_mix = true;
	column_state.resize(1);
	column_state[0] = Note::EMPTY;
	peak_volume_l = peak_volume_r = -900;
}

Track::~Track() {
	for (int i = 0; i < effects.size(); i++) {
		delete effects[i];
	}
	for (int i = 0; i < automations.size(); i++) {
		delete automations[i];
	}
	for (int i = 0; i < disabled_automations.size(); i++) {
		delete disabled_automations[i];
	}
}
