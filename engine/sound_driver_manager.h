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

#include "dsp/frame.h"
#include "engine/sound_driver.h"
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/
class SoundDriverManager {

	enum {
		MAX_SOUND_DRIVERS = 64
	};

public:
	enum BufferSize {
		BUFFER_SIZE_64,
		BUFFER_SIZE_128,
		BUFFER_SIZE_256,
		BUFFER_SIZE_512,
		BUFFER_SIZE_1024,
		BUFFER_SIZE_2048,
		BUFFER_SIZE_4096,
		BUFFER_SIZE_MAX
	};

	enum MixFrequency {
		MIX_FREQ_22050,
		MIX_FREQ_44100,
		MIX_FREQ_48000,
		MIX_FREQ_96000,
		MIX_FREQ_192000,
		MIX_FREQ_MAX
	};

	typedef void (*MixCallback)(AudioFrame *p_buffer, int p_frames);

private:
	static int buffer_size_frames[BUFFER_SIZE_MAX];
	static int mix_frequenzy_hz[MIX_FREQ_MAX];

	static SoundDriver *sound_drivers[MAX_SOUND_DRIVERS];
	static int sound_driver_count;
	static int current_driver;
	static MixFrequency mixing_hz;
	static BufferSize buffer_size;
	static BufferSize step_size;
	friend class SoundDriver;

	static MixCallback mix_callback;

public:
	static void lock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads
	static void unlock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads

	static bool init_driver(int p_driver = -1); ///< -1 is current
	static void finish_driver();
	static bool is_driver_active();
	static int get_driver_count();
	static SoundDriver *get_driver(int p_which = -1); ///< -1 is current

	static int get_current_driver_index();

	static void set_mix_frequency(MixFrequency p_frequency);
	static MixFrequency get_mix_frequency();

	static void set_buffer_size(BufferSize p_size);
	static BufferSize get_buffer_size();

	static void set_step_buffer_size(BufferSize p_size);
	static BufferSize get_step_buffer_size();

	static void register_driver(SoundDriver *p_driver);

	static int get_mix_frequency_hz(MixFrequency p_frequency);
	static int get_buffer_size_frames(BufferSize p_size);

	static void set_mix_callback(MixCallback p_callback);
};

#endif
