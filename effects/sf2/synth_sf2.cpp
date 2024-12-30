#include "synth_sf2.h"
#include "engine/sound_driver_manager.h"
#include "globals/md5.h"


std::map<String,AudioSynthSF2::SF2Data> AudioSynthSF2::sf2_map;

int AudioSynthSF2::_get_internal_control_port_count() const {
	return 0;
}
ControlPort *AudioSynthSF2::_get_internal_control_port(int p_index) {
	return NULL;
}

bool AudioSynthSF2::has_secondary_input() const {
	return false;
}
void AudioSynthSF2::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	int midi_event_count;
	float time = float(block_size) / mix_rate;
	const MIDIEventStamped *midi_events = _process_midi_events(p_events, p_event_count, time, midi_event_count);
	if (!soundfont_copy) {
		for (int i = 0; i < block_size; i++) {
			p_out[i] = AudioFrame(0, 0);
		}
		return;
	}
	for (int i = 0; i < midi_event_count; i++) {
		const MIDIEvent &e = midi_events[i].event;
		switch (e.type) {
			case MIDIEvent::SEQ_TEMPO: {

			} break;
			case MIDIEvent::SEQ_SIGNATURE: {

			} break;
			case MIDIEvent::SEQ_BAR: {

			} break;
			case MIDIEvent::SEQ_BEAT: {

			} break;
			case MIDIEvent::SEQ_SCALE: {

			} break;
			case MIDIEvent::STREAM_TAIL: {

			} break;
			case MIDIEvent::MIDI_NOTE_OFF: {
				tsf_channel_note_off(soundfont_copy, 0, e.note.note);
			} break;
			case MIDIEvent::MIDI_NOTE_ON: {
				tsf_channel_note_on(soundfont_copy, 0, e.note.note, e.note.velocity / 127.0);
			} break;
			case MIDIEvent::MIDI_NOTE_PRESSURE: {
			} break;
			case MIDIEvent::MIDI_CONTROLLER: {
				tsf_channel_midi_control(soundfont_copy, 0, e.control.index, e.control.parameter);
			} break;
			case MIDIEvent::MIDI_PATCH_SELECT: {

			} break;
			case MIDIEvent::MIDI_AFTERTOUCH: {

			} break;
			case MIDIEvent::MIDI_PITCH_BEND: {
				tsf_channel_set_pitchwheel(soundfont_copy, 0, e.pitch_bend.bend);
			} break;
			case MIDIEvent::MIDI_SYSEX: {

			} break;
		}
	}
	//printf("render floats: %i\n", block_size);
	tsf_render_float(soundfont_copy, (float *)p_out, block_size);
}
void AudioSynthSF2::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
}
void AudioSynthSF2::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioSynthSF2::set_sampling_rate(int p_hz) {
	mix_rate = p_hz;
	_update_mixer();
}

void AudioSynthSF2::reset() {
	if (!soundfont_copy) {
		return;
	}
	_reset_midi();
	//tsf_reset(soundfont);
	//tsf_note_off_all(soundfont);
}

/*info */
String AudioSynthSF2::get_name() const {
	return "SF2 Player";
}
String AudioSynthSF2::get_unique_id() const {
	return "sf2";
}
String AudioSynthSF2::get_provider_id() const {
	return "internal";
}

JSON::Node AudioSynthSF2::_internal_to_json() const {
	JSON::Node node = JSON::object();

	node.add("sf2_path", sf2_path.utf8().get_data());
	if (soundfont_copy) {
		node.add("preset_index",preset);
	}
	return node;
}

Error AudioSynthSF2::_internal_from_json(const JSON::Node &node) {


	if (node.has("sf2_path")) {
		String path;
		path.parse_utf8(node.get("sf2_path").toString().c_str());
		load_soundfount(path);
		if (soundfont_copy && node.has("preset_index")) {
			set_preset(node.get("preset_index").toInt());
		}
	}
	return OK;
}


/* Load/Save */

int AudioSynthSF2::get_preset_count() const {
	if (!soundfont_copy) {
		return 0;
	}
	return tsf_get_presetcount(soundfont_copy);
}
String AudioSynthSF2::get_preset_name(int p_preset) const {
	if (!soundfont_copy) {
		return String();
	}
	return String(tsf_get_presetname(soundfont_copy, p_preset));
}
int AudioSynthSF2::get_preset() const {
	return preset;
}
void AudioSynthSF2::set_preset(int p_preset) {
	SoundDriverManager::lock_driver();
	tsf_channel_set_presetindex(soundfont_copy, 0, p_preset);
	SoundDriverManager::unlock_driver();
	preset = p_preset;
	printf("set preset to %i\n",p_preset);
}

void AudioSynthSF2::_update_mixer() {
	if (!soundfont_copy) {
		return;
	}
	tsf_set_output(soundfont_copy, TSF_STEREO_INTERLEAVED, mix_rate);
}


Error AudioSynthSF2::load_soundfount(const String &p_path) {

	if (sf2_path != String()) {
		_unload_sf2();
		sf2_path = String();
	}

	sf2_path = p_path;

	if (sf2_path != String()) {
		_load_sf2();
	}

	return soundfont_copy !=nullptr ? OK : ERR_CANT_OPEN;
}

void AudioSynthSF2::_unload_sf2() {

	if (soundfont_copy) {

		tsf_close(soundfont_copy);
		soundfont_copy = NULL;
	}

	std::map<String,SF2Data>::iterator I = sf2_map.find(sf2_path);
	if (I != sf2_map.end()) {
		sf2_map[sf2_path].users--;
		if (sf2_map[sf2_path].users==0) {
			tsf_close(sf2_map[sf2_path].soundfont);
			sf2_map.erase(I);
		}
	}

	sf2_path = String();
}

bool AudioSynthSF2::_load_sf2() {

	ERR_FAIL_COND_V(soundfont_copy != nullptr, false);
	if (sf2_map.find(sf2_path) == sf2_map.end()) {

#ifdef WINDOWS_ENABLED
		FILE *f = _wfopen(sf2_path.c_str(), L"rb");
#else
		FILE *f = fopen(sf2_path.utf8().get_data(), "rb");
#endif

		if (!f) {
			return false;
		}

		fseek(f, 0, SEEK_END);
		size_t pos = ftell(f);

		SF2Data sf2;

		Vector<uint8_t> data;
		data.resize(pos);
		sf2.users=1;

		fseek(f, 0, SEEK_SET);
		fread(&data[0], pos, 1, f);

		fclose(f);

		if (data.size() == 0) {
			return false;
		}

		sf2.soundfont = tsf_load_memory(&data[0], data.size());
		sf2_map[sf2_path] = sf2;

	}

	if (sf2_map[sf2_path].soundfont) {
		SoundDriverManager::lock_driver();
		soundfont_copy = tsf_copy(sf2_map[sf2_path].soundfont);
		_update_mixer();
		SoundDriverManager::unlock_driver();
	}

	return true;
}

AudioSynthSF2::AudioSynthSF2() {
	soundfont_copy = NULL;
	preset = 0;
	mix_rate = 44100;
	block_size = 128;
}

AudioSynthSF2::~AudioSynthSF2() {
	_unload_sf2();
}
