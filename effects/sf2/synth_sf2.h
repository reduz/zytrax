#ifndef EFFECT_SYNTH_SF2_H
#define EFFECT_SYNTH_SF2_H

#include "engine/audio_effect_midi.h"
#include "globals/map.h"
#include "globals/vector.h"
#include "tsf.h"

class AudioSynthSF2 : public AudioEffectMIDI {
private:
	std::shared_ptr<Vector<uint8_t> > sf2_data;
	String data_key;
	tsf *soundfont;
	int preset;
	int mix_rate;
	int block_size;

	void _update_mixer();
	void _load_soundfont();

public:
	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);
	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);

	virtual void reset();

	virtual String get_shared_data_key() const;
	virtual std::shared_ptr<Vector<uint8_t> > get_shared_data() const;
	virtual void set_shared_data(const std::shared_ptr<Vector<uint8_t> > &p_shared_data);

	/*info */
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	/* Load/Save */

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	int get_preset_count() const;
	String get_preset_name(int p_preset) const;
	int get_preset() const;
	void set_preset(int p_preset);

	Error load_soundfount(const String &p_path);

	AudioSynthSF2();
	~AudioSynthSF2();
};

#endif // EFFECT_SYNTH_SF2_H
