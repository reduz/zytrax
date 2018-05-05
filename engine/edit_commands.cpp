#include "edit_commands.h"
#if 0
void EditCommands::automation_set_point(Automation *p_automation, int p_pattern, Tick p_offset, float p_value) {

	String action_name; // action is too generic, for performance and memory, name is omitted.

	CommandBase *undo=NULL;
	if (p_automation->has_point(p_pattern,p_offset)) {
		float prev = p_automation->get_point(p_pattern,p_offset);
		undo=command(p_automation,&Automation::set_point,p_pattern,p_offset,prev);
	} else {

		undo=command(p_automation,&Automation::remove_point,p_pattern,p_offset);
	}

	add_action(
		action_name,
		command(p_automation,&Automation::set_point,p_pattern,p_offset,p_value),
		undo
	);



}

void EditCommands::automation_remove_point(Automation *p_automation, int p_pattern, Tick p_offset) {

	String action_name; // action is too generic, for performance and memory, name is omitted.

	if (!p_automation->has_point(p_pattern,p_offset))
		return; //removing what isn't there is pointless..

	float prev = p_automation->get_point(p_pattern,p_offset);

	add_action(
		action_name,
		command(p_automation,&Automation::remove_point,p_pattern,p_offset),
		command(p_automation,&Automation::set_point,p_pattern,p_offset,prev)
	);

}

void EditCommands::track_add_audio_effect(Track *p_track, AudioEffect *p_effect,int p_pos) {

	String action_name = "Add Effect: "+p_effect->get_info()->caption;

	if (p_pos<0)
		p_pos=p_track->get_audio_effect_count();
	else if (p_pos>p_track->get_audio_effect_count())
		p_pos=p_track->get_audio_effect_count();

	add_action(
		action_name,
		command(p_track,&Track::add_audio_effect,p_effect,p_pos)->with_data(p_effect),
		command(p_track,&Track::remove_audio_effect,p_pos)
	);

}
void EditCommands::track_remove_audio_effect(Track *p_track, int p_pos) {

	ERR_FAIL_INDEX(p_pos,p_track->get_audio_effect_count());
	AudioEffect* effect = p_track->get_audio_effect(p_pos);

	String action_name = "Remove Effect: "+effect->get_info()->caption;

	add_action(
		action_name,
		command(p_track,&Track::remove_audio_effect,p_pos),
		command(p_track,&Track::add_audio_effect,effect,p_pos)->with_data(effect)
	);

}

void EditCommands::track_add_automation(Track *p_track, Automation *p_automation,int p_pos) {

	String action_name = "Add Automation: "+p_automation->get_control_port()->get_name();

	if (p_pos<0)
		p_pos=p_track->get_automation_count();
	else if (p_pos>p_track->get_automation_count())
		p_pos=p_track->get_automation_count();

	add_action(
		action_name,
		command(p_track,&Track::add_automation,p_automation,p_pos)->with_data(p_automation),
		command(p_track,&Track::remove_automation,p_pos)
	);

}
void EditCommands::track_remove_automation(Track *p_track, int p_pos) {

	ERR_FAIL_INDEX(p_pos,p_track->get_automation_count());
	Automation* automation = p_track->get_automation(p_pos);

	String action_name = "Remove Automation: "+automation->get_control_port()->get_name();

	add_action(
		action_name,
		command(p_track,&Track::remove_automation,p_pos),
		command(p_track,&Track::add_automation,automation,p_pos)->with_data(automation)
	);

}

void EditCommands::song_pattern_set_beats(Song *p_song, int p_pattern, int p_beats) {


	String action_name = "Pattern "+String::num(p_pattern)+" Set Beats";
	int old_beats = p_song->pattern_get_beats(p_pattern);
	add_action(
		action_name,
		command(p_song,&Song::pattern_set_beats,p_pattern,p_beats),
		command(p_song,&Song::pattern_set_beats,p_pattern,old_beats)
	);

}

void EditCommands::song_pattern_set_measure(Song *p_song, int p_pattern, int p_measure) {

	String action_name = "Pattern "+String::num(p_pattern)+" Set Measure";
	int old_measure = p_song->pattern_get_measure(p_pattern);
	add_action(
		action_name,
		command(p_song,&Song::pattern_set_measure,p_pattern,p_measure),
		command(p_song,&Song::pattern_set_measure,p_pattern,old_measure)
	);

}
void EditCommands::song_pattern_set_bars(Song *p_song, int p_pattern, int p_bars) {

	String action_name = "Pattern "+String::num(p_pattern)+" Set Bars";
	int old_bars = p_song->pattern_get_bars(p_pattern);
	add_action(
		action_name,
		command(p_song,&Song::pattern_set_bars,p_pattern,p_bars),
		command(p_song,&Song::pattern_set_bars,p_pattern,old_bars)
	);

}

