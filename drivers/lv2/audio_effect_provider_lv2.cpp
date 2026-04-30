#include "audio_effect_provider_lv2.h"
#include "engine/sound_driver_manager.h"

AudioEffectProviderLV2 *AudioEffectProviderLV2::singleton = NULL;
AudioEffectProviderLV2::URIS AudioEffectProviderLV2::uris;

AudioEffect *AudioEffectProviderLV2::instantiate_effect(const AudioEffectInfo *p_info) {

	// Fix #5: Check provider_id once before the loop, not every iteration.
	if (p_info->provider_id != "LV2") {
		return NULL;
	}

	if (!plugins) {
		lilv_world_load_all(world);
		plugins = lilv_world_get_all_plugins(world);
	}

	LilvIter *plug_itr = lilv_plugins_begin(plugins);

	for (int i = 0; i < lilv_plugins_size(plugins); i++) {
		const LilvPlugin *plugin = lilv_plugins_get(plugins, plug_itr);

		const LilvNode *node = lilv_plugin_get_uri(plugin);
		String unique_id;
		unique_id.parse_utf8(lilv_node_as_uri(node));

		if (unique_id != p_info->unique_ID) {
			plug_itr = lilv_plugins_next(plugins, plug_itr);
			continue;
		}

		AudioEffectLV2 *effect = new AudioEffectLV2;
		effect->setup(plugin, p_info->has_ui);

		return effect;
	}

	printf("not found?\n");
	return NULL;
}

void AudioEffectProviderLV2::scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata) {

	if (!plugins) {
		lilv_world_load_all(world);
		plugins = lilv_world_get_all_plugins(world);
	}

	int valid_effects = 0;

	// Fix #3: Create native_ui_type once before the loop.
	static const char *ui_type_uri = "http://lv2plug.in/ns/extensions/ui#Gtk3UI";
	LilvNode *native_ui_type = lilv_new_uri(world, ui_type_uri);

	LilvIter *plug_itr = lilv_plugins_begin(plugins);
	for (int i = 0; i < lilv_plugins_size(plugins); i++) {
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

		bool has_ui = false;
		const LilvNode *ui_type = NULL;
		LilvUIs *supported_uis = lilv_plugin_get_uis(plugin);
		LILV_FOREACH(uis, u, supported_uis) {
			const LilvUI *this_ui = lilv_uis_get(supported_uis, u);
			if (lilv_ui_is_supported(this_ui,
						suil_ui_supported,
						native_ui_type,
						&ui_type)) {
				has_ui = true;
				break;
			}
		}
		// Fix #4: Free supported_uis after use.
		lilv_uis_free(supported_uis);

		bool valid = false;

		if (has_ui) {

			if (event_input == 1) {
				//a synth with mono or stereo output is ok.
				if (input_count == 0 && output_count == 1) {
					valid = true;
				}
				if (input_count == 0 && output_count == 2) {
					valid = true;
				}
				if (input_count == 2 && output_count == 2) {
					valid = true;
				}
			} else {
				//for an effect, it needs stereo in and out

				if (input_count == 2 && output_count == 2) {
					valid = true;
				}
				if (input_count == 4 && output_count == 2) {
					valid = true;
				}
			}

		} else {

			if (event_input == 1) {
				if (input_count == 0 && output_count == 1) {
					valid = true;
				}
				if (input_count == 0 && output_count == 2) {
					valid = true;
				}
				if (input_count == 2 && output_count == 2) {
					valid = true;
				}

			} else {
				if (input_count == 4 && output_count == 2) {
					//sidechain
					valid = true;
				}
				if (input_count == 2 && output_count == 1) {
					//sidechain mono
					valid = true;
				}
				if (input_count == 2 && output_count == 2) {
					//stereo
					valid = true;
				}
				if (input_count == 1 && output_count == 1) {
					//mono
					valid = true;
				}
			}
		}

		LilvNodes *features = lilv_plugin_get_required_features(plugin);
		int count = lilv_nodes_size(features);
		lilv_nodes_free(features);

		if (valid) {

			AudioEffectInfo info;
			const LilvNode *uri_node = lilv_plugin_get_uri(plugin);
			info.provider_id = "LV2";
			info.unique_ID.parse_utf8(lilv_node_as_uri(uri_node));

			// Fix #2: lilv_plugin_get_name returns an owned node — must free it.
			LilvNode *name_node = lilv_plugin_get_name(plugin);
			info.caption.parse_utf8(lilv_node_as_string(name_node));
			lilv_node_free(name_node);

			const LilvNode *class_node = lilv_plugin_class_get_label(lilv_plugin_get_class(plugin));
			info.category.parse_utf8(lilv_node_as_string(class_node));

			info.provider_caption = "LADSPAv2";
			info.synth = event_input == 1;
			info.version = "0";
			info.has_ui = has_ui;

			p_factory->add_audio_effect(info);

			if (p_callback) {
				p_callback(info.caption, p_userdata);
			}
			valid_effects++;
		} else {
			// Fix #2: Same issue in the discard branch.
			LilvNode *name_node = lilv_plugin_get_name(plugin);
			printf("discarded %s ins %i, outs %i, events %i, ui %i\n", lilv_node_as_string(name_node), input_count, output_count, event_input, int(has_ui));
			lilv_node_free(name_node);
		}

		plug_itr = lilv_plugins_next(plugins, plug_itr);
	}

	// Fix #3: Free native_ui_type after the loop.
	lilv_node_free(native_ui_type);

	printf("Total valid effects: %i\n", valid_effects);
}

