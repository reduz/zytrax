
#include "sound_driver_rtaudio.h"
#include "engine/sound_driver_manager.h"
#include "globals/error_macros.h"
#include "globals/vector.h"
#include <mutex>
//
#include "drivers/rtaudio/rtaudio/RtAudio.h"

Vector<RtAudio *> rt_audios;

class SoundDriverRTAudio : public SoundDriver {
public:
	RtAudio *rt_audio;
	std::recursive_mutex mutex;
	int device;
	int mix_rate;
	RtAudio::DeviceInfo info;
	RtAudio::StreamParameters parameters;
	unsigned int buffer_frames;
	int step_frames;

	bool active;

	virtual String get_id() const {
		String name = RtAudio::getApiName(rt_audio->getCurrentApi()).c_str();
		name += "::";
		name += info.name.c_str();
		return name;
	}

	static int rt_audio_callbacks(void *outputBuffer, void *inputBuffer,
				      unsigned int nFrames,
				      double streamTime,
				      RtAudioStreamStatus status,
				      void *userData) {

		SoundDriverRTAudio *driver = (SoundDriverRTAudio *)userData;
		return driver->rt_audio_callback(outputBuffer, inputBuffer, nFrames, streamTime, status);
	}

	int rt_audio_callback(void *outputBuffer, void *inputBuffer,
			      unsigned int nFrames,
			      double streamTime,
			      RtAudioStreamStatus status) {

		if (mutex.try_lock()) {
			int evw;
			mix((AudioFrame *)outputBuffer, nFrames,nullptr,0,evw);
			mutex.unlock();
		}
		return 0;
	}

	virtual Vector<MidiDeviceInfo> get_midi_devices() const {

		return Vector<MidiDeviceInfo>();
	}

	virtual uint32_t get_midi_devices_hash() const {
		return 0;
	}


	virtual void lock() {
		mutex.lock();
	}
	virtual void unlock() {
		mutex.unlock();
	}

	virtual String get_name() const {
		String name = RtAudio::getApiDisplayName(rt_audio->getCurrentApi()).c_str();
		name += ": ";
		name += info.name.c_str();
		return name;
	}

	virtual float get_max_level_l() {
		return 0;
	}
	virtual float get_max_level_r() {
		return 0;
	}

	virtual bool is_active() {
		return active;
	}
	virtual bool init() {

		ERR_FAIL_COND_V(active, false);

		active = false;
		parameters.deviceId = device;
		parameters.nChannels = 2;
		parameters.firstChannel = 0;

		buffer_frames = SoundDriverManager::get_buffer_size_frames(SoundDriverManager::get_buffer_size());
		step_frames = SoundDriverManager::SoundDriverManager::get_buffer_size_frames(SoundDriverManager::get_step_buffer_size());
		mix_rate = SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::get_mix_frequency());

		try {
			rt_audio->openStream(&parameters, NULL, RTAUDIO_FLOAT32, mix_rate, &buffer_frames,
					     rt_audio_callbacks, this);
		} catch (RtAudioError &error) {
			error.printMessage();
			return false;
		}

		active = true;

		rt_audio->startStream();

		return true;
	}
	virtual void finish() {
		ERR_FAIL_COND(!active);
		active = false;
		rt_audio->stopStream();
		rt_audio->closeStream();
	}

	virtual int get_mix_rate() const {
		return mix_rate;
	}

	virtual int get_buffer_size() const {
		return buffer_frames;
	}

	virtual int get_step_size() const {
		return step_frames;
	}

	SoundDriverRTAudio() {
		active = false;
		device = 0;
		mix_rate = 44100;
	}
	~SoundDriverRTAudio() {}
};

static Vector<SoundDriverRTAudio *> drivers;

void register_rtaudio_driver() {

	std::vector<RtAudio::Api> apis;

	RtAudio::getCompiledApi(apis);

	for (int i = 0; i < apis.size(); i++) {

		RtAudio *rt_audio;

		try {
			rt_audio = new RtAudio(apis[i]);
		} catch (RtAudioError &error) {
			error.printMessage();
			continue;
		}

		rt_audios.push_back(rt_audio);

		// Determine the number of devices available
		int devices = rt_audio->getDeviceCount();
		// Scan through devices for various capabilities

		for (int j = 0; j < devices; j++) {

			SoundDriverRTAudio *driver = new SoundDriverRTAudio;
			try {
				driver->info = rt_audio->getDeviceInfo(j);
				if (driver->info.probed) {
					driver->device = j;
					driver->rt_audio = rt_audio;
					drivers.push_back(driver);
					SoundDriverManager::register_driver(driver);
					continue;
				}
			} catch (RtAudioError &error) {
				error.printMessage();
			}
			delete driver; //not worked
		}
	}
}

void cleanup_rtaudio_driver() {
	for (int i = 0; i < drivers.size(); i++) {
		delete drivers[i];
	}
	for (int i = 0; i < rt_audios.size(); i++) {
		delete rt_audios[i];
	}
}
