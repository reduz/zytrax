#ifndef EFFECT_MIDI_H
#define EFFECT_MIDI_H

#include "engine/audio_effect_midi.h"


class AudioEffectMIDIDevice : public AudioEffectMIDI {

	uint32_t port_hash;
	String device_layout_name;

	int block_size = 128;
	int sampling_rate = 4096;

	int bank_lsb =0;
	int bank_msb =0;
	int patch = 0;

	String bank_name;
	String patch_name;

	MidiEventRoutedDispatchCallback dispatch_callback = nullptr;
	void *dispatch_userdata = nullptr;
protected:


	virtual void _get_bank_and_patch(int &r_bank_lsb, int &r_bank_msb,int &r_patch);

	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

public:

	void set_port_hash(uint32_t p_hash);
	uint32_t get_port_hash() const;

	void set_device_layout_name(String p_name);
	String get_device_layout_name() const;

	void set_bank_name(String p_name);
	String get_bank_name() const;

	void set_patch_name(String p_name);
	String get_patch_name() const;

	void set_bank_msb(int p_index);
	int get_bank_msb() const;

	void set_bank_lsb(int p_index);
	int get_bank_lsb() const;

	void set_patch_index(int p_index);
	int get_patch_index() const;

	virtual void mute();


	//process
	virtual bool has_secondary_input() const;
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active);
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active);

	virtual void set_process_block_size(int p_size);
	virtual void set_sampling_rate(int p_hz);

	virtual void set_routed_midi_event_dispatch_callback(MidiEventRoutedDispatchCallback p_callback,void *p_userdata);

	//info
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	virtual void reset();

	AudioEffectMIDIDevice();
	~AudioEffectMIDIDevice();
};

#endif // EFFECT_MIDI_H
