#include "song_file.h"
#include "base64.h"
#include "engine/sound_driver_manager.h"
#include "json_file.h"

#include <map>

class AudioEffectDummy : public AudioEffect {
	//used to instantiate in place of plugins that were not found
public:
	String unique_id;
	String provider_id;
	Vector<ControlPortDefault> ports;
	JSON::Node json;

	virtual bool has_secondary_input() const { return false; }
	virtual void process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {}
	virtual void process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {}

	//info
	virtual String get_name() const { return "DummyPlugin"; }
	virtual String get_unique_id() const { return unique_id; }
	virtual String get_provider_id() const { return provider_id; }

	virtual int get_control_port_count() const { return ports.size(); }
	virtual ControlPort *get_control_port(int p_port) { return &ports[p_port]; }

	virtual void set_process_block_size(int p_size) {}
	virtual void set_sampling_rate(int p_hz) {}

	virtual void reset() {}

	/* Load/Save */

	virtual JSON::Node to_json() const { return json; }
	virtual Error from_json(const JSON::Node &node) {
		json = node;
		return OK;
	}

	AudioEffectDummy() {}
};

Error SongFile::save(const String &p_path) {

	JSON::Node node = JSON::object();

	node.add("software", VERSION_SOFTWARE_NAME);

	std::map<std::string, std::shared_ptr<Vector<uint8_t> > > shared_data_map;

	{
		JSON::Node version = JSON::object();
		version.add("major", VERSION_MAJOR);
		version.add("minor", VERSION_MINOR);
		version.add("status", _MKSTR(VERSION_STATUS));
		node.add("version", version);
	}

	{
		JSON::Node information = JSON::object();
		information.add("name", song->get_name().utf8().get_data());
		information.add("author", song->get_author().utf8().get_data());
		information.add("description", song->get_description().utf8().get_data());
		node.add("information", information);
	}

	{
		JSON::Node tempo = JSON::object();
		tempo.add("bpm", song->get_bpm());
		tempo.add("swing", int(song->get_swing() * 100));
		node.add("tempo", tempo);
	}

	JSON::Node tracks = JSON::array();

	for (int i = 0; i < song->get_track_count(); i++) {
		Track *t = song->get_track(i);
		JSON::Node track = JSON::object();
		track.add("name", t->get_name().utf8().get_data());
		track.add("volume", t->get_mix_volume_db());
		track.add("muted", t->is_muted());
		track.add("columns", t->get_column_count());
		track.add("command_columns", t->get_command_column_count());

		JSON::Node automations = JSON::array();

		for (int j = 0; j < t->get_automation_count(); j++) {
			Automation *a = t->get_automation(j);
			JSON::Node automation = JSON::object();
			int effect_idx = -1;
			String param_name;
			for (int k = 0; k < t->get_audio_effect_count(); k++) {
				AudioEffect *fx = t->get_audio_effect(k);

				if (a->get_owner() == fx) {
					for (int l = 0; l < fx->get_control_port_count(); l++) {
						if (a->get_control_port() == fx->get_control_port(l)) {
							param_name = fx->get_control_port(l)->get_identifier();
							effect_idx = k;
							break;
						}
					}
					break;
				}
			}

			if (effect_idx == -1) {
				continue;
			}

			automation.add("effect_index", effect_idx);
			automation.add("parameter", param_name.utf8().get_data());
			String edit_mode;
			switch (a->get_edit_mode()) {
				case Automation::EDIT_ROWS_DISCRETE: edit_mode = "discrete_numbers"; break;
				case Automation::EDIT_ENVELOPE_SMALL: edit_mode = "envelope_small"; break;
				case Automation::EDIT_ENVELOPE_LARGE: edit_mode = "envelope_large"; break;
			}
			automation.add("edit_mode", edit_mode.utf8().get_data());
			automations.add(automation);
		}
		track.add("automations", automations);

		JSON::Node effects = JSON::array();

		for (int j = 0; j < t->get_audio_effect_count(); j++) {
			AudioEffect *fx = t->get_audio_effect(j);
			JSON::Node effect = JSON::object();
			effect.add("provider_id", fx->get_provider_id().utf8().get_data());
			effect.add("id", fx->get_unique_id().utf8().get_data());
			effect.add("skip", fx->is_skipped());
			String shared_data_key = fx->get_shared_data_key();
			if (shared_data_key != String()) {
				std::string sd = shared_data_key.utf8().get_data();
				if (shared_data_map.find(sd) == shared_data_map.end()) {
					shared_data_map[sd] = fx->get_shared_data();
					effect.add("shared_data", sd);
				}
			}

			JSON::Node commands = JSON::object();
			bool added_commands = false;
			for (int k = 0; k < fx->get_control_port_count(); k++) {
				char command = fx->get_control_port(k)->get_command();
				if (command != 0) {
					const char s[2] = { command, 0 };
					commands.add(fx->get_control_port(k)->get_identifier().utf8().get_data(), s);
					added_commands = true;
				}
			}
			if (added_commands) {
				effect.add("commands", commands);
			}

			effect.add("state", fx->to_json());
			effects.add(effect);
		}

		track.add("effects", effects);

		JSON::Node sends = JSON::array();
		for (int j = 0; j < t->get_send_count(); j++) {
			JSON::Node send = JSON::object();
			send.add("to_track", t->get_send_track(j));
			send.add("amount", t->get_send_amount(j));
			sends.add(send);
		}

		track.add("sends", sends);

		tracks.add(track);
	}

	node.add("tracks", tracks);

	{
		//orders
		JSON::Node orders = JSON::array();
		int max_order = -1;
		for (int i = 0; i <= Song::ORDER_MAX; i++) {
			if (song->order_get(i) != Song::ORDER_EMPTY) {
				max_order = i;
			}
		}
		for (int i = 0; i <= max_order; i++) {
			int order = song->order_get(i);
			if (order == Song::ORDER_SKIP) {
				order = -1; //more readable i guess
			}
			orders.add(order);
		}

		node.add("orderlist", orders);
	}

	{
		//patterns
		JSON::Node patterns = JSON::array();
		for (int i = 0; i < Song::MAX_PATTERN; i++) {

			JSON::Node pattern = JSON::object();
			bool pattern_valid = false;

			JSON::Node tracks = JSON::array();
			for (int j = 0; j < song->get_track_count(); j++) {
				Track *t = song->get_track(j);
				JSON::Node track = JSON::object();
				bool track_valid = false;

				int note_count = t->get_note_count(i);
				if (note_count) {
					JSON::Node notes = JSON::array();
					for (int k = 0; k < note_count; k++) {

						JSON::Node note = JSON::object();
						Track::Pos p = t->get_note_pos_by_index(i, k);

						note.add("tick", (int)p.tick);
						note.add("column", p.column);

						Track::Note n = t->get_note_by_index(i, k);

						if (n.note <= Track::Note::MAX_NOTE) {
							note.add("note", n.note);
						} else if (n.note == Track::Note::OFF) {
							note.add("note", "off");
						}
						if (n.volume != Track::Note::EMPTY) {
							note.add("volume", n.volume);
						}

						notes.add(note);
					}
					track.add("notes", notes);
					track_valid = true;
				}

				int command_count = t->get_command_count(i);
				if (command_count) {
					JSON::Node commands = JSON::array();
					for (int k = 0; k < command_count; k++) {

						JSON::Node command = JSON::object();
						Track::Pos p = t->get_command_pos_by_index(i, k);

						command.add("tick", (int)p.tick);
						command.add("column", p.column);

						Track::Command n = t->get_command_by_index(i, k);

						if (n.command != Track::Command::EMPTY) {
							command.add("command", int(n.command));
						}
						//there is always parameter
						command.add("parameter", n.parameter);

						commands.add(command);
					}
					track.add("commands", commands);
					track_valid = true;
				}

				bool automation_valid = false;
				JSON::Node automations = JSON::array();
				for (int k = 0; k < t->get_automation_count(); k++) {
					Automation *a = t->get_automation(k);
					int point_count = a->get_point_count(i);
					if (point_count > 0) {
						JSON::Node automation = JSON::object();
						automation.add("index", k);
						JSON::Node points = JSON::array();

						for (int l = 0; l < point_count; l++) {

							JSON::Node point = JSON::object();

							Tick t = a->get_point_tick_by_index(i, l);

							point.add("tick", (int)t);

							int value = a->get_point_by_index(i, l);

							point.add("value", value);

							points.add(point);
						}
						automation.add("points", points);
						automation_valid = true;
						automations.add(automation);
					}
				}

				if (automation_valid) {

					track.add("automations", automations);
				}

				if (track_valid || automation_valid) {
					track.add("index", j);
					pattern_valid = true;
					tracks.add(track);
				}
			}

			bool config_not_default = false;

			if (song->pattern_get_beats_per_bar(i) != Song::DEFAULT_BEATS_PER_BAR) {
				config_not_default = true;
			}

			if (song->pattern_get_beats(i) != Song::DEFAULT_PATTERN_BEATS) {
				config_not_default = true;
			}

			if (song->pattern_get_swing_beat_divisor(i) != Song::SWING_BEAT_DIVISOR_1) {
				config_not_default = true;
			}

			if (pattern_valid || config_not_default) {
				pattern.add("index", i);
				pattern.add("tracks", tracks);
				pattern.add("beats", song->pattern_get_beats(i));
				pattern.add("beats_per_bar", song->pattern_get_beats_per_bar(i));
				pattern.add("swing_divisor_index", song->pattern_get_swing_beat_divisor(i));
				patterns.add(pattern);
			}
		}

		node.add("patterns", patterns);
	}

	if (shared_data_map.begin() != shared_data_map.end()) {
		JSON::Node shared = JSON::object();
		for (auto I = shared_data_map.begin(); I != shared_data_map.end(); I++) {
			shared.add(I->first, base64_encode(*I->second));
		}
		node.add("shared_data", shared);
	}

	return save_json(p_path, node);
}

