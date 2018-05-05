//
// C++ Implementation: sound_driver_manager
//
// Description: 
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "sound_driver_manager.h"
#include "error_macros.h"

SoundDriver *SoundDriverManager::sound_drivers[MAX_SOUND_DRIVERS]={0,0,0,0,0};
int SoundDriverManager::sound_driver_count=0;
int SoundDriverManager::current_driver=0;

int SoundDriverManager::get_current_driver_index() {
	
	return current_driver;
}

void SoundDriverManager::lock_driver() {
	
	if (sound_driver_count==0)
		return;
		
	ERR_FAIL_INDEX(current_driver,sound_driver_count);	
	
	sound_drivers[current_driver]->lock();
	
}
void SoundDriverManager::unlock_driver() {
	
	if (sound_driver_count==0)
		return;
	
	ERR_FAIL_INDEX(current_driver,sound_driver_count);	
	
	sound_drivers[current_driver]->unlock();
	
}

bool SoundDriverManager::is_driver_active() {
	
	ERR_FAIL_INDEX_V(current_driver,sound_driver_count,false);	
	
	return sound_drivers[current_driver]->is_active();
	
}

bool SoundDriverManager::init_driver(int p_driver){
	
	if (p_driver==-1)
		p_driver=current_driver;
	
		
	ERR_FAIL_INDEX_V(p_driver,sound_driver_count,true); //init current driver, but current is invalid
	
	if (current_driver>=0 && current_driver<sound_driver_count && sound_drivers[current_driver]->is_active())
		sound_drivers[current_driver]->finish();
	
	current_driver=p_driver;
	
	return sound_drivers[current_driver]->init();

	
}
void SoundDriverManager::finish_driver(){
	
	ERR_FAIL_INDEX(current_driver,sound_driver_count); 
	
	if (!sound_drivers[current_driver]->is_active())
		return;
	
	sound_drivers[current_driver]->finish();
	
}

int SoundDriverManager::get_driver_count(){
	
	return sound_driver_count;
	
}
SoundDriver *SoundDriverManager::get_driver(int p_which){
	
	if (p_which==-1)
		p_which=current_driver;
	
	ERR_FAIL_INDEX_V(p_which,sound_driver_count,0); 
	
	return sound_drivers[p_which];
	
}


void SoundDriverManager::register_driver(SoundDriver *p_driver){
	
	ERR_FAIL_COND(sound_driver_count>=MAX_SOUND_DRIVERS);
	sound_drivers[ sound_driver_count++ ] = p_driver;
	
}

