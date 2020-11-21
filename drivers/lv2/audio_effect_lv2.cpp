#include "audio_effect_lv2.h"
#include "audio_effect_provider_lv2.h"
#include "engine/sound_driver_manager.h"

LV2_URID AudioEffectLV2::uri_to_idm(const char *uri) {
	//this whole API is ***** anyway, so a simple spinlock will do
	LV2_URID ret;
	while (locked.test_and_set(std::memory_order_acquire)) {
	}
	CStringKey urics;
	urics.cstring = uri;
	const Map<CStringKey, LV2_URID>::Element *E = uri_id_map.find(urics);
	if (E) {
		ret = E->get();
	} else {
		ret = uri_id_map.size();
		uri_id_map[urics] = ret;
		id_uri_map[ret] = urics;
	}
	locked.clear(std::memory_order_release);
	return ret;
}

const char *AudioEffectLV2::id_to_urim(LV2_URID id) {

	const char *ret = NULL;
	while (locked.test_and_set(std::memory_order_acquire)) {
	}
	CStringKey urics;
	const Map<LV2_URID, CStringKey>::Element *E = id_uri_map.find(id);
	if (E) {
		ret = E->get().cstring;
	} else {
		ERR_PRINT("wtf requested an uri for an existing ID?");
	}
	locked.clear(std::memory_order_release);
	return ret;
}

LV2_URID AudioEffectLV2::map_uri(LV2_URID_Unmap_Handle handle,
		const char *uri) {
	AudioEffectLV2 *self = (AudioEffectLV2 *)handle;
	return self->uri_to_idm(uri);
}

const char *AudioEffectLV2::unmap_uri(LV2_URID_Unmap_Handle handle,
		LV2_URID urid) {
	AudioEffectLV2 *self = (AudioEffectLV2 *)handle;
	return self->id_to_urim(urid);
}

uint32_t AudioEffectLV2::uri_to_id(LV2_URI_Map_Callback_Data callback_data,
		const char *map,
		const char *uri) {

	AudioEffectLV2 *self = (AudioEffectLV2 *)callback_data;
	return self->uri_to_idm(uri);
}

bool AudioEffectLV2::has_secondary_input() const {
	return in_buffers.size() == 4;
}

void AudioEffectLV2::_process_events(const Event *p_events, int p_event_count) {
	if (!evbuf) {
		return;
	}
	lv2_evbuf_reset(evbuf, true);

	float time = float(buffer_size) / sampling_rate;
	int midi_event_count;
	LV2_Evbuf_Iterator iter = lv2_evbuf_begin(evbuf);
	const MIDIEventStamped *midi_events = _process_midi_events(p_events, p_event_count, time, midi_event_count);
	for (int i = 0; i < midi_event_count; i++) {
		uint8_t buf[3];
		if (midi_events[i].event.write(buf)) {
			lv2_evbuf_write(&iter, midi_events[i].frame, 0, midi_event_id, 3, buf);
		}
	}
}

