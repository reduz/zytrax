#include "track.h"
#include <stdio.h>

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

void Automation::set_visible(bool p_visible) {

	visible = p_visible;
}

void Automation::set_display_mode(DisplayMode p_mode) {

	display_mode = p_mode;
}

bool Automation::is_visible() const {

	return visible;
}

Automation::DisplayMode Automation::get_display_mode() const {

	return display_mode;
}

bool Automation::is_empty() const {
	return (visible && display_mode == DISPLAY_ROWS && data.empty());
}

Automation::Automation(ControlPort *p_port, AudioEffect *p_owner) {

	port = p_port;
	owner = p_owner;
	display_mode = DISPLAY_ROWS;
	visible = true;
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
	automations.insert(p_pos, p_automation);
}
void Track::remove_automation(int p_pos) {

	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_pos, automations.size());
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

///
int Track::get_event_column_count() const {

	int visible_autos = 0;
	for (int i = 0; i < automations.size(); i++) {
		if (automations[i]->is_visible())
			visible_autos++;
	}
	return note_columns + visible_autos;
}

void Track::set_event(int p_pattern, int p_column, Tick p_pos, const Event &p_event) {

	ERR_FAIL_INDEX(p_column, get_event_column_count());

	if (p_column < note_columns) {
		//note
		Pos p;
		p.column = p_column;
		p.tick = p_pos;
		set_note(p_pattern, p, p_event);

	} else {

		p_column -= note_columns;
		for (int i = 0; i < automations.size(); i++) {
			if (!automations[i]->is_visible())
				continue;

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
	} else {

		p_column -= note_columns;
		for (int i = 0; i < automations.size(); i++) {
			if (!automations[i]->is_visible())
				continue;

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

	if (p_from.column < note_columns) {
		//has notes
		List<PosNote> pn;
		Pos end = p_to;
		if (end.column >= note_columns)
			end.column = note_columns - 1;

		get_notes_in_range(p_pattern, p_from, end, &pn);

		for (const List<PosNote>::Element *E = pn.front(); E; E = E->next()) {

			events.insert(E->get().pos, E->get().note);
		}
	}

	if (p_to.column >= note_columns) {
		//has commands

		int begin = p_from.column - (note_columns);
		int end = p_to.column - (note_columns);

		if (begin < 0)
			begin = 0;

		if (end >= automations.size())
			end = automations.size() - 1;

		for (int i = begin; i <= end; i++) {

			int f, c;
			automations[i]->get_points_in_range(p_pattern, p_from.tick, p_to.tick, f, c);
			for (int j = 0; j < c; j++) {

				uint8_t v = automations[i]->get_point_by_index(p_pattern, j + f);
				Tick t = automations[i]->get_point_tick_by_index(p_pattern, j + f);
				Pos p;
				p.column = i + note_columns;
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

void Track::set_swing_step(int p_swing_step) {

	_AUDIO_LOCK_
	swing_step = p_swing_step;
}
int Track::get_swing_step() const {

	return swing_step;
}

ControlPort *Track::get_volume_port() {

	return &volume;
}
ControlPort *Track::get_pan_port() {

	return &pan;
}
ControlPort *Track::get_swing_port() {

	return &swing;
}

void Track::set_muted(bool p_mute) {

	muted = p_mute;
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

float Track::get_peak_volume_db() const {
	return -40;
}

void Track::add_send(int p_track, int p_pos) {

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
	ERR_FAIL_INDEX(p_send, sends.size());
	sends.remove(p_send);
}

void Track::swap_sends(int p_send, int p_with_send) {
	_AUDIO_LOCK_
	ERR_FAIL_INDEX(p_send, sends.size());
	ERR_FAIL_INDEX(p_with_send, sends.size());
	SWAP(sends[p_send], sends[p_with_send]);
}

Track::Track() {

	swing.name = "Swing";
	swing.min = 0;
	swing.max = 1;
	swing.step = 0.01;
	swing.initial = 0;
	swing.value = 0;

	volume.name = "Volume";
	volume.max = 1;
	volume.min = 0;
	volume.step = 0.01;
	volume.initial = 0.7;
	volume.value = 0.7;

	pan.name = "Pan";
	pan.max = 1;
	pan.min = -1;
	pan.step = 0.01;
	pan.initial = 0;
	pan.value = 0;

	name = "New Track";
	swing_step = 1;
	note_columns = 1;

	mix_volume = -12;

	muted = false;
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