String AudioEffectProviderLV2::get_id() const {
	return "LV2";
}

String AudioEffectProviderLV2::get_name() const {
	return "LADSPAv2";
}

AudioEffectProviderLV2::AudioEffectProviderLV2(int *argc, char ***argv) {

	plugins = NULL;
	singleton = this;
	world = lilv_world_new();

	// Fix #7: Bail out if world creation failed.
	if (!world) {
		printf("ERROR: Failed to create LilvWorld\n");
		return;
	}

	uris.atom_port = lilv_new_uri(world, LILV_URI_ATOM_PORT);
	uris.audio_port = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
	uris.control_port = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
	uris.cv_port = lilv_new_uri(world, LILV_URI_CV_PORT);
	uris.event_port = lilv_new_uri(world, LILV_URI_EVENT_PORT);
	uris.input_port = lilv_new_uri(world, LILV_URI_INPUT_PORT);
	uris.midi_event = lilv_new_uri(world, LILV_URI_MIDI_EVENT);
	uris.output_port = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
	uris.port = lilv_new_uri(world, LILV_URI_PORT);

	uris.lv2_AudioPort = lilv_new_uri(world, LV2_CORE__AudioPort);
	uris.lv2_CVPort = lilv_new_uri(world, LV2_CORE__CVPort);
	uris.lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
	uris.lv2_InputPort = lilv_new_uri(world, LV2_CORE__InputPort);
	uris.lv2_OutputPort = lilv_new_uri(world, LV2_CORE__OutputPort);
	uris.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
	uris.lv2_control = lilv_new_uri(world, LV2_CORE__control);
	uris.lv2_default = lilv_new_uri(world, LV2_CORE__default);
	uris.lv2_enumeration = lilv_new_uri(world, LV2_CORE__enumeration);
	uris.lv2_integer = lilv_new_uri(world, LV2_CORE__integer);
	uris.lv2_maximum = lilv_new_uri(world, LV2_CORE__maximum);
	uris.lv2_minimum = lilv_new_uri(world, LV2_CORE__minimum);
	uris.lv2_name = lilv_new_uri(world, LV2_CORE__name);
	uris.lv2_reportsLatency = lilv_new_uri(world, LV2_CORE__reportsLatency);
	uris.lv2_sampleRate = lilv_new_uri(world, LV2_CORE__sampleRate);
	uris.lv2_symbol = lilv_new_uri(world, LV2_CORE__symbol);
	uris.lv2_toggled = lilv_new_uri(world, LV2_CORE__toggled);

	uris.work_interface = lilv_new_uri(world, LV2_WORKER__interface);

	uris.port_minimum_size = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	uris.atom_chunk = lilv_new_uri(world, LV2_ATOM__Chunk);
	uris.atom_sequence = lilv_new_uri(world, LV2_ATOM__Sequence);
}

// Fix #1: Destructor frees all owned URI nodes and the world.
AudioEffectProviderLV2::~AudioEffectProviderLV2() {

	if (!world) {
		return;
	}

	lilv_node_free(uris.atom_port);
	lilv_node_free(uris.audio_port);
	lilv_node_free(uris.control_port);
	lilv_node_free(uris.cv_port);
	lilv_node_free(uris.event_port);
	lilv_node_free(uris.input_port);
	lilv_node_free(uris.midi_event);
	lilv_node_free(uris.output_port);
	lilv_node_free(uris.port);
	lilv_node_free(uris.port_minimum_size);
	lilv_node_free(uris.atom_chunk);
	lilv_node_free(uris.atom_sequence);

	lilv_node_free(uris.lv2_AudioPort);
	lilv_node_free(uris.lv2_CVPort);
	lilv_node_free(uris.lv2_ControlPort);
	lilv_node_free(uris.lv2_InputPort);
	lilv_node_free(uris.lv2_OutputPort);
	lilv_node_free(uris.lv2_connectionOptional);
	lilv_node_free(uris.lv2_control);
	lilv_node_free(uris.lv2_default);
	lilv_node_free(uris.lv2_enumeration);
	lilv_node_free(uris.lv2_integer);
	lilv_node_free(uris.lv2_maximum);
	lilv_node_free(uris.lv2_minimum);
	lilv_node_free(uris.lv2_name);
	lilv_node_free(uris.lv2_reportsLatency);
	lilv_node_free(uris.lv2_sampleRate);
	lilv_node_free(uris.lv2_symbol);
	lilv_node_free(uris.lv2_toggled);

	lilv_node_free(uris.work_interface);

	lilv_world_free(world);

	if (singleton == this) {
		singleton = NULL;
	}
}