void AudioEffectLV2::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	//mono
	if (in_buffers.size() == 1) {
		float *in = in_buffer_ptrs[0];
		for (int i = 0; i < buffer_size; i++) {
			in[i] = (p_in[i].l + p_in[i].r) * 0.5;
		}

	} else if (in_buffers.size() >= 2) {
		float *in_l = in_buffer_ptrs[0];
		float *in_r = in_buffer_ptrs[1];
		for (int i = 0; i < buffer_size; i++) {
			in_l[i] = p_in[i].l;
			in_r[i] = p_in[i].r;
		}
	}

	_process_events(p_events, p_event_count);
	lilv_instance_run(instance, buffer_size);
	if (instance2) {
		lilv_instance_run(instance2, buffer_size);
	}

	//mono
	if (out_buffers.size() == 1) {
		float *out = out_buffer_ptrs[0];
		for (int i = 0; i < buffer_size; i++) {
			p_out[i].l = out[i];
			p_out[i].r = out[i];
		}

	} else {
		float *out_l = out_buffer_ptrs[0];
		float *out_r = out_buffer_ptrs[1];
		for (int i = 0; i < buffer_size; i++) {
			p_out[i].l = out_l[i];
			p_out[i].r = out_r[i];
		}
	}
}
void AudioEffectLV2::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
	//mono
	if (in_buffers.size() == 2) {
		float *in = in_buffer_ptrs[0];
		float *in2 = in_buffer_ptrs[1];
		for (int i = 0; i < buffer_size; i++) {
			in[i] = (p_in[i].l + p_in[i].r) * 0.5;
			in2[i] = (p_secondary[i].l + p_secondary[i].r) * 0.5;
		}

	} else if (in_buffers.size() >= 4) {
		float *in_l = in_buffer_ptrs[0];
		float *in_r = in_buffer_ptrs[1];
		float *in2_l = in_buffer_ptrs[2];
		float *in2_r = in_buffer_ptrs[3];
		for (int i = 0; i < buffer_size; i++) {
			in_l[i] = p_in[i].l;
			in_r[i] = p_in[i].r;
			in2_l[i] = p_secondary[i].l;
			in2_r[i] = p_secondary[i].r;
		}
	}

	_process_events(p_events, p_event_count);

	lilv_instance_run(instance, buffer_size);
	if (instance2) {
		lilv_instance_run(instance2, buffer_size);
	}

	//mono
	if (out_buffers.size() == 1) {
		float *out = out_buffer_ptrs[0];
		for (int i = 0; i < buffer_size; i++) {
			p_out[i].l = out[i];
			p_out[i].r = out[i];
		}

	} else {
		float *out_l = out_buffer_ptrs[0];
		float *out_r = out_buffer_ptrs[1];
		for (int i = 0; i < buffer_size; i++) {
			p_out[i].l = out_l[i];
			p_out[i].r = out_r[i];
		}
	}
}

void AudioEffectLV2::set_process_block_size(int p_size) {
	if (buffer_size == p_size) {
		return;
	}
	buffer_size = p_size;
	_update_buffers_and_ports();
}
void AudioEffectLV2::set_sampling_rate(int p_hz) {
	if (sampling_rate == p_hz) {
		return;
	}
}
//info
String AudioEffectLV2::get_name() const {
	return name;
}
String AudioEffectLV2::get_unique_id() const {
	return identifier;
}
String AudioEffectLV2::get_provider_id() const {
	return "LV2";
}

int AudioEffectLV2::_get_internal_control_port_count() const {
	return control_ports.size();
}
ControlPort *AudioEffectLV2::_get_internal_control_port(int p_port) {
	return control_ports[p_port];
}

void AudioEffectLV2::reset() {
}

/* Load/Save */

JSON::Node AudioEffectLV2::_internal_to_json() const {
	JSON::Node n = JSON::object();
	return n;
}
Error AudioEffectLV2::_internal_from_json(const JSON::Node &node) {
	return OK;
}

// Not gonna support all this from the go, but let's lie to the plugin

Error AudioEffectLV2::setup(const LilvPlugin *p_plugin, bool p_has_ui) {
	plugin = p_plugin;
	has_ui = p_has_ui;
	sampling_rate = SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::get_mix_frequency());
	_plugin_init();
	return OK;
}