void EditCommands::song_order_set(Song *p_song,int p_order, int p_pattern) {

	String action_name = "Set Order "+String::num(p_order);
	int old_pattern = p_song->order_get(p_order);
	add_action(
		action_name,
		command(p_song,&Song::order_set,p_order,p_pattern),
		command(p_song,&Song::order_set,p_order,old_pattern)
	);

}
void EditCommands::song_track_add(Song *p_song,Track *p_track,int p_pos) {

	String type;
	switch(p_track->get_type()) {
		case Track::TYPE_AUDIO: type="Audio"; break;
		case Track::TYPE_PATTERN: type="Pattern"; break;
		case Track::TYPE_GLOBAL: type="Global"; break;
	}

	String action_name = "Add "+type+" Track";
	// validate p_pos
	if (p_pos<0)
		p_pos=p_song->get_track_count(); //by default add last
	else if (p_pos>p_song->get_track_count())
		p_pos=p_song->get_track_count();

	add_action(
		action_name,
		command(p_song,&Song::add_track,p_track,p_pos)->with_data(p_track),
		command(p_song,&Song::remove_track,p_pos)
	);

}
void EditCommands::song_track_remove(Song *p_song,int p_pos) {

	String action_name = "Remove Track "+String::num(p_pos);
	// validate p_pos
	if (p_pos<0 || p_pos>=p_song->get_track_count())
		return;

	Track *track = p_song->get_track(p_pos);
	ERR_FAIL_COND(!track);

	add_action(
		action_name,
		command(p_song,&Song::remove_track,p_pos),
		command(p_song,&Song::add_track,track,p_pos)->with_data(track)
	);

}

void EditCommands::pattern_track_set_note_columns(PatternTrack *p_pattern_track,int p_columns) {

	String action_name = "Pattern Track Set To "+String::num(p_columns)+ " Note Columns";
	// validate p_pos
	ERR_FAIL_COND(p_columns<1 || p_columns>256);

	int old_columns=p_pattern_track->get_note_columns();

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_note_columns,p_columns),
		command(p_pattern_track,&PatternTrack::set_note_columns,old_columns)
	);
}

void EditCommands::pattern_track_set_note(PatternTrack *p_pattern_track,int p_pattern, PatternPos p_pos, PatternNote p_note) {

	String action_name; //action too generic
	// validate p_pos

	PatternNote old_note = p_pattern_track->get_note(p_pattern,p_pos);
	if (old_note==p_note)
		return;

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_note,p_pattern,p_pos,p_note),
		command(p_pattern_track,&PatternTrack::set_note,p_pattern,p_pos,old_note)
	);

}

void EditCommands::pattern_track_set_command_columns(PatternTrack *p_pattern_track,int p_columns) {

	String action_name = "Pattern Track Set To "+String::num(p_columns)+ " Command Columns";
	// validate p_pos
	ERR_FAIL_COND(p_columns<0 || p_columns>256);

	int old_columns=p_pattern_track->get_note_columns();

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_note_columns,p_columns),
		command(p_pattern_track,&PatternTrack::set_note_columns,old_columns)
	);
}
void EditCommands::pattern_track_set_command(PatternTrack *p_pattern_track,int p_pattern, PatternPos p_pos, PatternCommand p_command) {

	String action_name; //action too generic
	// validate p_pos
	PatternCommand old_command = p_pattern_track->get_command(p_pattern,p_pos);
	if (old_command==p_command)
		return;

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_command,p_pattern,p_pos,p_command),
		command(p_pattern_track,&PatternTrack::set_command,p_pattern,p_pos,old_command)
	);

}

void EditCommands::pattern_track_set_swing(PatternTrack *p_pattern_track,float p_swing) {

	String action_name = "Pattern Track Set Swing";
	// validate p_pos
	float old_swing=p_pattern_track->get_swing();

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_swing,p_swing),
		command(p_pattern_track,&PatternTrack::set_swing,old_swing)
	);

}
void EditCommands::pattern_track_set_swing_step(PatternTrack *p_pattern_track,int p_swing_step) {

	String action_name = "Pattern Track Set Swing Step";
	// validate p_pos
	int old_swing_step=p_pattern_track->get_swing_step();

	add_action(
		action_name,
		command(p_pattern_track,&PatternTrack::set_swing_step,p_swing_step),
		command(p_pattern_track,&PatternTrack::set_swing_step,old_swing_step)
	);

}
#endif
