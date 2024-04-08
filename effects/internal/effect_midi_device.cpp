#include "effect_midi_device.h"

#include "dsp/db.h"
#include <math.h>


void AudioEffectMIDIDevice::set_port_hash(uint32_t p_hash) {
	port_hash=p_hash;	
	_reset_midi();
}

uint32_t AudioEffectMIDIDevice::get_port_hash() const {
	return port_hash;
}


void AudioEffectMIDIDevice::set_device_layout_name(String p_name) {
	device_layout_name=p_name;
	_reset_midi();
}

String AudioEffectMIDIDevice::get_device_layout_name() const {
	return device_layout_name;
}

void AudioEffectMIDIDevice::set_bank_name(String p_name) {
	bank_name=p_name;
	_reset_midi();
}

String AudioEffectMIDIDevice::get_bank_name() const {
	return bank_name;
}


void AudioEffectMIDIDevice::set_patch_name(String p_name) {
	patch_name=p_name;
	_reset_midi();
}

String AudioEffectMIDIDevice::get_patch_name() const {
	return patch_name;
}


void AudioEffectMIDIDevice::set_bank_msb(int p_index) {
	bank_msb=p_index;
	_reset_midi();
}

int AudioEffectMIDIDevice::get_bank_msb() const {
	return bank_msb;
}


void AudioEffectMIDIDevice::set_bank_lsb(int p_index) {
	bank_lsb=p_index;
	_reset_midi();
}

int AudioEffectMIDIDevice::get_bank_lsb() const {
	return bank_lsb;
}


void AudioEffectMIDIDevice::set_patch_index(int p_index) {
	patch=p_index;
	_reset_midi();
}

int AudioEffectMIDIDevice::get_patch_index() const {
	return patch;
}



int AudioEffectMIDIDevice::_get_internal_control_port_count() const {

	return 0;
}
ControlPort *AudioEffectMIDIDevice::_get_internal_control_port(int p_index) {

	return nullptr;
}


JSON::Node AudioEffectMIDIDevice::_internal_to_json() const {

	JSON::Node node = JSON::object();
	node.add("bank_lsb",bank_lsb);
	node.add("bank_msb",bank_msb);
	node.add("patch",patch);
	node.add("port_hash",int32_t(port_hash));
	node.add("device_layout_name",device_layout_name.utf8().get_data());
	node.add("bank_name",bank_name.utf8().get_data());
	node.add("patch_name",patch_name.utf8().get_data());
	return node;
}

Error AudioEffectMIDIDevice::_internal_from_json(const JSON::Node &node) {

	if (node.has("bank_lsb")) {
		bank_lsb = node.get("bank_lsb").toInt();
	}
	if (node.has("bank_msb")) {
		bank_msb = node.get("bank_msb").toInt();
	}
	if (node.has("patch")) {
		patch = node.get("patch").toInt();
	}
	if (node.has("port_hash")) {
		port_hash = int32_t(node.get("port_hash").toInt());
	}
	if (node.has("device_layout_name")) {
		device_layout_name = node.get("device_layout_name").toString().c_str();
	}
	if (node.has("bank_name")) {
		bank_name = node.get("bank_name").toString().c_str();
	}
	if (node.has("patch_name")) {
		patch_name = node.get("patch_name").toString().c_str();
	}

	return OK;
}

//process
bool AudioEffectMIDIDevice::has_secondary_input() const {
	return false;
}

void AudioEffectMIDIDevice::_get_bank_and_patch(int &r_bank_lsb, int &r_bank_msb,int &r_patch) {

	r_bank_lsb = bank_lsb;
	r_bank_msb = bank_msb;
	r_patch = patch;
}

void AudioEffectMIDIDevice::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {
	// Pass audio by.
	for(int i=0;i<block_size;i++) {
		p_out[i]=p_in[i];
	}

	float time = float(block_size) / sampling_rate;
	int stamped_count;
	MIDIEventStamped * events = _process_midi_events(p_events, p_event_count, time, stamped_count);

	if (dispatch_callback) {
		MIDIEventRouted event;
		for(int i=0;i<stamped_count;i++) {
			event.event = events[i].event;
			event.frame = events[i].frame;			
			event.port_hash = port_hash;
			//printf("-> frame: %i - port %i - ev %s\n",events[i].frame,port_hash,String(events[i].event).utf8().get_data());
			dispatch_callback(event,dispatch_userdata);
		}
	}
}

void AudioEffectMIDIDevice::mute() {
	if (!dispatch_callback) {
		return;
	}


	MIDIEventRouted event;

	event.event.type = MIDIEvent::MIDI_CONTROLLER;
	event.event.control.index = 123; // All notes off
	event.event.control.parameter = 0;
	event.event.channel = get_midi_channel();
	event.frame = 0;
	event.port_hash = get_port_hash();
	dispatch_callback(event,dispatch_userdata);

	event.event.control.index = 123; // All notes off
	dispatch_callback(event,dispatch_userdata);

	event.event.control.index = 120; // All sound off
	dispatch_callback(event,dispatch_userdata);

}

void AudioEffectMIDIDevice::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {

}


void AudioEffectMIDIDevice::set_routed_midi_event_dispatch_callback(MidiEventRoutedDispatchCallback p_callback, void *p_userdata) {
	dispatch_callback = p_callback;
	dispatch_userdata= p_userdata;
}

void AudioEffectMIDIDevice::set_process_block_size(int p_size) {
	block_size = p_size;
}
void AudioEffectMIDIDevice::set_sampling_rate(int p_hz) {
	sampling_rate = p_hz;
}
//info
String AudioEffectMIDIDevice::get_name() const {
	return "MIDI Device";
}
String AudioEffectMIDIDevice::get_unique_id() const {
	return "midi_device";
}
String AudioEffectMIDIDevice::get_provider_id() const {
	return "internal";
}


void AudioEffectMIDIDevice::reset() {
	_reset_midi();
}

AudioEffectMIDIDevice::AudioEffectMIDIDevice() {


}
AudioEffectMIDIDevice::~AudioEffectMIDIDevice() {
}
