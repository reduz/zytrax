#include "midi_driver_manager.h"

void MIDIInputDriver::event(double p_stamp, const MIDIEvent &p_event) {
	if (MIDIDriverManager::event_callback) {
		MIDIDriverManager::event_callback(p_stamp, p_event);
	}
}

///////////////////////

MIDIInputDriver *MIDIDriverManager::input_drivers[MIDIDriverManager::MAX_MIDI_DRIVERS];
int MIDIDriverManager::input_driver_count = 0;
int MIDIDriverManager::input_current_driver = -1;

MIDIDriverManager::EventCallback MIDIDriverManager::event_callback = NULL;

void MIDIDriverManager::lock_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->lock();
	}
}
void MIDIDriverManager::unlock_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->unlock();
	}
}

bool MIDIDriverManager::init_input_driver(int p_driver) {
	ERR_FAIL_COND_V(p_driver != -1 && p_driver < 0 && p_driver >= input_driver_count, false);

	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->finish();
	}

	input_current_driver = p_driver;

	if (input_current_driver != -1) {
		return input_drivers[input_current_driver]->init();
	}

	return false;
}
void MIDIDriverManager::finish_input_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->finish();
	}
}
bool MIDIDriverManager::is_input_driver_active() {
	return (input_current_driver != -1 && input_drivers[input_current_driver]->is_active());
}
int MIDIDriverManager::get_input_driver_count() {
	return input_driver_count;
}
MIDIInputDriver *MIDIDriverManager::get_input_driver(int p_which) {
	ERR_FAIL_INDEX_V(p_which, input_driver_count, NULL);
	return input_drivers[p_which];
}

int MIDIDriverManager::get_current_input_driver_index() {
	return input_current_driver;
}

void MIDIDriverManager::add_input_driver(MIDIInputDriver *p_driver) {
	ERR_FAIL_COND(input_driver_count == MAX_MIDI_DRIVERS);
	input_drivers[input_driver_count++] = p_driver;
}
void MIDIDriverManager::set_event_callback(EventCallback p_callback) {
	event_callback = p_callback;
}

///////////////


MIDIBankManager::DeviceFile MIDIBankManager::device_files[MIDIBankManager::MAX_DEVICE_FILES];
Vector<MIDIBankManager::Bank> MIDIBankManager::banks;

void MIDIBankManager::set_device_file_path(int p_device_file,String p_path) {
	ERR_FAIL_INDEX(p_device_file,MAX_DEVICE_FILES);
	device_files[p_device_file].path = p_path;

	p_path.replace("\\","/");
	int last_idx = p_path.find_last("/");
	if (last_idx != -1) {
		p_path = p_path.substr(last_idx+1,p_path.length());
	}
	int dot_idx = p_path.find(".");
	if (dot_idx != -1) {
		p_path = p_path.substr(0,dot_idx);
	}

	device_files[p_device_file].name = p_path;

}

String MIDIBankManager::get_device_file_path(int p_device_file) {
	if (p_device_file==-1) {
		return "";
	}
	return device_files[p_device_file].path;
}

String MIDIBankManager::get_device_file_name(int p_device_file) {
	if (p_device_file==-1) {
		return GM_DEVICE_NAME;
	}
	return device_files[p_device_file].name;
}

