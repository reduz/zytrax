//
// C++ Interface: sound_driver_manager
//
// Description: 
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SOUND_DRIVER_MANAGER_H
#define SOUND_DRIVER_MANAGER_H

#include "engine/sound_driver.h"
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/
class SoundDriverManager {

	enum {
		MAX_SOUND_DRIVERS=5
	};
	
	static SoundDriver *sound_drivers[MAX_SOUND_DRIVERS];
	static int sound_driver_count;
	static int current_driver;


public:
	
	static void lock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads
	static void unlock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads
	
	static bool init_driver(int p_driver=-1); ///< -1 is current
	static void finish_driver();
	static bool is_driver_active();
	static int get_driver_count();
	static SoundDriver *get_driver(int p_which=-1); ///< -1 is current
	
	static int get_current_driver_index();
	
	
	static void register_driver(SoundDriver *p_driver);
	

};

#endif
