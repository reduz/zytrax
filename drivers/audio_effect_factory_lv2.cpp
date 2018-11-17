#include "audio_effect_factory_lv2.h"
#include "engine/sound_driver_manager.h"
#ifdef UNIX_ENABLED
//process
bool AudioEffectLV2::process(const AudioFrame2 *p_in, AudioFrame2 *p_out, const Event *p_events, bool p_prev_active) {

	return false;
}

//info
bool AudioEffectLV2::has_synth() const {
	return false;
}

const AudioEffectInfo *AudioEffectLV2::get_info() const {
}

int AudioEffectLV2::get_control_port_count() const {
	return 0;
}
ControlPort *AudioEffectLV2::get_control_port(int p_port) {
	return 0;
}
const ControlPort *AudioEffectLV2::get_control_port(int p_port) const {
	return NULL;
}

void AudioEffectLV2::_clear() {

	if (instance) {
		lilv_instance_free(instance);
		instance = NULL;
	}

	if (instance_mono_pair) {
		lilv_instance_free(instance_mono_pair);
		instance_mono_pair = NULL;
	}

	controls.clear();
	control_outs.clear();

	active = false;
}

void AudioEffectLV2::reset() {

	_clear();
	const LilvPlugins *plugins = lilv_world_get_all_plugins(world);

	const LilvPlugin *plugin = NULL;
	LilvIter *plug_itr = lilv_plugins_begin(plugins);
	for (int i = 0; i < lilv_plugins_size(plugins); ++i) {
		const LilvPlugin *plugin = lilv_plugins_get(plugins, plug_itr);
		String unique_id;
		unique_id.parse_utf8(lilv_node_as_uri(lilv_plugin_get_uri(plugin)));

		if (unique_id == p_info->unique_ID) {
			break;
		}
		plug_itr = lilv_plugins_next(plugins, plug_itr);
	}

	ERR_FAIL_COND_V(!plugin, NULL); //sorry not found

	int ports = lilv_plugin_get_num_ports(plugin);

	int input_count = 0;
	int output_count = 0;
	int event_input = 0;
	int control_in = 0;
	int control_out = 0;


	buff_size=SoundDriverManager::get_internal_buffer_size();
	int midi_buff_size = 4096;
	bool use_midi = false;
	bool use_old_midi_api = false;

	//check buffer sizes
	for (int j = 0; j < ports; j++) {

		const LilvPort *port = lilv_plugin_get_port_by_index(plugin, j);
		if (lilv_port_is_a(plugin, port, uris.audio_port)) {

			LilvNode* min_size = lilv_port_get(plugin,port,AudioEffectProviderLV2::singleton->uris.port_minimum_size);
			if (min_size && lilv_node_is_int(min_size)) {
				int size = lilv_node_as_int(min_size);
				buff_size = MIN(size,buff_size);
			}
		}

		if ((lilv_port_is_a(plugin, port, uris.event_port) || lilv_port_is_a(plugin, port, uris.atom_port)) && lilv_port_is_a(plugin, port, uris.input_port)) {

			LilvNode* min_size = lilv_port_get(plugin,port,AudioEffectProviderLV2::singleton->uris.port_minimum_size);
			if (min_size && lilv_node_is_int(min_size)) {
				int size = lilv_node_as_int(min_size);
				midi_buff_size = MIN(size,midi_buff_size);
				use_midi=true;
			}

			use_old_midi_api = lilv_port_is_a(plugin, port, uris.event_port);
		}
	}

	in_buff_left.resize(buff_size);
	in_buff_right.resize(buff_size);
	out_buff_left.resize(buff_size);
	out_buff_right.resize(buff_size);

	if (use_midi) {
		evbuf = lv2_evbuf_new( midi_buff_size, use_old_midi_api ? LV2_EVBUF_EVENT : LV2_EVBUF_ATOM,
					jalv->map.map(jalv->map.handle,
						      lilv_node_as_string(jalv->nodes.atom_Chunk)),
					jalv->map.map(jalv->map.handle,
						      lilv_node_as_string(jalv->nodes.atom_Sequence)));




					jalv->plugin, port->lilv_port, jalv->nodes.rsz_minimumSize);
				if (min_size && lilv_node_is_int(min_size)) {
					port->buf_size = lilv_node_as_int(min_size);
					jalv->opts.buffer_size = MAX(
						jalv->opts.buffer_size, port->buf_size * N_BUFFER_CYCLES);
				}
				lilv_node_free(min_size);

			if (lilv_port_is_a(plugin, port, uris.output_port)) {
				output_count++;
			} else if (lilv_port_is_a(plugin, port, uris.input_port)) {
				input_count++;
			}
		}
		if ((lilv_port_is_a(plugin, port, uris.event_port) || lilv_port_is_a(plugin, port, uris.atom_port)) && lilv_port_is_a(plugin, port, uris.input_port)) {

			event_input++;
		}

		if (lilv_port_is_a(plugin, port, uris.control_port)) {
			if (lilv_port_is_a(plugin, port, uris.output_port)) {
				control_out++;
			} else if (lilv_port_is_a(plugin, port, uris.input_port)) {
				control_in++;
			}
		}
	}

	int plugins_to_create = output_count;

	mix_rate = SoundDriverManager::get_driver()->get_mix_rate();

	LV2_Feature *feat = NULL;
	instance = lilv_plugin_instantiate(plugin, mix_rate, &feat);
	if (!instance)
		return; //well, failed

	if (output_count == 1) {
		instance_mono_pair = lilv_plugin_instantiate(plugin, mix_rate, &feat);
	}
}

