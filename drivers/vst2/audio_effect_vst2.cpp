#include "audio_effect_vst2.h"
#include "audio_effect_provider_vst2.h"

int AudioEffectVST2::_get_internal_control_port_count() const {
	return control_ports.size();
}

ControlPort *AudioEffectVST2::_get_internal_control_port(int p_index) {

	return &control_ports[p_index];
}

bool AudioEffectVST2::has_secondary_input() const {
	return false;
}

void AudioEffectVST2::process(const Event *p_events, int p_event_count, const Frame *p_in, Frame *p_out, bool p_prev_active) {
}
void AudioEffectVST2::process_with_secondary(const Event *p_events, int p_event_count, const Frame *p_in, const Frame *p_secondary, Frame *p_out, bool p_prev_active) {
}

String AudioEffectVST2::get_name() const {
	return name;
}
String AudioEffectVST2::get_unique_id() const {
	return unique_id;
}

String AudioEffectVST2::get_provider_id() const {
	return provider_id;
}

void AudioEffectVST2::reset() {
}

bool AudioEffectVST2::has_user_interface() const {
	return (effect->flags & effFlagsHasEditor);
}
void AudioEffectVST2::get_user_interface_size(int &r_width, int &r_height) {

	ERect *rect = NULL;
	effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0);
	ERR_FAIL_COND(!rect);
	r_width = rect->right - rect->left;
	r_height = rect->bottom - rect->top;
}

void AudioEffectVST2::process_user_interface() {

	effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0);
}

void AudioEffectVST2::open_user_interface(void *p_window_ptr) {

	effect->dispatcher(effect, effEditOpen, 0, 0, p_window_ptr, 0);
}

void AudioEffectVST2::resize_user_interface(int p_width, int p_height) {

	//fst->amc (fst->plugin, 15 /*audioMasterSizeWindow */, width, height, NULL, 0);
	effect->dispatcher(effect, audioMasterSizeWindow, p_width, p_height, NULL, 0);
}

void AudioEffectVST2::close_user_interface() {
	effect->dispatcher(effect, effEditClose, 0, 0, NULL, 0);
}

#define DEBUG_CALLBACK(m_text) printf("VST Callback: %s\n", m_text)

