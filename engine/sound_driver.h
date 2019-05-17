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
#include "rstring.h"
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/

class SoundDriver {
protected:
	void mix(AudioFrame *p_buffer, int p_frames);

public:
	virtual void lock() = 0; ///< Lock called from UI,game,etc (non-audio) thread, to access audio variables
	virtual void unlock() = 0; ///< UnLock called from UI,game,etc (non-audio) thread, to access audio variables

	virtual String get_name() const = 0;
	virtual String get_id() const = 0;

	virtual float get_max_level_l() = 0;
	virtual float get_max_level_r() = 0;

	virtual bool is_active() = 0;
	virtual bool init() = 0;
	virtual void finish() = 0;

	virtual int get_mix_rate() const = 0;

	SoundDriver();
	virtual ~SoundDriver();
};

#endif