/* Load/Save */

Error AudioEffectLV2::save(TreeSaver *p_tree) {
	return OK;
}

Error AudioEffectLV2::load(TreeLoader *p_tree) {
	return OK;
}

AudioEffectLV2::AudioEffectLV2() {

	active = false;
	instance = NULL;
	instance_mono_pair = NULL;
	mix_rate = 44100;
	evbuf = NULL;
}

AudioEffectLV2::~AudioEffectLV2() {
	_clear();
}

AudioEffect *AudioEffectProviderLV2::create_effects(const AudioEffectInfo *p_info) {

	return ((AudioEffectProviderLV2 *)p_info->provider)->create_effect(p_info);
}

AudioEffect *AudioEffectProviderLV2::create_effect(const AudioEffectInfo *p_info) {

	AudioEffectLV2 *effect = new AudioEffectLV2;

	effect->info = *p_info;

	effect->reset();

	if (!effect->active) {
		delete effect;
		effect = NULL;
	}

	return effect;

	return NULL;
}
void AudioEffectProviderLV2::scan_effects(AudioEffectFactory *p_factory) {

	lilv_world_load_all(world);
	const LilvPlugins *plugins = lilv_world_get_all_plugins(world);

	int valid_effects = 0;

	LilvIter *plug_itr = lilv_plugins_begin(plugins);
	for (int i = 0; i < lilv_plugins_size(plugins); ++i) {
		const LilvPlugin *plugin = lilv_plugins_get(plugins, plug_itr);

		//lets see if this is useful
		int ports = lilv_plugin_get_num_ports(plugin);

		int input_count = 0;
		int output_count = 0;
		int event_input = 0;
		int event_output = 0;

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

		bool valid = false;

		if (input_count == 0 && event_input == 1) {
			//generator
			if (output_count == 1 || output_count == 2) {
				valid = true;
			}
		} else if (input_count == output_count) {
			if (output_count == 1 || output_count == 2) {
				valid = true;
			}
		}

		if (event_output > 0)
			valid = false; //not sure what this is, but i guess its not useful

		if (event_input > 1)
			valid = false; //only one input supported

		LilvNodes *features = lilv_plugin_get_required_features(plugin);
		int count = lilv_nodes_size(features);
		lilv_nodes_free(features);
		if (count > 0) {
			valid = false;
		}

		if (valid) {

			AudioEffectInfo info;
			const LilvNode *node = lilv_plugin_get_uri(plugin);
			info.unique_ID.parse_utf8(lilv_node_as_uri(node));

			node = lilv_plugin_get_name(plugin);
			info.caption.parse_utf8(lilv_node_as_string(node));

			node = lilv_plugin_class_get_label(lilv_plugin_get_class(plugin));
			info.category.parse_utf8(lilv_node_as_string(node));

			info.provider = this;
			info.synth = event_input == 1;
			info.version = 0;

			p_factory->add_audio_effect(info);

			printf("plugin.uri = %s\n", info.unique_ID.utf8().get_data());
			printf("plugin.caption = %s\n", info.caption.utf8().get_data());
			printf("plugin.category = %s\n", info.category.utf8().get_data());

			valid_effects++;
		}

		plug_itr = lilv_plugins_next(plugins, plug_itr);
	}

	printf("Total valid effects: %i\n", valid_effects);

#if 0
	/* Cache URIs for concepts we'll use */
	nodes.atom_AtomPort = lilv_new_uri(world, LV2_ATOM__AtomPort);
	nodes.atom_Chunk = lilv_new_uri(world, LV2_ATOM__Chunk);
	nodes.atom_Float = lilv_new_uri(world, LV2_ATOM__Float);
	nodes.atom_Path = lilv_new_uri(world, LV2_ATOM__Path);
	nodes.atom_Sequence = lilv_new_uri(world, LV2_ATOM__Sequence);
	nodes.ev_EventPort = lilv_new_uri(world, LV2_EVENT__EventPort);
	nodes.lv2_AudioPort = lilv_new_uri(world, LV2_CORE__AudioPort);
	nodes.lv2_CVPort = lilv_new_uri(world, LV2_CORE__CVPort);
	nodes.lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
	nodes.lv2_InputPort = lilv_new_uri(world, LV2_CORE__InputPort);
	nodes.lv2_OutputPort = lilv_new_uri(world, LV2_CORE__OutputPort);
	nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
	nodes.lv2_control = lilv_new_uri(world, LV2_CORE__control);
	nodes.lv2_default = lilv_new_uri(world, LV2_CORE__default);
	nodes.lv2_enumeration = lilv_new_uri(world, LV2_CORE__enumeration);
	nodes.lv2_integer = lilv_new_uri(world, LV2_CORE__integer);
	nodes.lv2_maximum = lilv_new_uri(world, LV2_CORE__maximum);
	nodes.lv2_minimum = lilv_new_uri(world, LV2_CORE__minimum);
	nodes.lv2_name = lilv_new_uri(world, LV2_CORE__name);
	nodes.lv2_reportsLatency = lilv_new_uri(world, LV2_CORE__reportsLatency);
	nodes.lv2_sampleRate = lilv_new_uri(world, LV2_CORE__sampleRate);
	nodes.lv2_symbol = lilv_new_uri(world, LV2_CORE__symbol);
	nodes.lv2_toggled = lilv_new_uri(world, LV2_CORE__toggled);
	nodes.midi_MidiEvent = lilv_new_uri(world, LV2_MIDI__MidiEvent);
	nodes.pg_group = lilv_new_uri(world, LV2_PORT_GROUPS__group);
	nodes.pprops_logarithmic = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
	nodes.pprops_notOnGUI = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
	nodes.pprops_rangeSteps = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
	nodes.pset_Preset = lilv_new_uri(world, LV2_PRESETS__Preset);
	nodes.pset_bank = lilv_new_uri(world, LV2_PRESETS__bank);
	nodes.rdfs_comment = lilv_new_uri(world, LILV_NS_RDFS "comment");
	nodes.rdfs_label = lilv_new_uri(world, LILV_NS_RDFS "label");
	nodes.rdfs_range = lilv_new_uri(world, LILV_NS_RDFS "range");
	nodes.rsz_minimumSize = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	nodes.work_interface = lilv_new_uri(world, LV2_WORKER__interface);
	nodes.work_schedule = lilv_new_uri(world, LV2_WORKER__schedule);
	nodes.end = NULL;
#endif
}