void MIDIBankManager::_load_device_file(int p_index, String p_file) {

	// Thankfully this format is very simple
#ifdef WINDOWS_ENABLED
	FILE *f = _wfopen(p_file.c_str(), L"rb");
#else
	FILE *f = fopen(p_file.utf8().get_data(), "rb");
#endif
	if (!f) {
		fprintf(stderr,"Cant open MIDI device banks file at path %s\n",p_file.utf8().get_data());
		return;
	}

	Vector<uint8_t> accum;
	String bank_name = "default";
	int bank_index=-1;
	bool bank_empty = true;
	bool zerobased = false;
	bool reading_nrpn = false;

	while(!feof(f)) {
		uint8_t c = fgetc(f);
		if (c==0xD) {
			continue;
		}
		if (c=='\n') {
			accum.push_back(0);
			String s;
			s.parse_utf8((const char*)&accum[0]);
			accum.clear();
			s = s.strip_edges();

			int semicolon = s.find(";");
			if (semicolon!=-1) {
				s=s.substr(0,semicolon).strip_edges(); // Remove semicolon.
			}

			if (s=="") {
				// Empty line.
				continue;
			}

			if (s.to_upper()=="ZEROBASED") {
				zerobased = true;
				continue;
			}

			int bracket_open = s.find("[");
			if (bracket_open != -1) {
				int bracket_close = s.find("]");
				if (bracket_close!=-1) {					
					bank_name=s.substr(bracket_open+1,bracket_close-bracket_open-1).strip_edges();
					if (bank_name.to_lower()=="nrpn") {
						reading_nrpn=true;
					}
					bank_empty=true;
				}
				continue;
			}

			int equals = s.find("=");
			if (equals!=-1) {

				if (reading_nrpn) {
					String nrpn = s.get_slice("=",0).strip_edges().to_lower();
					nrpn.replace(" ","");
					if (nrpn.length() >=4 ) {
						NRPN n;
						char v = 0;
						for(int i=0;i<4;i++) {
							char c= nrpn[i];
							char nibble;
							if (c>='0' && c<='9') {
								nibble = c - '0';
							} else if (c>='a' && c<='f') {
								nibble = 10 + (c - 'a');
							}

							switch(i) {
								case 0: v = nibble << 4; break;
								case 1: v |= nibble; n.msb=nibble; break;
								case 2: v = nibble << 4; break;
								case 3: v |= nibble; n.lsb=nibble; break;
							}
						}

						n.name = s.get_slice("=",1).strip_edges();
						n.device_name = device_files[p_index].name;
						nrpn_list.push_back(n);
					}
				} else {
					String number = s.get_slice("=",0).strip_edges();

					int patch_index=0;
					int bank_msb=0;
					int bank_lsb=0;

					int nc = number.get_slice_count(".");
					if (nc>=1) {
						patch_index = number.get_slice(".",0).to_int();
						if (!zerobased) {
							patch_index--;
						}
					}
					if (nc>=2) {
						bank_msb = number.get_slice(".",1).to_int();
					}
					if (nc>=3) {
						bank_lsb = number.get_slice(".",2).to_int();
					}

					String name = s.get_slice("=",1).strip_edges();

					if (bank_empty) {
						Bank bank;
						bank.device_name=device_files[p_index].name;
						bank.name = bank_name;
						bank_index=banks.size();
						banks.push_back(bank);
						bank_empty=false;
					}

					Bank::Patch p;
					p.index=patch_index;
					p.lsb=bank_lsb;
					p.msb=bank_msb;
					p.name=name;
					banks[bank_index].patch_list.push_back(p);
				}

				//printf("Device: %s - Bank: %s - MSB: %i - LSB: %i - Patch (%i): %s\n",banks[bank_index].device_name.utf8().get_data(),banks[bank_index].name.utf8().get_data(),p.msb,p.lsb,p.index,p.name.utf8().get_data());
			}

		} else {
			accum.push_back(c);
		}
	}

	fclose(f);
}

void MIDIBankManager::reload_devices() {

	banks.clear();

	// Add the General MIDI bank

	{
		const char* general_midi_patches[] = {
		    "Acoustic Grand Piano",
		    "Bright Acoustic Piano",
		    "Electric Grand Piano",
		    "Honky-tonk Piano",
		    "Electric Piano 1",
		    "Electric Piano 2",
		    "Harpsichord",
		    "Clavi",
		    "Celesta",
		    "Glockenspiel",
		    "Music Box",
		    "Vibraphone",
		    "Marimba",
		    "Xylophone",
		    "Tubular Bells",
		    "Dulcimer",
		    "Drawbar Organ",
		    "Percussive Organ",
		    "Rock Organ",
		    "Church Organ",
		    "Reed Organ",
		    "Accordion",
		    "Harmonica",
		    "Tango Accordion",
		    "Acoustic Guitar (nylon)",
		    "Acoustic Guitar (steel)",
		    "Electric Guitar (jazz)",
		    "Electric Guitar (clean)",
		    "Electric Guitar (muted)",
		    "Overdriven Guitar",
		    "Distortion Guitar",
		    "Guitar Harmonics",
		    "Acoustic Bass",
		    "Electric Bass (finger)",
		    "Electric Bass (pick)",
		    "Fretless Bass",
		    "Slap Bass 1",
		    "Slap Bass 2",
		    "Synth Bass 1",
		    "Synth Bass 2",
		    "Violin",
		    "Viola",
		    "Cello",
		    "Contrabass",
		    "Tremolo Strings",
		    "Pizzicato Strings",
		    "Orchestral Harp",
		    "Timpani",
		    "String Ensemble 1",
		    "String Ensemble 2",
		    "SynthStrings 1",
		    "SynthStrings 2",
		    "Choir Aahs",
		    "Voice Oohs",
		    "Synth Voice",
		    "Orchestra Hit",
		    "Trumpet",
		    "Trombone",
		    "Tuba",
		    "Muted Trumpet",
		    "French Horn",
		    "Brass Section",
		    "SynthBrass 1",
		    "SynthBrass 2",
		    "Soprano Sax",
		    "Alto Sax",
		    "Tenor Sax",
		    "Baritone Sax",
		    "Oboe",
		    "English Horn",
		    "Bassoon",
		    "Clarinet",
		    "Piccolo",
		    "Flute",
		    "Recorder",
		    "Pan Flute",
		    "Blown Bottle",
		    "Shakuhachi",
		    "Whistle",
		    "Ocarina",
		    "Lead 1 (square)",
		    "Lead 2 (sawtooth)",
		    "Lead 3 (calliope)",
		    "Lead 4 (chiff)",
		    "Lead 5 (charang)",
		    "Lead 6 (voice)",
		    "Lead 7 (fifths)",
		    "Lead 8 (bass + lead)",
		    "Pad 1 (new age)",
		    "Pad 2 (warm)",
		    "Pad 3 (polysynth)",
		    "Pad 4 (choir)",
		    "Pad 5 (bowed)",
		    "Pad 6 (metallic)",
		    "Pad 7 (halo)",
		    "Pad 8 (sweep)",
		    "FX 1 (rain)",
		    "FX 2 (soundtrack)",
		    "FX 3 (crystal)",
		    "FX 4 (atmosphere)",
		    "FX 5 (brightness)",
		    "FX 6 (goblins)",
		    "FX 7 (echoes)",
		    "FX 8 (sci-fi)",
		    "Sitar",
		    "Banjo",
		    "Shamisen",
		    "Koto",
		    "Kalimba",
		    "Bagpipe",
		    "Fiddle",
		    "Shanai",
		    "Tinkle Bell",
		    "Agogo",
		    "Steel Drums",
		    "Woodblock",
		    "Taiko Drum",
		    "Melodic Tom",
		    "Synth Drum",
		    "Reverse Cymbal",
		    "Guitar Fret Noise",
		    "Breath Noise",
		    "Seashore",
		    "Bird Tweet",
		    "Telephone Ring",
		    "Helicopter",
		    "Applause",
		    "Gunshot"
		};

		Bank gm_bank;
		gm_bank.name="GM";
		gm_bank.device_name=GM_DEVICE_NAME;
		for(int i=0;i<128;i++) {
			Bank::Patch p;
			p.index=i;
			p.name=general_midi_patches[i];
			p.lsb=0;
			p.msb=0;
			gm_bank.patch_list.push_back(p);
		}

		banks.push_back(gm_bank);

	}

	for(int i=0;i<MAX_DEVICE_FILES;i++) {
		if (device_files[i].path!="") {
			_load_device_file(i,device_files[i].path);
		}
	}
}


