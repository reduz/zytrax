#ifndef SOUND_DRIVER_RTAUDIO_H
#define SOUND_DRIVER_RTAUDIO_H

#include "drivers/RtAudio.h"
#include "engine/sound_driver.h"

class SoundDriverRTAudio : public SoundDriver {
public:
  virtual void lock();
  virtual void unlock();

  virtual const char *get_name();

  virtual uint16_t get_max_level_l();
  virtual uint16_t get_max_level_r();

  virtual bool is_active();
  virtual bool init(int p_mix_rate, int p_buffer_size);
  virtual void finish();

  virtual int get_mix_rate() const;

  SoundDriverRTAudio();
};

#endif // SOUND_DRIVER_RTAUDIO_H