String AudioEffectProviderLV2::get_name() const {
	return "LADSPAv2";
}
/*
uint32_t AudioEffectProviderLV2::uri_to_id(LV2_URI_Map_Callback_Data callback_data,
		const char *map,
		const char *uri) {

	AudioEffectProviderLV2 *self = (AudioEffectProviderLV2 *)callback_data;
	//	Jalv* jalv = (Jalv*)callback_data;
	//	zix_sem_wait(&jalv->symap_lock);
	const LV2_URID id = self->symap_map(self->symap, uri);
	//	zix_sem_post(&jalv->symap_lock);
	return id;
}
*/

AudioEffectProviderLV2 *AudioEffectProviderLV2::singleton=NULL;

AudioEffectProviderLV2::AudioEffectProviderLV2(int *argc, char ***argv) {

	singleton=this;
	world = lilv_world_new();

	uris.atom_port = lilv_new_uri(world, LILV_URI_ATOM_PORT);
	uris.audio_port = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
	uris.control_port = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
	uris.cv_port = lilv_new_uri(world, LILV_URI_CV_PORT);
	uris.event_port = lilv_new_uri(world, LILV_URI_EVENT_PORT);
	uris.input_port = lilv_new_uri(world, LILV_URI_INPUT_PORT);
	uris.midi_event = lilv_new_uri(world, LILV_URI_MIDI_EVENT);
	uris.output_port = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
	uris.port = lilv_new_uri(world, LILV_URI_PORT);

	uris.port_minimum_size = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	uris.atom_chunk = lilv_new_uri(world, LV2_ATOM__Chunk);
	uris.atom_sequence = lilv_new_uri(world, LV2_ATOM__Sequence);

#if 0
	suil_init(argc, argv, SUIL_ARG_NONE);

	symap = symap_new();
	lv2_atom_forge_init(&forge, &map);
	env = serd_env_new(NULL);

	serd_env_set_prefix_from_strings(
			env, (const uint8_t *)"patch", (const uint8_t *)LV2_PATCH_PREFIX);
	serd_env_set_prefix_from_strings(
			env, (const uint8_t *)"time", (const uint8_t *)LV2_TIME_PREFIX);
	serd_env_set_prefix_from_strings(
			env, (const uint8_t *)"xsd", (const uint8_t *)NS_XSD);

	sratom = sratom_new(&jalv.map);
	ui_sratom = sratom_new(&jalv.map);
	sratom_set_env(sratom, jalv.env);
	sratom_set_env(ui_sratom, env);

	midi_event_id = uri_to_id(this, "http://lv2plug.in/ns/ext/event", LV2_MIDI__MidiEvent);

	urids.atom_Float = symap_map(symap, LV2_ATOM__Float);
	urids.atom_Int = symap_map(symap, LV2_ATOM__Int);
	urids.atom_Object = symap_map(symap, LV2_ATOM__Object);
	urids.atom_Path = symap_map(symap, LV2_ATOM__Path);
	urids.atom_String = symap_map(symap, LV2_ATOM__String);
	urids.atom_eventTransfer = symap_map(symap, LV2_ATOM__eventTransfer);
	urids.bufsz_maxBlockLength = symap_map(symap, LV2_BUF_SIZE__maxBlockLength);
	urids.bufsz_minBlockLength = symap_map(symap, LV2_BUF_SIZE__minBlockLength);
	urids.bufsz_sequenceSize = symap_map(symap, LV2_BUF_SIZE__sequenceSize);
	urids.log_Error = symap_map(symap, LV2_LOG__Error);
	urids.log_Trace = symap_map(symap, LV2_LOG__Trace);
	urids.log_Warning = symap_map(symap, LV2_LOG__Warning);
	urids.midi_MidiEvent = symap_map(symap, LV2_MIDI__MidiEvent);
	urids.param_sampleRate = symap_map(symap, LV2_PARAMETERS__sampleRate);
	urids.patch_Get = symap_map(symap, LV2_PATCH__Get);
	urids.patch_Put = symap_map(symap, LV2_PATCH__Put);
	urids.patch_Set = symap_map(symap, LV2_PATCH__Set);
	urids.patch_body = symap_map(symap, LV2_PATCH__body);
	urids.patch_property = symap_map(symap, LV2_PATCH__property);
	urids.patch_value = symap_map(symap, LV2_PATCH__value);
	urids.time_Position = symap_map(symap, LV2_TIME__Position);
	urids.time_bar = symap_map(symap, LV2_TIME__bar);
	urids.time_barBeat = symap_map(symap, LV2_TIME__barBeat);
	urids.time_beatUnit = symap_map(symap, LV2_TIME__beatUnit);
	urids.time_beatsPerBar = symap_map(symap, LV2_TIME__beatsPerBar);
	urids.time_beatsPerMinute = symap_map(symap, LV2_TIME__beatsPerMinute);
	urids.time_frame = symap_map(symap, LV2_TIME__frame);
	urids.time_speed = symap_map(symap, LV2_TIME__speed);
	urids.ui_updateRate = symap_map(symap, LV2_UI__updateRate);

	temp_dir = "/tmp/jalv-XXXXXX/";

	memset(&jalv, '\0', sizeof(Jalv));
	jalv.prog_name = argv[0];
	jalv.block_length = 4096; /* Should be set by backend */
	jalv.midi_buf_size = 1024; /* Should be set by backend */
	jalv.play_state = JALV_PAUSED;
	jalv.bpm = 120.0f;
	jalv.control_in = (uint32_t)-1;
#endif
}
#endif