VstIntPtr VSTCALLBACK AudioEffectVST2::host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {

	//simple host for exploring plugin
	AudioEffectVST2 *vst_effect = effect ? (AudioEffectVST2 *)effect->resvd1 : 0;

	switch (opcode) {
		//VST 1.0 opcodes
		case audioMasterAutomate:
			DEBUG_CALLBACK("audioMasterAutomate");
			// index, value, returns 0
			if (vst_effect) {
				//update automation on host
				//plug->parameter_changed_externally (index, opt);
			}
			return 0;

		case audioMasterVersion:
			DEBUG_CALLBACK("audioMasterVersion");
			// vst version, currently 2 (0 for older)
			return 2400;

		case audioMasterCurrentId:
			DEBUG_CALLBACK("audioMasterCurrentId");
			// returns the unique id of a plug that's currently loading
			return 0;

		case audioMasterIdle:
			DEBUG_CALLBACK("audioMasterIdle");
#ifdef WINDOWS_VST_SUPPORT
			fst_audio_master_idle();
#endif
			if (vst_effect) {
				vst_effect->process_user_interface();
			}
			return 0;

		case DECLARE_VST_DEPRECATED(audioMasterWantMidi):
			DEBUG_CALLBACK("audioMasterWantMidi");
			return 1;
#if 0
		case audioMasterGetTime:
			DEBUG_CALLBACK ("audioMasterGetTime");
			newflags = kVstNanosValid | kVstAutomationWriting | kVstAutomationReading;

			timeinfo->nanoSeconds = g_get_monotonic_time () * 1000;

			if (plug && session) {
				samplepos_t now = plug->transport_sample();

				timeinfo->samplePos = now;
				timeinfo->sampleRate = session->sample_rate();

				if (value & (kVstTempoValid)) {
					const Tempo& t (session->tempo_map().tempo_at_sample (now));
					timeinfo->tempo = t.quarter_notes_per_minute ();
					newflags |= (kVstTempoValid);
				}
				if (value & (kVstTimeSigValid)) {
					const MeterSection& ms (session->tempo_map().meter_section_at_sample (now));
					timeinfo->timeSigNumerator = ms.divisions_per_bar ();
					timeinfo->timeSigDenominator = ms.note_divisor ();
					newflags |= (kVstTimeSigValid);
				}
				if ((value & (kVstPpqPosValid)) || (value & (kVstBarsValid))) {
					Timecode::BBT_Time bbt;

					try {
						bbt = session->tempo_map().bbt_at_sample_rt (now);
						bbt.beats = 1;
						bbt.ticks = 0;
						/* exact quarter note */
						double ppqBar = session->tempo_map().quarter_note_at_bbt_rt (bbt);
						/* quarter note at sample position (not rounded to note subdivision) */
						double ppqPos = session->tempo_map().quarter_note_at_sample_rt (now);
						if (value & (kVstPpqPosValid)) {
							timeinfo->ppqPos = ppqPos;
							newflags |= kVstPpqPosValid;
						}

						if (value & (kVstBarsValid)) {
							timeinfo->barStartPos = ppqBar;
							newflags |= kVstBarsValid;
						}

					} catch (...) {
						/* relax */
					}
				}

				if (value & (kVstSmpteValid)) {
					Timecode::Time t;

					session->timecode_time (now, t);

					timeinfo->smpteOffset = (t.hours * t.rate * 60.0 * 60.0) +
						(t.minutes * t.rate * 60.0) +
						(t.seconds * t.rate) +
						(t.frames) +
						(t.subframes);

					timeinfo->smpteOffset *= 80.0; /* VST spec is 1/80th samples */

					if (session->timecode_drop_frames()) {
						if (session->timecode_frames_per_second() == 30.0) {
							timeinfo->smpteFrameRate = 5;
						} else {
							timeinfo->smpteFrameRate = 4; /* 29.97 assumed, thanks VST */
						}
					} else {
						if (session->timecode_frames_per_second() == 24.0) {
							timeinfo->smpteFrameRate = 0;
						} else if (session->timecode_frames_per_second() == 24.975) {
							timeinfo->smpteFrameRate = 2;
						} else if (session->timecode_frames_per_second() == 25.0) {
							timeinfo->smpteFrameRate = 1;
						} else {
							timeinfo->smpteFrameRate = 3; /* 30 fps */
						}
					}
					newflags |= (kVstSmpteValid);
				}

				if (session->actively_recording ()) {
					newflags |= kVstTransportRecording;
				}

				if (plug->transport_speed () != 0.0f) {
					newflags |= kVstTransportPlaying;
				}

				if (session->get_play_loop ()) {
					newflags |= kVstTransportCycleActive;
					Location * looploc = session->locations ()->auto_loop_location ();
					if (looploc) try {
						timeinfo->cycleStartPos = session->tempo_map ().quarter_note_at_sample_rt (looploc->start ());
						timeinfo->cycleEndPos = session->tempo_map ().quarter_note_at_sample_rt (looploc->end ());

						newflags |= kVstCyclePosValid;
					} catch (...) { }
				}

			} else {
				timeinfo->samplePos = 0;
				timeinfo->sampleRate = AudioEngine::instance()->sample_rate();
			}

			if ((timeinfo->flags & (kVstTransportPlaying | kVstTransportRecording | kVstTransportCycleActive))
			    !=
			    (newflags        & (kVstTransportPlaying | kVstTransportRecording | kVstTransportCycleActive)))
			{
				newflags |= kVstTransportChanged;
			}

			timeinfo->flags = newflags;
			return (intptr_t) timeinfo;
#endif
		case audioMasterProcessEvents:
			DEBUG_CALLBACK("audioMasterProcessEvents");
#if 0
			// VstEvents* in <ptr>
			if (plug && plug->midi_buffer()) {
				VstEvents* v = (VstEvents*)ptr;
				for (int n = 0 ; n < v->numEvents; ++n) {
					VstMidiEvent *vme = (VstMidiEvent*) (v->events[n]->dump);
					if (vme->type == kVstMidiType) {
						plug->midi_buffer()->push_back(vme->deltaSamples, 3, (uint8_t*)vme->midiData);
					}
				}
			}
#endif
			return 0;
#if 0
		case audioMasterSetTime:
			DEBUG_CALLBACK ("audioMasterSetTime");
			// VstTimenfo* in <ptr>, filter in <value>, not supported
			return 0;

		case audioMasterTempoAt:
			DEBUG_CALLBACK ("audioMasterTempoAt");
			// returns tempo (in bpm * 10000) at sample sample location passed in <value>
			if (session) {
				const Tempo& t (session->tempo_map().tempo_at_sample (value));
				return t.quarter_notes_per_minute() * 1000;
			} else {
				return 0;
			}
			break;

		case audioMasterGetNumAutomatableParameters:
			DEBUG_CALLBACK ("audioMasterGetNumAutomatableParameters");
			return 0;

		case audioMasterGetParameterQuantization:
			DEBUG_CALLBACK ("audioMasterGetParameterQuantization");
			// returns the integer value for +1.0 representation,
			// or 1 if full single float precision is maintained
			// in automation. parameter index in <value> (-1: all, any)
			return 0;
#endif
		case audioMasterIOChanged:
			DEBUG_CALLBACK("audioMasterIOChanged");
			// numInputs and/or numOutputs has changed
			return 0;
#if 0
		case audioMasterNeedIdle:
			DEBUG_CALLBACK ("audioMasterNeedIdle");
			// plug needs idle calls (outside its editor window)
			if (plug) {
				plug->state()->wantIdle = 1;
			}
			return 0;
#endif
		case audioMasterSizeWindow:
			DEBUG_CALLBACK("audioMasterSizeWindow");
			if (vst_effect) {
				int w = index;
				int h = value;
				if (vst_effect->resize_callback) {
					vst_effect->resize_callback(vst_effect->resize_userdata, w, h);
				}
				printf("sizewindow %i,%i\n", w, h);

				return 1;
			}

		case audioMasterGetSampleRate:
			DEBUG_CALLBACK("audioMasterGetSampleRate");
			if (vst_effect) {
				return 44100;
			}
			return 0;

		case audioMasterGetBlockSize:
			DEBUG_CALLBACK("audioMasterGetBlockSize");
			if (vst_effect) {
				return 128;
			}
			return 0;

		case audioMasterGetInputLatency:
			DEBUG_CALLBACK("audioMasterGetInputLatency");
			return 0;

		case audioMasterGetOutputLatency:
			DEBUG_CALLBACK("audioMasterGetOutputLatency");
			return 0;
#if 0
		case audioMasterGetPreviousPlug:
			DEBUG_CALLBACK ("audioMasterGetPreviousPlug");
			// input pin in <value> (-1: first to come), returns cEffect*
			return 0;

		case audioMasterGetNextPlug:
			DEBUG_CALLBACK ("audioMasterGetNextPlug");
			// output pin in <value> (-1: first to come), returns cEffect*
			return 0;

		case audioMasterWillReplaceOrAccumulate:
			DEBUG_CALLBACK ("audioMasterWillReplaceOrAccumulate");
			// returns: 0: not supported, 1: replace, 2: accumulate
			return 0;
#endif
		case audioMasterGetCurrentProcessLevel:
			DEBUG_CALLBACK("audioMasterGetCurrentProcessLevel");
			// returns: 0: not supported,
			// 1: currently in user thread (gui)
			// 2: currently in audio thread (where process is called)
			// 3: currently in 'sequencer' thread (midi, timer etc)
			// 4: currently offline processing and thus in user thread
			// other: not defined, but probably pre-empting user thread.
			return 0;

		case audioMasterGetAutomationState:
			DEBUG_CALLBACK("audioMasterGetAutomationState");
			// returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
			// offline
			return 0;

		case audioMasterOfflineStart:
			DEBUG_CALLBACK("audioMasterOfflineStart");
			return 0;

		case audioMasterOfflineRead:
			DEBUG_CALLBACK("audioMasterOfflineRead");
			// ptr points to offline structure, see below. return 0: error, 1 ok
			return 0;

		case audioMasterOfflineWrite:
			DEBUG_CALLBACK("audioMasterOfflineWrite");
			// same as read
			return 0;

		case audioMasterOfflineGetCurrentPass:
			DEBUG_CALLBACK("audioMasterOfflineGetCurrentPass");
			return 0;

		case audioMasterOfflineGetCurrentMetaPass:
			DEBUG_CALLBACK("audioMasterOfflineGetCurrentMetaPass");
			return 0;
#if 0
		case audioMasterSetOutputSampleRate:
			DEBUG_CALLBACK ("audioMasterSetOutputSampleRate");
			// for variable i/o, sample rate in <opt>
			return 0;

		case audioMasterGetSpeakerArrangement:
			DEBUG_CALLBACK ("audioMasterGetSpeakerArrangement");
			// (long)input in <value>, output in <ptr>
			return 0;
#endif
		case audioMasterGetVendorString:
			DEBUG_CALLBACK("audioMasterGetVendorString");
			// fills <ptr> with a string identifying the vendor (max 64 char)
			strcpy((char *)ptr, "ZyTrax");
			return 1;

		case audioMasterGetProductString:
			DEBUG_CALLBACK("audioMasterGetProductString");
			// fills <ptr> with a string with product name (max 64 char)
			strcpy((char *)ptr, "ZyTrax");
			return 1;

		case audioMasterGetVendorVersion:
			DEBUG_CALLBACK("audioMasterGetVendorVersion");
			// returns vendor-specific version
			return 900;

		case audioMasterVendorSpecific:
			DEBUG_CALLBACK("audioMasterVendorSpecific");
			// no definition, vendor specific handling
			return 0;
#if 0
		case audioMasterSetIcon:
			DEBUG_CALLBACK ("audioMasterSetIcon");
			// void* in <ptr>, format not defined yet
			return 0;
#endif
		case audioMasterCanDo:
			DEBUG_CALLBACK("audioMasterCanDo");
			if (strcmp((char *)ptr, "supplyIdle") == 0 ||
					strcmp((char *)ptr, "sendVstTimeInfo") == 0 ||
					strcmp((char *)ptr, "sendVstEvents") == 0 ||
					strcmp((char *)ptr, "sendVstMidiEvent") == 0 ||
					strcmp((char *)ptr, "sizeWindow") == 0) {
				return 1;
			} else {
				return 0;
			}

		case audioMasterGetLanguage:
			DEBUG_CALLBACK("audioMasterGetLanguage");
			// see enum
			return kVstLangEnglish;
#if 0
		case audioMasterOpenWindow:
			DEBUG_CALLBACK ("audioMasterOpenWindow");
			// returns platform specific ptr
			return 0;

		case audioMasterCloseWindow:
			DEBUG_CALLBACK ("audioMasterCloseWindow");
			// close window, platform specific handle in <ptr>
			return 0;
#endif
		case audioMasterGetDirectory:
			DEBUG_CALLBACK("audioMasterGetDirectory");
			// get plug directory, FSSpec on MAC, else char*
			return 0;

		case audioMasterUpdateDisplay:
			DEBUG_CALLBACK("audioMasterUpdateDisplay");
			// something has changed, update 'multi-fx' display
			if (vst_effect) {
				//
			}
			return 0;

		case audioMasterBeginEdit:
			DEBUG_CALLBACK("audioMasterBeginEdit");
			// begin of automation session (when mouse down), parameter index in <index>
			return 0;

		case audioMasterEndEdit:
			DEBUG_CALLBACK("audioMasterEndEdit");
			// end of automation session (when mouse up),     parameter index in <index>
			return 0;

		case audioMasterOpenFileSelector:
			DEBUG_CALLBACK("audioMasterOpenFileSelector");
			// open a fileselector window with VstFileSelect* in <ptr>
			return 0;

		default:
			printf("Unhandled in dispatcher: %i\n", opcode);

			break;
	}

	return 0;
}

