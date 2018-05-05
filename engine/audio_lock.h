#ifndef AUDIO_LOCK_H
#define AUDIO_LOCK_H

class AudioLock {
public:
	static void lock();
	static void unlock();

	AudioLock() { lock(); }
	~AudioLock() { unlock(); }
};


#define _AUDIO_LOCK_ AudioLock __audio_lock__;

#endif // AUDIO_LOCK_H
