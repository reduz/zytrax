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

int SoundDriverManager::buffer_size_frames[SoundDriverManager::BUFFER_SIZE_MAX] = {
	64, 128, 256, 512, 1024, 2048, 4096
};
int SoundDriverManager::mix_frequenzy_hz[SoundDriverManager::MIX_FREQ_MAX] = {
	22050, 44100, 48000, 96000, 192000
};

SoundDriver *SoundDriverManager::sound_drivers[MAX_SOUND_DRIVERS];
int SoundDriverManager::sound_driver_count = 0;
int SoundDriverManager::current_driver = 0;

SoundDriverManager::MixFrequency SoundDriverManager::mixing_hz = SoundDriverManager::MIX_FREQ_48000;
SoundDriverManager::BufferSize SoundDriverManager::buffer_size = SoundDriverManager::BUFFER_SIZE_1024;
SoundDriverManager::BufferSize SoundDriverManager::step_size = SoundDriverManager::BUFFER_SIZE_256;

SoundDriverManager::MixCallback SoundDriverManager::mix_callback = NULL;

int SoundDriverManager::get_current_driver_index() {

	return current_driver;
}

void SoundDriverManager::lock_driver() {

	if (sound_driver_count == 0)
		return;

	ERR_FAIL_INDEX(current_driver, sound_driver_count);

	sound_drivers[current_driver]->lock();
}
void SoundDriverManager::unlock_driver() {

	if (sound_driver_count == 0)
		return;

	ERR_FAIL_INDEX(current_driver, sound_driver_count);

	sound_drivers[current_driver]->unlock();
}

bool SoundDriverManager::is_driver_active() {

	ERR_FAIL_INDEX_V(current_driver, sound_driver_count, false);

	return sound_drivers[current_driver]->is_active();
}

bool SoundDriverManager::init_driver(int p_driver) {

	printf("init driver : %i\n",p_driver);
	if (p_driver == -1) {
		p_driver = current_driver;
	}

	ERR_FAIL_INDEX_V(p_driver, sound_driver_count, true); //init current driver, but current is invalid

	if (current_driver >= 0 && current_driver < sound_driver_count && sound_drivers[current_driver]->is_active()) {
		sound_drivers[current_driver]->finish();
	}


	current_driver = p_driver;

	printf("current is : %s\n",sound_drivers[current_driver]->get_id().utf8().get_data());

	return sound_drivers[current_driver]->init();
}
void SoundDriverManager::finish_driver() {

	ERR_FAIL_INDEX(current_driver, sound_driver_count);

	if (!sound_drivers[current_driver]->is_active())
		return;

	sound_drivers[current_driver]->finish();
}

int SoundDriverManager::get_driver_count() {

	return sound_driver_count;
}
SoundDriver *SoundDriverManager::get_driver(int p_which) {

	if (p_which == -1)
		p_which = current_driver;

	ERR_FAIL_INDEX_V(p_which, sound_driver_count, 0);

	return sound_drivers[p_which];
}

void SoundDriverManager::set_mix_frequency(MixFrequency p_frequency) {

	mixing_hz = p_frequency;
}
SoundDriverManager::MixFrequency SoundDriverManager::get_mix_frequency() {
	return mixing_hz;
}

void SoundDriverManager::set_buffer_size(BufferSize p_size) {

	buffer_size = p_size;
}
SoundDriverManager::BufferSize SoundDriverManager::get_buffer_size() {
	return buffer_size;
}

void SoundDriverManager::set_step_buffer_size(BufferSize p_size) {
	step_size = p_size;
}
SoundDriverManager::BufferSize SoundDriverManager::get_step_buffer_size() {
	return step_size;
}

int SoundDriverManager::get_mix_frequency_hz(MixFrequency p_frequency) {
	return mix_frequenzy_hz[p_frequency];
}
int SoundDriverManager::get_buffer_size_frames(BufferSize p_size) {
	return buffer_size_frames[p_size];
}

void SoundDriverManager::set_mix_callback(MixCallback p_callback) {
	mix_callback = p_callback;
}

void SoundDriverManager::register_driver(SoundDriver *p_driver) {

	ERR_FAIL_COND(sound_driver_count >= MAX_SOUND_DRIVERS);
	sound_drivers[sound_driver_count++] = p_driver;
}
