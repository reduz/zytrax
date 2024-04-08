#include <gtkmm.h>

#ifdef VST2_ENABLED
#include "drivers/vst2/factory_wrapper_vst2.h"
#endif

#ifdef LV2_ENABLED
#include "drivers/lv2/audio_effect_provider_lv2.h"
#include "drivers/lv2/effect_editor_lv2.h"
#endif

#include "effects/effects.h"
#include "engine/song.h"
#include "globals/json_file.h"
#include "gui/interface.h"

#ifdef RTAUDIO_ENABLED
#include "drivers/rtaudio/sound_driver_rtaudio.h"
#endif

#ifdef JACK_ENABLED
#include "drivers/jack/sound_driver_jack.h"
#endif

#ifdef RTMIDI_ENABLED
#include "drivers/rtmidi/midi_driver_rtmidi.h"
#endif
int main(int argc, char *argv[]) {

	AudioEffectFactory effect_factory;

	printf("one\n");
#ifdef VST2_ENABLED
	AudioEffectProvider *provider_vst2 = create_vst2_provider();
	effect_factory.add_provider(provider_vst2);
#endif

	printf("two\n");
#ifdef LV2_ENABLED
	AudioEffectProviderLV2 provider_lv2(&argc, &argv);
	effect_factory.add_provider(&provider_lv2);
	EffectEditorLV2::initialize_lv2_editor();
#endif
	printf("three\n");
#ifdef RTAUDIO_ENABLED
	register_rtaudio_driver();
#endif

#ifdef JACK_ENABLED
	register_jack_driver();
#endif

#ifdef RTMIDI_ENABLED
	register_rtmidi_driver();
#endif
	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.examples.base");

	Theme theme;
	KeyBindings key_bindings;

	/* Time to load the Settings */
	{
		String path = SettingsDialog::get_settings_path() + "/settings.json";
		JSON::Node node;
		int use_driver_index = SoundDriverManager::get_driver_count() ? 0 : -1;
		int use_midi_in_driver_index = MIDIDriverManager::get_input_driver_count() ? 0 : -1;

		if (load_json(path, node) == OK) {

			if (node.has("audio")) { //audio
				JSON::Node audio_node = node.get("audio");
				std::string driver_id = audio_node.get("id").toString();
				printf("Driver ID '%s'\n",driver_id.c_str());
				for (int i = 0; i < SoundDriverManager::get_driver_count(); i++) {
					SoundDriver *driver = SoundDriverManager::get_driver(i);
					if (driver->get_id() == driver_id.c_str()) {
						printf("found\n");
						use_driver_index = i;
						break;
					}

				}

				int mixing_hz = audio_node.get("mixing_hz");
				int buffer_size = audio_node.get("buffer_size");
				int block_size = audio_node.get("block_size");

				if (mixing_hz >= 0 && mixing_hz < SoundDriverManager::MIX_FREQ_MAX) {
					SoundDriverManager::set_mix_frequency(SoundDriverManager::MixFrequency(mixing_hz));
				}
				if (buffer_size >= 0 && buffer_size < SoundDriverManager::BUFFER_SIZE_MAX) {
					SoundDriverManager::set_buffer_size(SoundDriverManager::BufferSize(buffer_size));
				}

				if (block_size >= 0 && block_size < SoundDriverManager::BUFFER_SIZE_MAX) {
					SoundDriverManager::set_step_buffer_size(SoundDriverManager::BufferSize(block_size));
				}

				std::string midi_driver_id = audio_node.get("midi_in_id").toString();

				for (int i = 0; i < MIDIDriverManager::get_input_driver_count(); i++) {
					MIDIInputDriver *driver = MIDIDriverManager::get_input_driver(i);
					if (driver->get_id() == midi_driver_id.c_str()) {
						use_midi_in_driver_index = i;
					}

					break;
				}
			}

			if (node.has("plugins")) { //plugins
				JSON::Node plugin_node = node.get("plugins");
				for (int i = 0; i < AudioEffectProvider::MAX_SCAN_PATHS; i++) {
					std::string key = String::num(i).ascii().get_data();
					if (plugin_node.has(key)) {
						std::string path = plugin_node.get(key).toString();
						String pathu;
						pathu.parse_utf8(path.c_str());
						AudioEffectProvider::set_scan_path(i, pathu);
					}
				}
			}

			if (node.has("theme")) { //theme
				JSON::Node theme_node = node.get("theme");
				if (theme_node.has("font")) {

					theme.font.parse_utf8(theme_node.get("font").toString().c_str());
				}

				if (theme_node.has("colors")) {

					JSON::Node colors_node = theme_node.get("colors");
					for (int i = 0; i < Theme::COLOR_MAX; i++) {
						if (colors_node.has(theme.color_names[i])) {

							JSON::Node array = colors_node.get(theme.color_names[i]);
							theme.colors[i].set_red(array.get(0).toFloat());
							theme.colors[i].set_green(array.get(1).toFloat());
							theme.colors[i].set_blue(array.get(2).toFloat());
						}
					}

					if (theme_node.has("use_dark_theme") && bool(theme_node.get("use_dark_theme").toBool())) {
						theme.color_scheme = Theme::COLOR_SCHEME_DARK;
					}
				}
			}

			if (node.has("key_bindings")) { //key bindings

				JSON::Node bindings = node.get("key_bindings");
				if (bindings.has("keys")) {

					JSON::Node array = bindings.get("keys");

					for (int i = 0; i < array.getCount(); i++) {

						JSON::Node bind = array.get(i);
						std::string name = bind.get("name").toString();

						KeyBindings::KeyBind name_index = KeyBindings::BIND_MAX;
						for (int j = 0; j < KeyBindings::BIND_MAX; j++) {
							KeyBindings::KeyBind b = KeyBindings::KeyBind(j);
							if (name == key_bindings.get_keybind_name(b)) {
								name_index = b;

								break;
							}
						}

						if (name_index != KeyBindings::BIND_MAX) {

							int key = bind.get("key").toInt();
							int state = bind.get("mods").toInt();

							key_bindings.set_keybind(name_index, key, state);
						}
					}
				}
			}

			if (node.has("default_commands")) { //default commands

				JSON::Node def_commands = node.get("default_commands");

				for (int i = 0; i < def_commands.getCount(); i++) {

					JSON::Node command = def_commands.get(i);

					int index = command.get("index").toInt();
					String name;
					name.parse_utf8(command.get("identifier").toString().c_str());
					char c = char(command.get("command").toInt());

					SettingsDialog::set_default_command(index, name, c);
				}
			}

			if (node.has("midi_banks")) { //default commands

				JSON::Node midi_banks = node.get("midi_banks");

				for (int i = 0; i < midi_banks.getCount(); i++) {

					JSON::Node bank = midi_banks.get(i);

					int index = bank.get("index").toInt();
					String path;
					path.parse_utf8(bank.get("path").toString().c_str());

					MIDIBankManager::set_device_file_path(index,path);
				}

			}

			if (node.has("favorite_midi_patches")) { //default commands

				JSON::Node favorites = node.get("favorite_midi_patches");

				for (int i = 0; i < favorites.getCount(); i++) {

					MIDIBankManager::get_favorites().insert(favorites.get(i).toString());
				}

			}

		}

		SoundDriverManager::init_driver(use_driver_index);
		MIDIDriverManager::init_input_driver(use_midi_in_driver_index);
		MIDIBankManager::reload_devices();
	}

	printf("regfx\n");
	register_effects(&effect_factory);

	/* make it dark */
	if (theme.color_scheme == Theme::COLOR_SCHEME_DARK) {
		g_object_set(gtk_settings_get_default(),
				"gtk-application-prefer-dark-theme", TRUE,
				NULL);
	}

	/* Load the cached plugins */

	{ //plugins

		String path = SettingsDialog::get_settings_path() + "/plugins.json";
		JSON::Node node;

		if (load_json(path, node) == OK) {

			JSON::Node plugin_array = node.get("plugins");

			for (int i = 0; i < plugin_array.getCount(); i++) {

				JSON::Node plugin_node = plugin_array.get(i);
				AudioEffectInfo info;

				info.caption.parse_utf8(plugin_node.get("caption").toString().c_str());
				info.description.parse_utf8(plugin_node.get("description").toString().c_str());
				info.author.parse_utf8(plugin_node.get("author").toString().c_str());
				info.category.parse_utf8(plugin_node.get("category").toString().c_str());
				info.unique_ID.parse_utf8(plugin_node.get("unique_id").toString().c_str());
				info.icon_string.parse_utf8(plugin_node.get("icon_string").toString().c_str());
				info.version.parse_utf8(plugin_node.get("version").toString().c_str());
				info.provider_caption.parse_utf8(plugin_node.get("provider_caption").toString().c_str());
				info.provider_id.parse_utf8(plugin_node.get("provider_id").toString().c_str());
				info.path.parse_utf8(plugin_node.get("path").toString().c_str());

				info.synth = plugin_node.get("synth").toBool();
				info.has_ui = plugin_node.get("has_ui").toBool();

				effect_factory.add_audio_effect(info);
			}
		}
	}

	/* Initialize the UI */

	printf("go\n");

	Interface * window = new Interface(app.operator->(), &effect_factory, &theme, &key_bindings);
	window->set_default_size(1280, 720);
#ifdef VST2_ENABLED
	window->add_editor_plugin_function(get_vst2_editor_function());
#endif
#ifdef LV2_ENABLED
	window->add_editor_plugin_function(get_lv2_editor_function());
#endif
	int ret = app->run(*window);

	delete window;
	printf("bye\n");

	SoundDriverManager::finish_driver();

#ifdef VST2_ENABLED
	delete provider_vst2;
#endif

#ifdef RTAUDIO_ENABLED
	cleanup_rtaudio_driver();
#endif

#ifdef JACK_ENABLED
	cleanup_jack_driver();
#endif

#ifdef LV2_ENABLED
	EffectEditorLV2::finalize_lv2_editor();
#endif
	return ret;
}