void AudioEffectLV2::_plugin_init() {

	//lets see if this is useful
	int ports = lilv_plugin_get_num_ports(plugin);

	int input_count = 0;
	int output_count = 0;
	int event_input = 0;
	int event_output = 0;

	const AudioEffectProviderLV2::URIS &uris = AudioEffectProviderLV2::get_uris();

	for (int j = 0; j < ports; j++) {

		const LilvPort *port = lilv_plugin_get_port_by_index(plugin, j);
		if (lilv_port_is_a(plugin, port, uris.audio_port)) {
			if (lilv_port_is_a(plugin, port, uris.output_port)) {
				output_count++;
			} else if (lilv_port_is_a(plugin, port, uris.input_port)) {
				input_count++;
			}
		}
		if ((lilv_port_is_a(plugin, port, uris.event_port) || lilv_port_is_a(plugin, port, uris.atom_port)) && lilv_port_is_a(plugin, port, uris.input_port)) {

			event_input++;
		}
		if ((lilv_port_is_a(plugin, port, uris.event_port) || lilv_port_is_a(plugin, port, uris.atom_port)) && lilv_port_is_a(plugin, port, uris.output_port)) {

			event_output++;
		}
	}

	bool double_instance = false;

	if (!has_ui) {

		if (event_input == 1) {
			if (input_count == 0 && output_count == 1) {

				double_instance = true;
			}
		} else {

			if (input_count == 2 && output_count == 1) {
				//sidechain mono

				double_instance = true;
			}

			if (input_count == 1 && output_count == 1) {
				//mono

				double_instance = true;
			}
		}
	}

	{

		urids.atom_Float = uri_to_idm(LV2_ATOM__Float);
		urids.atom_Int = uri_to_idm(LV2_ATOM__Int);
		urids.atom_Object = uri_to_idm(LV2_ATOM__Object);
		urids.atom_Path = uri_to_idm(LV2_ATOM__Path);
		urids.atom_String = uri_to_idm(LV2_ATOM__String);
		urids.atom_eventTransfer = uri_to_idm(LV2_ATOM__eventTransfer);
		urids.bufsz_maxBlockLength = uri_to_idm(LV2_BUF_SIZE__maxBlockLength);
		urids.bufsz_minBlockLength = uri_to_idm(LV2_BUF_SIZE__minBlockLength);
		urids.bufsz_sequenceSize = uri_to_idm(LV2_BUF_SIZE__sequenceSize);
		urids.log_Error = uri_to_idm(LV2_LOG__Error);
		urids.log_Trace = uri_to_idm(LV2_LOG__Trace);
		urids.log_Warning = uri_to_idm(LV2_LOG__Warning);
		urids.midi_MidiEvent = uri_to_idm(LV2_MIDI__MidiEvent);
		urids.param_sampleRate = uri_to_idm(LV2_PARAMETERS__sampleRate);
		urids.patch_Get = uri_to_idm(LV2_PATCH__Get);
		urids.patch_Put = uri_to_idm(LV2_PATCH__Put);
		urids.patch_Set = uri_to_idm(LV2_PATCH__Set);
		urids.patch_body = uri_to_idm(LV2_PATCH__body);
		urids.patch_property = uri_to_idm(LV2_PATCH__property);
		urids.patch_value = uri_to_idm(LV2_PATCH__value);
		urids.time_Position = uri_to_idm(LV2_TIME__Position);
		urids.time_bar = uri_to_idm(LV2_TIME__bar);
		urids.time_barBeat = uri_to_idm(LV2_TIME__barBeat);
		urids.time_beatUnit = uri_to_idm(LV2_TIME__beatUnit);
		urids.time_beatsPerBar = uri_to_idm(LV2_TIME__beatsPerBar);
		urids.time_beatsPerMinute = uri_to_idm(LV2_TIME__beatsPerMinute);
		urids.time_frame = uri_to_idm(LV2_TIME__frame);
		urids.time_speed = uri_to_idm(LV2_TIME__speed);
		urids.ui_updateRate = uri_to_idm(LV2_UI__updateRate);

		midi_event_id = uri_to_idm(LV2_MIDI__MidiEvent);
	}

	{

		LV2_URI_Map_Feature uri_map = { NULL, &uri_to_id };

		LV2_Extension_Data_Feature ext_data = { NULL };

		LV2_Feature uri_map_feature = { NS_EXT "uri-map", NULL };
		LV2_Feature map_feature = { LV2_URID__map, NULL };
		LV2_Feature unmap_feature = { LV2_URID__unmap, NULL };
		LV2_Feature make_path_feature = { LV2_STATE__makePath, NULL };
		LV2_Feature sched_feature = { LV2_WORKER__schedule, NULL };
		LV2_Feature state_sched_feature = { LV2_WORKER__schedule, NULL };
		LV2_Feature safe_restore_feature = { LV2_STATE__threadSafeRestore, NULL };
		LV2_Feature log_feature = { LV2_LOG__log, NULL };
		LV2_Feature options_feature = { LV2_OPTIONS__options, NULL };
		LV2_Feature def_state_feature = { LV2_STATE__loadDefaultState, NULL };

		/** These features have no data */
		LV2_Feature buf_size_features[3] = {
			{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
			{ LV2_BUF_SIZE__fixedBlockLength, NULL },
			{ LV2_BUF_SIZE__boundedBlockLength, NULL }
		};

		const LV2_Feature *features[] = {
			&uri_map_feature, &map_feature, &unmap_feature,
			//&sched_feature,
			//&log_feature,
			//&options_feature,
			//&def_state_feature,
			//&safe_restore_feature,
			&buf_size_features[0],
			&buf_size_features[1],
			&buf_size_features[2],
			NULL
		};

		uri_map_feature.data = &uri_map;
		uri_map.callback_data = this;

		map.handle = this;
		map.map = map_uri;
		map_feature.data = &map;

		//jalv.worker.jalv       = &jalv;
		//jalv.state_worker.jalv = &jalv;

		unmap.handle = this;
		unmap.unmap = unmap_uri;
		unmap_feature.data = &unmap;

		lv2_atom_forge_init(&forge, &map);

		instance = lilv_plugin_instantiate(plugin, sampling_rate, features);
		if (double_instance) {
			instance2 = lilv_plugin_instantiate(plugin, sampling_rate, features);
		} else {
			instance2 = NULL;
		}
	}

	//ext_data.data_access = lilv_instance_get_descriptor(instance)->extension_data;

	/* Create workers if necessary */
	if (lilv_plugin_has_extension_data(plugin, AudioEffectProviderLV2::get_uris().work_interface)) {
		printf("**STUB Plugin uses Work Interface\n");
#if 0
		const LV2_Worker_Interface *iface = (const LV2_Worker_Interface *)
				lilv_instance_get_extension_data(jalv.instance, LV2_WORKER__interface);

		jalv_worker_init(&jalv, &jalv.worker, iface, true);
		if (jalv.safe_restore) {
			jalv_worker_init(&jalv, &jalv.state_worker, iface, false);
		}
#endif
	}

	int plugin_port_count = lilv_plugin_get_num_ports(plugin);

	for (int i = 0; i < plugin_port_count; i++) {
		const LilvPort *port = lilv_plugin_get_port_by_index(plugin, i);

		if ((lilv_port_is_a(plugin, port, uris.event_port) || lilv_port_is_a(plugin, port, uris.atom_port)) && lilv_port_is_a(plugin, port, uris.input_port)) {
			//input port, create an event buffer and connect it
			bool old_type = lilv_port_is_a(plugin, port, uris.input_port);
			evbuf = lv2_evbuf_new(512 * 1024,
					old_type ? LV2_EVBUF_EVENT : LV2_EVBUF_ATOM,
					uri_to_idm(lilv_node_as_string(uris.atom_chunk)),
					uri_to_idm(lilv_node_as_string(uris.atom_sequence)));

			lilv_instance_connect_port(instance, i, lv2_evbuf_get_buffer(evbuf));
			if (instance2) {
				lilv_instance_connect_port(instance2, i, lv2_evbuf_get_buffer(evbuf));
			}
		} else if (lilv_port_is_a(plugin, port, uris.control_port) && lilv_port_is_a(plugin, port, uris.output_port)) {
			lilv_instance_connect_port(instance, i, &out_control_trash);
			if (instance2) {
				lilv_instance_connect_port(instance2, i, &out_control_trash);
			}
		} else if (lilv_port_is_a(plugin, port, uris.control_port) && lilv_port_is_a(plugin, port, uris.input_port)) {

			LV2ControlPort *cport = NULL;
			String identifier;
			identifier.parse_utf8(lilv_node_as_string(lilv_port_get_symbol(plugin, port)));

			for (int j = 0; j < control_ports.size(); j++) {
				if (control_ports[j]->identifier == identifier) {
					cport = control_ports[j];
					break;
				}
			}
			if (!cport) {
				cport = new LV2ControlPort;
			}

			if (lilv_port_has_property(plugin, port, uris.lv2_sampleRate)) {
				cport->multiplier = 44100;
				cport->needs_sr_multiplier = true;
			} else {
				cport->multiplier = 1;
				cport->needs_sr_multiplier = false;
			}

			cport->identifier = identifier;
			cport->min = lilv_node_as_float(lilv_port_get(plugin, port, uris.lv2_minimum));
			cport->max = lilv_node_as_float(lilv_port_get(plugin, port, uris.lv2_maximum));
			cport->value = lilv_node_as_float(lilv_port_get(plugin, port, uris.lv2_default));
			//bool is_toggle = lilv_node_as_bool( lilv_port_get(plugin,port,uris.lv2_toggled) );
			//bool is_enum = lilv_node_as_bool( lilv_port_get(plugin,port,uris.lv2_enumeration) );
			cport->name.parse_utf8(lilv_node_as_string(lilv_port_get_name(plugin, port)));

			lilv_instance_connect_port(instance, i, &cport->value);
			if (instance2) {
				lilv_instance_connect_port(instance2, i, &cport->value);
			}

			control_ports.push_back(cport);
		} else if (!lilv_port_is_a(plugin, port, uris.audio_port)) {
			printf("unknown port?\n");
			lilv_instance_connect_port(instance, i, NULL);
			if (instance2) {
				lilv_instance_connect_port(instance2, i, NULL);
			}
		}
	}

	lilv_instance_activate(instance);
	if (instance2) {
		lilv_instance_activate(instance2);
	}

	{
		const LilvNode *node = lilv_plugin_get_name(plugin);
		identifier.parse_utf8(lilv_node_as_uri(node));
		node = lilv_plugin_get_name(plugin);
		name.parse_utf8(lilv_node_as_string(node));
	}

	_update_buffers_and_ports();
}

void AudioEffectLV2::_plugin_finish() {
	lilv_instance_free(instance);
	if (instance2) {
		lilv_instance_free(instance2);
	}
}

void AudioEffectLV2::_update_buffers_and_ports() {
	const AudioEffectProviderLV2::URIS &uris = AudioEffectProviderLV2::get_uris();

	lilv_instance_deactivate(instance);
	if (instance2) {
		lilv_instance_deactivate(instance2);
	}

	int plugin_port_count = lilv_plugin_get_num_ports(plugin);

	in_buffers.clear();
	out_buffers.clear();

	for (int i = 0; i < plugin_port_count; i++) {
		const LilvPort *port = lilv_plugin_get_port_by_index(plugin, i);
		if (lilv_port_is_a(plugin, port, uris.audio_port)) {
			if (lilv_port_is_a(plugin, port, uris.output_port)) {

				Vector<float> buffer;
				buffer.resize(buffer_size);
				out_buffers.push_back(buffer);
				float *ptr = &out_buffers[out_buffers.size() - 1][0];
				lilv_instance_connect_port(instance, i, ptr);
				out_buffer_ptrs.push_back(ptr);

				if (instance2) {

					Vector<float> buffer2;
					buffer2.resize(buffer_size);
					out_buffers.push_back(buffer2);
					float *ptr2 = &out_buffers[out_buffers.size() - 1][0];
					lilv_instance_connect_port(instance2, i, ptr2);
					out_buffer_ptrs.push_back(ptr2);
				}

			} else if (lilv_port_is_a(plugin, port, uris.input_port)) {

				Vector<float> buffer;
				buffer.resize(buffer_size);
				in_buffers.push_back(buffer);
				float *ptr = &in_buffers[in_buffers.size() - 1][0];
				lilv_instance_connect_port(instance, i, ptr);
				in_buffer_ptrs.push_back(ptr);

				if (instance2) {

					Vector<float> buffer2;
					buffer2.resize(buffer_size);
					in_buffers.push_back(buffer2);
					float *ptr2 = &in_buffers[in_buffers.size() - 1][0];
					lilv_instance_connect_port(instance2, i, ptr2);
					in_buffer_ptrs.push_back(ptr2);
				}
			}
		}
	}

	for (int i = 0; i < control_ports.size(); i++) {
		if (control_ports[i]->needs_sr_multiplier) {
			control_ports[i]->multiplier = sampling_rate;
		}
	}
	lilv_instance_activate(instance);
	if (instance2) {
		lilv_instance_activate(instance2);
	}
}

AudioEffectLV2::AudioEffectLV2() {
	memset(&ext_data, 0, sizeof(LV2_Extension_Data_Feature));
	evbuf = NULL;
	buffer_size = 128;
	sampling_rate = 44100;
}

AudioEffectLV2::~AudioEffectLV2() {
	_plugin_finish();
	for (int i = 0; i < control_ports.size(); i++) {
		delete control_ports[i];
	}
}