int MIDIBankManager::get_bank_count() {
	return banks.size();
}

String MIDIBankManager::get_bank_device(int p_bank) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),String());
	return banks[p_bank].device_name;
}

String MIDIBankManager::get_bank_name(int p_bank) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),String());
	return banks[p_bank].name;
}


int MIDIBankManager::get_bank_patch_count(int p_bank) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),0);
	return banks[p_bank].patch_list.size();
}

String MIDIBankManager::get_bank_patch_name(int p_bank,int p_patch) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),String());
	ERR_FAIL_INDEX_V(p_patch,banks[p_bank].patch_list.size(),String());
	return banks[p_bank].patch_list[p_patch].name;
}

int MIDIBankManager::get_bank_patch_index(int p_bank,int p_patch) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),0);
	ERR_FAIL_INDEX_V(p_patch,banks[p_bank].patch_list.size(),0);
	return banks[p_bank].patch_list[p_patch].index;
}

int MIDIBankManager::get_bank_patch_msb(int p_bank,int p_patch) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),0);
	ERR_FAIL_INDEX_V(p_patch,banks[p_bank].patch_list.size(),0);
	return banks[p_bank].patch_list[p_patch].msb;
}

int MIDIBankManager::get_bank_patch_lsb(int p_bank,int p_patch) {
	ERR_FAIL_INDEX_V(p_bank,banks.size(),0);
	ERR_FAIL_INDEX_V(p_patch,banks[p_bank].patch_list.size(),0);
	return banks[p_bank].patch_list[p_patch].lsb;
}

int MIDIBankManager::get_nrpn_count() {
	return nrpn_list.size();
}

int MIDIBankManager::get_nrpn_msb(int p_index) {
	ERR_FAIL_INDEX_V(p_index,nrpn_list.size(),0);
	return nrpn_list[p_index].msb;
}

int MIDIBankManager::get_nrpn_lsb(int p_index) {
	ERR_FAIL_INDEX_V(p_index,nrpn_list.size(),0);
	return nrpn_list[p_index].lsb;

}

String MIDIBankManager::get_nrpn_name(int p_index) {
	ERR_FAIL_INDEX_V(p_index,nrpn_list.size(),String());
	return nrpn_list[p_index].name;

}

String MIDIBankManager::get_nrpn_device_name(int p_index) {
	ERR_FAIL_INDEX_V(p_index,nrpn_list.size(),String());
	return nrpn_list[p_index].device_name;
}

Vector<MIDIBankManager::NRPN> MIDIBankManager::nrpn_list;


std::set<std::string> MIDIBankManager::favorites;

void MIDIBankManager::set_favorite(std::string p_device,std::string p_bank,int p_msb, int p_lsb, int p_patch_index,bool p_favorite) {
	std::string fav_str = p_device+"::"+p_bank+"::"+std::to_string(p_msb)+"::"+std::to_string(p_lsb)+"::"+std::to_string(p_patch_index);
	if (p_favorite) {
		favorites.insert(fav_str);
	} else {
		favorites.erase(fav_str);
	}
}

bool MIDIBankManager::is_favorite(std::string p_device,std::string p_bank,int p_msb, int p_lsb, int p_patch_index) {
	std::string fav_str = p_device+"::"+p_bank+"::"+std::to_string(p_msb)+"::"+std::to_string(p_lsb)+"::"+std::to_string(p_patch_index);
	bool fav = favorites.find(fav_str)!=favorites.end();
	return fav;
}
