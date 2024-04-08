//
// C++ Interface: sound_driver
//
// Description:
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SOUND_DRIVER_H
#define SOUND_DRIVER_H

#include "dsp/frame.h"
#include "globals/config.h"
#include "dsp/midi_event.h"
#include "globals/rstring.h"
#include "globals/vector.h"
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/

class SoundDriver {
protected:

    void mix(AudioFrame *p_buffer, int p_frames, MIDIEventRouted *p_event_buffer, int p_event_buffer_max_size, int &r_events_written);

public:	
	virtual void lock() = 0; ///< Lock called from UI,game,etc (non-audio) thread, to access audio variables
	virtual void unlock() = 0; ///< UnLock called from UI,game,etc (non-audio) thread, to access audio variables

	virtual String get_name() const = 0;
	virtual String get_id() const = 0;

	virtual float get_max_level_l() = 0;
	virtual float get_max_level_r() = 0;

	struct MidiDeviceInfo {
		String name;
		uint32_t hash;
	};

	virtual Vector<MidiDeviceInfo> get_midi_devices() const = 0;
	virtual uint32_t get_midi_devices_hash() const =0;
	virtual bool is_active() = 0;
	virtual bool init() = 0;
	virtual void finish() = 0;

	virtual int get_mix_rate() const = 0;

	SoundDriver();
	virtual ~SoundDriver();
};

#endif
