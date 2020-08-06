#include "synth_sf2.h"
#include "engine/sound_driver_manager.h"
#include "globals/md5.h"

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
	if (!soundfont) {
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
				tsf_channel_note_off(soundfont, 0, e.note.note);
			} break;
			case MIDIEvent::MIDI_NOTE_ON: {
				tsf_channel_note_on(soundfont, 0, e.note.note, e.note.velocity / 127.0);
			} break;
			case MIDIEvent::MIDI_NOTE_PRESSURE: {
			} break;
			case MIDIEvent::MIDI_CONTROLLER: {
				tsf_channel_midi_control(soundfont, 0, e.control.index, e.control.parameter);
			} break;
			case MIDIEvent::MIDI_PATCH_SELECT: {

			} break;
			case MIDIEvent::MIDI_AFTERTOUCH: {

			} break;
			case MIDIEvent::MIDI_PITCH_BEND: {
				tsf_channel_set_pitchwheel(soundfont, 0, e.pitch_bend.bend);
			} break;
			case MIDIEvent::MIDI_SYSEX: {

			} break;
		}
	}
	//printf("render floats: %i\n", block_size);
	tsf_render_float(soundfont, (float *)p_out, block_size);
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
	if (!soundfont) {
		return;
	}

	//tsf_reset(soundfont);
	tsf_note_off_all(soundfont);
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

String AudioSynthSF2::get_shared_data_key() const {
	return data_key;
}
std::shared_ptr<Vector<uint8_t> > AudioSynthSF2::get_shared_data() const {
	return sf2_data;
}
void AudioSynthSF2::set_shared_data(const std::shared_ptr<Vector<uint8_t> > &p_shared_data) {
	sf2_data = p_shared_data;
	_load_soundfont();
}

/* Load/Save */

JSON::Node AudioSynthSF2::_internal_to_json() const {
	JSON::Node node = JSON::object();
	node.add("preset", get_preset());
	return node;
}
Error AudioSynthSF2::_internal_from_json(const JSON::Node &node) {

	if (node.has("preset")) {
		set_preset(node.get("preset").toInt());
	}
	return OK;
}

int AudioSynthSF2::get_preset_count() const {
	if (!soundfont) {
		return 0;
	}
	return tsf_get_presetcount(soundfont);
}
String AudioSynthSF2::get_preset_name(int p_preset) const {
	if (!soundfont) {
		return String();
	}
	return String(tsf_get_presetname(soundfont, p_preset));
}
int AudioSynthSF2::get_preset() const {
	return preset;
}
void AudioSynthSF2::set_preset(int p_preset) {
	SoundDriverManager::lock_driver();
	tsf_channel_set_presetindex(soundfont, 0, p_preset);
	SoundDriverManager::unlock_driver();
}

void AudioSynthSF2::_update_mixer() {
	if (!soundfont) {
		return;
	}
	tsf_set_output(soundfont, TSF_STEREO_INTERLEAVED, mix_rate);
}

void AudioSynthSF2::_load_soundfont() {

	if (soundfont) {
		SoundDriverManager::lock_driver();
		tsf_close(soundfont);
		soundfont = NULL;
		SoundDriverManager::unlock_driver();
	}

	ERR_FAIL_COND(sf2_data->size() == 0);

	SoundDriverManager::lock_driver();
	soundfont = tsf_load_memory(&(*sf2_data)[0], sf2_data->size());
	_update_mixer();
	SoundDriverManager::unlock_driver();
	data_key = "";

	if (soundfont) {
		MD5_CTX md5;
		MD5Init(&md5);
		MD5Update(&md5, &(*sf2_data)[0], sf2_data->size());
		unsigned char md5c[16]; //128 bit
		MD5Final(md5c, &md5);

		for (int i = 0; i < 16; i++) {
			char s[3] = { 0, 0, 0 };
			static const char hexc[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
			s[0] = hexc[md5c[i] & 0xF];
			s[1] = hexc[md5c[i] >> 4];
			data_key += s;
		}

		printf("SF2 LOAD OK: MD5: %s SIZE: %i\n", data_key.utf8().get_data(), sf2_data->size());

		if (get_preset_count()) {
			set_preset(0);
		}
	}
}

Error AudioSynthSF2::load_soundfount(const String &p_path) {

#ifdef WINDOWS_ENABLED
	FILE *f = _wfopen(p_path.c_str(), L"rb");
#else
	FILE *f = fopen(p_path.utf8().get_data(), "rb");
#endif

	if (!f) {
		return ERR_CANT_OPEN;
	}

	fseek(f, 0, SEEK_END);
	size_t pos = ftell(f);

	sf2_data = std::make_shared<Vector<uint8_t> >();
	sf2_data->resize(pos);

	fseek(f, 0, SEEK_SET);
	fread(&(*sf2_data)[0], pos, 1, f);

	fclose(f);

	_load_soundfont();

	preset = 0;

	return soundfont != NULL ? OK : ERR_CANT_OPEN;
}

AudioSynthSF2::AudioSynthSF2() {
	soundfont = NULL;
	preset = 0;
	mix_rate = 44100;
	block_size = 128;
}

AudioSynthSF2::~AudioSynthSF2() {
	if (soundfont) {
		SoundDriverManager::lock_driver();
		tsf_close(soundfont);
		soundfont = NULL;
		SoundDriverManager::unlock_driver();
	}
}
