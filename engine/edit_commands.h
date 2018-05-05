#ifndef EDIT_COMMANDS_H
#define EDIT_COMMANDS_H

#include "undo_redo.h"
#include "song.h"


#if 0
class EditCommands : public UndoRedo {


public:

	/* Automation */

	void automation_set_point(Automation *p_automation, int p_pattern, Tick p_offset, float p_value);
	void automation_remove_point(Automation *p_automation, int p_pattern, Tick p_offset);

	/* Track */

	void track_add_audio_effect(Track *p_track, AudioEffect *p_effect,int p_pos=-1);
	void track_remove_audio_effect(Track *p_track, int p_pos);

	void track_add_automation(Track *p_track,Automation *p_automation,int p_pos=-1);
	void track_remove_automation(Track *p_track,int p_pos);

	/* Song */

	void song_pattern_set_beats(Song *p_song, int p_pattern, int p_beats);
	void song_pattern_set_measure(Song *p_song, int p_pattern, int p_measure);
	void song_pattern_set_bars(Song *p_song, int p_pattern, int p_bars);

	void song_order_set(Song *p_song,int p_order, int p_pattern);
	void song_track_add(Song *p_song,Track *p_track,int p_pos=-1);
	void song_track_remove(Song *p_song,int p_pos);

	/* Pattern Track */

	void pattern_track_set_note_columns(PatternTrack *p_pattern_track,int p_columns);
	void pattern_track_set_note(PatternTrack *p_pattern_track,int p_pattern, PatternPos p_pos, PatternNote p_note);

	void pattern_track_set_command_columns(PatternTrack *p_pattern_track,int p_columns);
	void pattern_track_set_command(PatternTrack *p_pattern_track,int p_pattern, PatternPos p_pos, PatternCommand p_command);

	void pattern_track_set_swing(PatternTrack *p_pattern_track,float p_swing);
	void pattern_track_set_swing_step(PatternTrack *p_pattern_track,int p_swing_step);



};

#endif
#endif // EDIT_COMMANDS_H
