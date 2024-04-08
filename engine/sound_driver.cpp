//
// C++ Implementation: sound_driver
//
// Description:
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "sound_driver.h"
#include "sound_driver_manager.h"

void SoundDriver::mix(AudioFrame *p_buffer, int p_frames, MIDIEventRouted *p_event_buffer, int p_event_buffer_max_size,int &r_events_written) {
	if (SoundDriverManager::mix_callback) {
	SoundDriverManager::mix_callback(p_buffer, p_frames, p_event_buffer, p_event_buffer_max_size, r_events_written);
	}
}

SoundDriver::SoundDriver() {
}

SoundDriver::~SoundDriver() {
}