Error SongFile::load(const String &p_path, List<MissingPlugin> *r_missing_plugins) {

	JSON::Node node;
	Error err = load_json(p_path, node);
	if (err != OK) {
		return err;
	}

	if (!node.has("software") || node.get("software").toString() != VERSION_SOFTWARE_NAME) {
		return ERR_FILE_UNRECOGNIZED;
	}

	{
		JSON::Node version = node.get("version");
		if (!version.has("major") || !version.has("minor")) {
			return ERR_FILE_CORRUPT;
		}

		int minor = version.get("minor").toInt();
		int major = version.get("major").toInt();

		if (VERSION_MAJOR * 1000 + VERSION_MINOR < major * 1000 + minor) {
			return ERR_FILE_TOO_NEW;
		}
	}

	if (!node.has("information")) {
		return ERR_FILE_CORRUPT;
	}

	std::map<std::string, std::shared_ptr<Vector<uint8_t> > > shared_data_map;

	if (node.has("shared_data")) {
		JSON::Node n = node.get("shared_data");
		for (auto I = n.begin(); I != n.end(); I++) {
			std::string key = I->first;
			std::string data64 = I->second.toString();
			Vector<uint8_t> data = base64_decode(data64);
			shared_data_map[key] = std::make_shared<Vector<uint8_t> >(data);

			printf("sd: %s\n", key.c_str());
		}
	}

	song->clear();

	{

		JSON::Node information = node.get("information");
		String str;
		str.parse_utf8(information.get("name").toString().c_str());
		song->set_name(str);
		str.parse_utf8(information.get("author").toString().c_str());
		song->set_author(str);
		str.parse_utf8(information.get("description").toString().c_str());
		song->set_description(str);
	}

	if (node.has("tempo")) {
		JSON::Node tempo = node.get("tempo");
		song->set_bpm(tempo.get("bpm").toInt());
		song->set_swing(tempo.get("swing").toFloat() / 100.0);
	}

	JSON::Node tracks = node.get("tracks");

	for (int i = 0; i < tracks.getCount(); i++) {

		JSON::Node track = tracks.get(i);
		Track *t = new Track;
		String name;
		name.parse_utf8(track.get("name").toString().c_str());
		t->set_name(name);
		t->set_mix_volume_db(track.get("volume").toFloat());
		t->set_muted(track.get("muted").toBool());
		int columns = track.get("columns").toInt();
		t->set_columns(MAX(1, columns));
		int command_columns = track.get("command_columns").toInt();
		t->set_command_columns(MAX(0, command_columns));

		JSON::Node effects = track.get("effects");

		for (int j = 0; j < effects.getCount(); j++) {

			JSON::Node effect = effects.get(j);

			String provider_id;
			provider_id.parse_utf8(effect.get("provider_id").toString().c_str());
			String id;
			id.parse_utf8(effect.get("id").toString().c_str());

			int effect_index = -1;
			for (int k = 0; k < fx_factory->get_audio_effect_count(); k++) {
				if (fx_factory->get_audio_effect(k)->provider_id == provider_id && fx_factory->get_audio_effect(k)->unique_ID == id) {
					effect_index = k;
				}
			}

			AudioEffect *fx = NULL;
			if (effect_index != -1) {
				fx = fx_factory->instantiate_effect(effect_index);
			}

			if (!fx) {
				//plugin not found, report and add a dummy
				if (r_missing_plugins) {
					MissingPlugin missing;
					missing.provider = provider_id;
					missing.id = id;
					r_missing_plugins->push_back(missing);
				}
				//create a dummy one to hold data (but does no audio).
				AudioEffectDummy *dummy = new AudioEffectDummy;
				dummy->provider_id = provider_id;
				dummy->unique_id = id;
				//check automations and add relevant ports
				JSON::Node automations = track.get("automations");
				for (int k = 0; k < automations.getCount(); k++) {
					JSON::Node automation = automations.get(k);
					int index = automation.get("effect_index").toInt();
					if (index == j) {
						String parameter;
						parameter.parse_utf8(automation.get("parameter").toString().c_str());
						ControlPortDefault cpdefault;
						cpdefault.identifier = parameter;
						cpdefault.name = parameter;
						dummy->ports.push_back(cpdefault);
					}
				}

				if (effect.has("commands")) {
					JSON::Node commands = effect.get("commands");

					for (JSON::Node::iterator I = commands.begin(); I != commands.end(); I++) {
						printf("ITERATION \n");
						String name;
						name.parse_utf8(I->first.c_str());
						int exists = false;
						for (int k = 0; k < dummy->ports.size(); k++) {
							if (dummy->ports[i].name == name) {
								exists = true;
								break;
							}
						}

						if (!exists) {
							ControlPortDefault cpdefault;
							cpdefault.identifier = name;
							cpdefault.name = name;
							dummy->ports.push_back(cpdefault);
						}
					}
				}
				fx = dummy;
			}

			if (effect.has("commands")) {
				JSON::Node commands = effect.get("commands");

				for (int k = 0; k < fx->get_control_port_count(); k++) {

					std::string cpname = fx->get_control_port(k)->get_identifier().utf8().get_data();
					if (commands.has(cpname)) {
						std::string cs = commands.get(cpname).toString();
						if (cs.length() >= 1) {
							char c = cs[0];
							fx->get_control_port(k)->set_command(c);
						}
					}
				}
			}

			fx->set_skip(effect.get("skip").toBool());
			if (effect.has("shared_data")) {
				std::string sd = effect.get("shared_data").toString();
				if (shared_data_map.find(sd) != shared_data_map.end()) {
					fx->set_shared_data(shared_data_map[sd]);
				}
			}
			fx->from_json(effect.get("state"));
			t->add_audio_effect(fx);
		}

		JSON::Node automations = track.get("automations");

		for (int j = 0; j < automations.getCount(); j++) {

			JSON::Node automation = automations.get(j);

			int effect_index = automation.get("effect_index").toInt();
			String param_name;
			param_name.parse_utf8(automation.get("parameter").toString().c_str());

			ERR_CONTINUE(effect_index < 0 || effect_index >= t->get_audio_effect_count());

			AudioEffect *fx = t->get_audio_effect(effect_index);
			ControlPort *control_port = NULL;

			for (int k = 0; k < fx->get_control_port_count(); k++) {
				if (fx->get_control_port(k)->get_identifier() == param_name) {
					control_port = fx->get_control_port(k);
				}
			}

			ERR_CONTINUE(!control_port);

			Automation *a = new Automation(control_port, fx);

			String edit_mode;
			edit_mode.parse_utf8(automation.get("edit_mode").toString().c_str());
			if (edit_mode == "envelope_large") {
				a->set_edit_mode(Automation::EDIT_ENVELOPE_LARGE);
			} else if (edit_mode == "envelope_small") {
				a->set_edit_mode(Automation::EDIT_ENVELOPE_SMALL);
			} else {
				a->set_edit_mode(Automation::EDIT_ROWS_DISCRETE);
			}

			t->add_automation(a);
		}

		JSON::Node sends = track.get("sends");
		for (int j = 0; j < sends.getCount(); j++) {
			JSON::Node send = sends.get(j);
			t->add_send(send.get("to_track").toInt(), send.get("amount").toFloat());
		}

		song->add_track(t);
	}

	{
		//orders
		JSON::Node orders = node.get("orderlist");

		for (int i = 0; i < orders.getCount(); i++) {
			int order = orders.get(i).toInt();
			if (order < 0) {
				song->order_set(i, Song::ORDER_SKIP);
			} else {
				song->order_set(i, order);
			}
		}
	}

	if (node.has("patterns")) {
		//patterns
		JSON::Node patterns = node.get("patterns");

		for (int i = 0; i < patterns.getCount(); i++) {

			JSON::Node pattern = patterns.get(i);

			ERR_CONTINUE(!pattern.has("index"));
			int index = pattern.get("index").toInt();

			if (pattern.has("beats")) {
				song->pattern_set_beats(index, pattern.get("beats").toInt());
			}

			if (pattern.has("beats_per_bar")) {
				song->pattern_set_beats_per_bar(index, pattern.get("beats_per_bar").toInt());
			}

			if (pattern.has("swing_divisor_index")) {
				song->pattern_set_swing_beat_divisor(index, Song::SwingBeatDivisor(pattern.get("swing_divisor_index").toInt()));
			}

			JSON::Node tracks = pattern.get("tracks");

			for (int j = 0; j < tracks.getCount(); j++) {

				JSON::Node track = tracks.get(j);
				int track_index = track.get("index").toInt();

				ERR_CONTINUE(track_index < 0 || track_index >= song->get_track_count());

				Track *t = song->get_track(track_index);

				if (track.has("notes")) {

					JSON::Node notes = track.get("notes");
					for (int k = 0; k < notes.getCount(); k++) {

						JSON::Node note = notes.get(k);

						Track::Pos p;
						p.tick = note.get("tick").toInt();
						p.column = note.get("column").toInt();

						Track::Note n;
						if (note.has("volume")) {
							n.volume = note.get("volume").toInt();
						} else {
							n.volume = Track::Note::EMPTY;
						}

						if (note.get("note").getType() == JSON::Node::T_STRING) {
							//note off
							n.note = Track::Note::OFF;
						} else {
							n.note = note.get("note").toInt();
						}

						t->set_note(index, p, n);
					}
				}

				if (track.has("commands")) {

					JSON::Node commands = track.get("commands");
					for (int k = 0; k < commands.getCount(); k++) {

						JSON::Node command = commands.get(k);

						Track::Pos p;
						p.tick = command.get("tick").toInt();
						p.column = command.get("column").toInt();

						Track::Command c;
						if (command.has("command")) {
							c.command = command.get("command").toInt();
						} else {
							c.command = Track::Command::EMPTY;
						}

						c.parameter = command.get("parameter").toInt();

						t->set_command(index, p, c);
					}
				}

				if (track.has("automations")) {

					JSON::Node automations = track.get("automations");

					for (int k = 0; k < automations.getCount(); k++) {

						JSON::Node automation = automations.get(k);
						int auto_index = automation.get("index").toInt();
						ERR_CONTINUE(auto_index < 0 || auto_index >= t->get_automation_count());

						Automation *a = t->get_automation(auto_index);

						if (automation.has("points")) {

							JSON::Node points = automation.get("points");

							for (int l = 0; l < points.getCount(); l++) {

								JSON::Node point = points.get(l);

								Tick t = point.get("tick").toInt();
								int value = point.get("value").toInt();

								a->set_point(index, t, value);
							}
						}
					}
				}
			}
		}
	}

	song->update_process_order();

	return OK;
}