/* Load/Save */

JSON::Node AudioEffectVST2::to_json() const {
	return JSON::object();
}
Error AudioEffectVST2::from_json(const JSON::Node &node) {
	return OK;
}

String AudioEffectVST2::get_path() const {
	return path;
}

float AudioEffectVST2::ControlPortVST2::get() const {
	return effect->getParameter(effect, index);
}
void AudioEffectVST2::ControlPortVST2::set(float p_val, bool p_make_initial) {
	effect->setParameter(effect, index, p_val);
	if (p_make_initial) {
		initial = p_val;
	}
}
String AudioEffectVST2::ControlPortVST2::get_value_as_text() const {
	char label[kVstMaxLabelLen + 1];
	effect->dispatcher(effect, effGetParamDisplay, index, 0, label, 0);
	label[kVstMaxLabelLen] = 0;
	String s;
	s.parse_utf8(label);
	return s + label;
}

Error AudioEffectVST2::open(const String &p_path, const String &p_unique_id, const String &p_name, const String &p_provider_id) {
	name = p_name;
	unique_id = p_unique_id;
	path = p_path;
	provider_id = p_provider_id;
	libhandle = LoadLibraryW(p_path.c_str());
	effect = AudioEffectProviderVST2::open_vst_from_lib_handle(libhandle, host);
	if (!effect) {
		return ERR_CANT_OPEN;
	}
	effect->resvd1 = (VstIntPtr)this;
	effect->dispatcher(effect, effOpen, 0, 0, NULL, 0.0f);
	vst_version = effect->dispatcher(effect, effGetVstVersion, 0, 0, NULL, 0.0f);
	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);

	control_ports.resize(effect->numParams);
	for (int i = 0; i < effect->numParams; i++) {
		ControlPortVST2 *cp = &control_ports[i];
		cp->visible = true; //bleh just allow all - i < 100; //the first 50 are visible, the rest are not
		cp->index = i;
		cp->effect = effect;
		char label[kVstMaxLabelLen + 1];
		effect->dispatcher(effect, effGetParamLabel, i, 0, label, 0); //just crap
		label[kVstMaxLabelLen] = 0;
		cp->label.parse_utf8(label);
		cp->label = " " + cp->label;
		effect->dispatcher(effect, effGetParamName, i, 0, label, 0); //just crap
		label[kVstMaxLabelLen] = 0;
		cp->name.parse_utf8(label);
		cp->identifier = "vst_param_" + String::num(i);
		float value = effect->getParameter(effect, i);
		cp->value = value;
		cp->initial = value;
	}
	effect->dispatcher(effect, effSetSampleRate, 0, 0, NULL, 44100); //just crap
	effect->dispatcher(effect, effSetBlockSize, 0, 128, NULL, 0.0f);
	effect->dispatcher(effect, effSetProcessPrecision, 0, kVstProcessPrecision32, NULL, 0.0f);
	effect->dispatcher(effect, effMainsChanged, 0, 1, NULL, 0.0f);
	effect->dispatcher(effect, effStartProcess, 0, 0, NULL, 0.0f);
	return OK;
}

void AudioEffectVST2::set_resize_callback(ResizeCallback p_callback, void *p_userdata) {
	resize_callback = p_callback;
	resize_userdata = p_userdata;
}
AudioEffectVST2::AudioEffectVST2() {

	libhandle = NULL;
	effect = NULL;
	resize_callback = NULL;
	resize_userdata = NULL;
}

AudioEffectVST2::~AudioEffectVST2() {
	if (effect) {
		effect->dispatcher(effect, effClose, 0, 0, NULL, 0.0f);
	}
	if (libhandle) {
		FreeLibrary(libhandle);
	}
}
