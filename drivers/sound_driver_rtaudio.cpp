#include "sound_driver_rtaudio.h"

void SoundDriverRTAudio::lock() {}

void SoundDriverRTAudio::unlock() {}

const char *SoundDriverRTAudio::get_name() { return "RtAudio"; }

uint16_t SoundDriverRTAudio::get_max_level_l() { return 0; }

uint16_t SoundDriverRTAudio::get_max_level_r() { return 0; }

bool SoundDriverRTAudio::is_active() { return false; }

bool SoundDriverRTAudio::init(int p_mix_rate, int p_buffer_size) {

  return true;
}

void SoundDriverRTAudio::finish() {}

int SoundDriverRTAudio::get_mix_rate() const { return 44100; }

SoundDriverRTAudio::SoundDriverRTAudio() {}