//////////////////////////////////////////////////////////////////////////////

Error SongFile::export_wav(const String &p_path, int p_export_hz, ExportWavPatternCallback p_callback, void *p_userdata) {

#ifdef WINDOWS_ENABLED
	FILE *f = _wfopen(p_path.c_str(), L"wb");
#else
	FILE *f = fopen(p_path.utf8().get_data(), "wb");
#endif

	if (!f) {
		return ERR_CANT_OPEN;
	}

	fwrite("RIFF", 4, 1, f);
	uint32_t total_size = 4 /* WAVE */ + 8 /* fmt+size */ + 16 /* format */ + 8 /* data+size */;
	fwrite(&total_size, 4, 1, f);
	fwrite("WAVE", 4, 1, f);
	fwrite("fmt ", 4, 1, f);

	uint32_t format = 16;
	fwrite(&format, 4, 1, f);

	uint16_t compression = 1; //standard pcm
	fwrite(&compression, 2, 1, f);
	uint16_t channels = 2; //stereo
	fwrite(&channels, 2, 1, f);

	uint32_t sampling_rate = p_export_hz;
	fwrite(&sampling_rate, 4, 1, f);

	uint16_t bits_per_sample = 32;
	uint16_t blockalign = bits_per_sample / 8 * (2);
	uint32_t bytes_per_sec = sampling_rate * blockalign;

	fwrite(&bytes_per_sec, 4, 1, f);
	fwrite(&blockalign, 2, 1, f);
	fwrite(&bits_per_sample, 2, 1, f);

	fwrite("data", 4, 1, f);
	uint32_t data_size = 0;
	fwrite(&data_size, 4, 1, f);

	uint32_t data_from = ftell(f);

	//begin save data
	SoundDriverManager::lock_driver();

	uint32_t block_size = 64;
	AudioFrame *block = new AudioFrame[block_size];
	song->set_sampling_rate(sampling_rate);
	song->set_process_buffer_size(block_size);

	song->play(0, 0, false);
	int current_order = -1;
	while (song->is_playing()) {
		song->process_audio(block, block_size);
		for (int i = 0; i < block_size; i++) {
			int32_t l = int32_t(CLAMP(double(block[i].l), -1.0, 1.0) * double(2147483647.0));
			int32_t r = int32_t(CLAMP(double(block[i].r), -1.0, 1.0) * double(2147483647.0));

			fwrite(&l, 4, 1, f);
			fwrite(&r, 4, 1, f);
		}
		int order = song->get_playing_order();
		if (current_order != order) {
			current_order = order;
			printf("order: %i\n", current_order);
			if (p_callback) {
				p_callback(current_order, p_userdata);
			}
		}
	}
	song->stop();

	delete[] block;
	//restore
	song->set_sampling_rate(SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::get_mix_frequency()));
	song->set_process_buffer_size(SoundDriverManager::get_buffer_size_frames(SoundDriverManager::get_step_buffer_size()));

	SoundDriverManager::unlock_driver();

	//end save data

	data_size = ftell(f) - data_from;

	fseek(f, 4, SEEK_SET);
	total_size += data_size;
	fwrite(&total_size, 4, 1, f);

	fseek(f, 0x28, SEEK_SET);
	fwrite(&data_size, 4, 1, f);

	fclose(f);

	return OK;
}

SongFile::SongFile(Song *p_song, AudioEffectFactory *p_fx_factory) {
	song = p_song;
	fx_factory = p_fx_factory;
}
