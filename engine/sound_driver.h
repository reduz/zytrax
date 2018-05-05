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

#include "globals/config.h"

/**
	@author Juan Linietsky <reduzio@gmail.com>
*/


class SoundDriver {
public:
	
	virtual void lock()=0; ///< Lock called from UI,game,etc (non-audio) thread, to access audio variables
	virtual void unlock()=0; ///< UnLock called from UI,game,etc (non-audio) thread, to access audio variables
	
	virtual const char * get_name()=0;
	
	virtual Uint16 get_max_level_l()=0; //max level, range 0 - 1024
	virtual Uint16 get_max_level_r()=0; //max level, range 0 - 1024
	
	virtual bool is_active()=0;
	virtual bool init()=0;
	virtual void finish()=0;

	SoundDriver();
	virtual ~SoundDriver();

};

#endif
