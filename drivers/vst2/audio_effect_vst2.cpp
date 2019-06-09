#include "audio_effect_vst2.h"
#include "audio_effect_provider_vst2.h"
#include "base64.h"

int AudioEffectVST2::_get_internal_control_port_count() const {
	return control_ports.size();
}

ControlPort *AudioEffectVST2::_get_internal_control_port(int p_index) {

	return &control_ports[p_index];
}

bool AudioEffectVST2::has_secondary_input() const {
	return has_side_input;
}

void AudioEffectVST2::_process(const Event *p_events, int p_event_count) {

	if (effect->flags & effFlagsIsSynth) {

		//pass time to midi
		float time = float(process_block_size) / sampling_rate;
		//convert input events to actual MIDI events
		int midi_event_count;
		const MIDIEventStamped *midi_events = _process_midi_events(p_events, p_event_count, time, midi_event_count);

		event_pointers->numEvents = 0;

		if (stop_all_notes) {
			for (int i = 0; i < 127; i++) {

				//send noteoff for every channel
				VstMidiEvent &vstem = event_array[event_pointers->numEvents];
				vstem.midiData[0] = 0x80 | last_midi_channel;
				vstem.midiData[1] = i;
				vstem.midiData[2] = 127;
				vstem.midiData[3] = 0;
				event_pointers->numEvents++;
			}
			{
				//send all notes off
				VstMidiEvent &vstem = event_array[event_pointers->numEvents];
				vstem.midiData[0] = 0xB0 | last_midi_channel;
				vstem.midiData[1] = 0x7B; //all notes off
				vstem.midiData[2] = 0;
				vstem.midiData[3] = 0;
				vstem.deltaFrames = 0;
				event_pointers->numEvents++;
			}
			{
				//send reset controllers
				VstMidiEvent &vstem = event_array[event_pointers->numEvents];
				vstem.midiData[0] = 0xB0 | last_midi_channel;
				vstem.midiData[1] = 0x79; //reset all controllers
				vstem.midiData[2] = 0;
				vstem.midiData[3] = 0;
				vstem.deltaFrames = 0;
				event_pointers->numEvents++;
			}

			stop_all_notes = false;
		}

		for (int i = 0; i < midi_event_count; i++) {
			if (event_pointers->numEvents == MAX_INPUT_EVENTS) {
				break;
			}
			const MIDIEvent &ev = midi_events[i].event;
			int frame_offset = midi_events[i].frame;

			VstMidiEvent &vstem = event_array[event_pointers->numEvents];
			vstem.deltaFrames = frame_offset;

			switch (ev.type) {
				case MIDIEvent::MIDI_NOTE_ON: {

					vstem.midiData[0] = 0x90 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.note.note;
					vstem.midiData[2] = ev.note.velocity;
					vstem.midiData[3] = 0;
					last_midi_channel = ev.channel; //remember for note off?
				} break;
				case MIDIEvent::MIDI_NOTE_OFF: {

					vstem.midiData[0] = 0x80 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.note.note;
					vstem.midiData[2] = ev.note.velocity;
					vstem.midiData[3] = 0;
				} break;
				case MIDIEvent::MIDI_CONTROLLER: {

					vstem.midiData[0] = 0xB0 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.control.index;
					vstem.midiData[2] = ev.control.parameter;
					vstem.midiData[3] = 0;

				} break;
				case MIDIEvent::MIDI_PITCH_BEND: {

					vstem.midiData[0] = 0xE0 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.pitch_bend.bend & 0x7F;
					vstem.midiData[2] = ev.pitch_bend.bend >> 7;
					vstem.midiData[3] = 0;
				} break;
				case MIDIEvent::MIDI_AFTERTOUCH: {

					vstem.midiData[0] = 0xD0 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.aftertouch.pressure;
					vstem.midiData[2] = 0;
					vstem.midiData[3] = 0;
				} break;
				case MIDIEvent::MIDI_NOTE_PRESSURE: {

					vstem.midiData[0] = 0xA0 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.note.note;
					vstem.midiData[2] = ev.note.velocity;
					vstem.midiData[3] = 0;
				} break;
				case MIDIEvent::MIDI_PATCH_SELECT: {

					vstem.midiData[0] = 0xC0 | ev.channel; //channel 0?
					vstem.midiData[1] = ev.note.note;
					vstem.midiData[2] = ev.note.velocity;
					vstem.midiData[3] = 0;
				} break;
				default: {
					//unhandled, dont add
					continue;
				}
			}

			event_pointers->numEvents++;
		}

		effect->dispatcher(effect, effProcessEvents, 0, 0, event_pointers, 0.0f);
	}

	float **in_buffer_ptrs = in_buffers.size() ? &in_buffers[0] : 0;
	float **out_buffer_ptrs = out_buffers.size() ? &out_buffers[0] : 0;

	effect->processReplacing(effect, in_buffer_ptrs, out_buffer_ptrs, process_block_size);
}
void AudioEffectVST2::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	if (in_buffers.size() >= 2) {
		float *in_l = in_buffers[0];
		float *in_r = in_buffers[1];
		for (int i = 0; i < process_block_size; i++) {
			in_l[i] = p_in[i].l;
			in_r[i] = p_in[i].r;
		}
	}

	_process(p_events, p_event_count);

	float *out_l = out_buffers[0];
	float *out_r = out_buffers[1];
	for (int i = 0; i < process_block_size; i++) {
		p_out[i].l = out_l[i];
		p_out[i].r = out_r[i];
	}
}
void AudioEffectVST2::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {
	if (in_buffers.size() >= 4) {
		float *in_l = in_buffers[0];
		float *in_r = in_buffers[1];
		float *sec_l = in_buffers[2];
		float *sec_r = in_buffers[3];
		for (int i = 0; i < process_block_size; i++) {
			in_l[i] = p_in[i].l;
			in_r[i] = p_in[i].r;
			sec_l[i] = p_secondary[i].l;
			sec_r[i] = p_secondary[i].r;
		}
	}

	_process(p_events, p_event_count);

	float *out_l = out_buffers[0];
	float *out_r = out_buffers[1];
	for (int i = 0; i < process_block_size; i++) {
		p_out[i].l = out_l[i];
		p_out[i].r = out_r[i];
	}
}

void AudioEffectVST2::_clear_buffers() {

	for (int i = 0; i < in_buffers.size(); i++) {
		delete[] in_buffers[i];
	}
	in_buffers.clear();
	for (int i = 0; i < out_buffers.size(); i++) {
		delete[] out_buffers[i];
	}
	out_buffers.clear();
}
void AudioEffectVST2::_update_buffers() {

	_clear_buffers();

	in_buffers.resize(effect->numInputs);
	for (int i = 0; i < effect->numInputs; i++) {
		in_buffers[i] = new float[process_block_size];
	}

	out_buffers.resize(effect->numOutputs);
	for (int i = 0; i < effect->numOutputs; i++) {
		out_buffers[i] = new float[process_block_size];
	}

	has_side_input = in_buffers.size() == 4 && out_buffers.size() >= 2;
}
void AudioEffectVST2::set_process_block_size(int p_size) {
	if (process_block_size == p_size) {
		return;
	}
	process_block_size = p_size;
	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);
	effect->dispatcher(effect, effSetBlockSize, 0, process_block_size, NULL, 0.0f);
	effect->dispatcher(effect, effMainsChanged, 0, 1, NULL, 0.0f);
	_update_buffers();
}

void AudioEffectVST2::set_sampling_rate(int p_hz) {
	if (sampling_rate == p_hz) {
		return;
	}
	sampling_rate = p_hz;
	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);
	effect->dispatcher(effect, effSetSampleRate, 0, 0, NULL, sampling_rate);
	effect->dispatcher(effect, effMainsChanged, 0, 1, NULL, 0.0f);
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

	//attempt to reset the plugin in any way possible, since it's not clear in VST how to do it

	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);
	//reset preset.. this maybe helps..
	int current_preset = effect->dispatcher(effect, effGetProgram, 0, 0, 0, 0.0f);
	effect->dispatcher(effect, effSetProgram, 0, current_preset, NULL, 0.0f);
	//switch the plugin back on (calls Resume)
	effect->dispatcher(effect, effMainsChanged, 0, 1, NULL, 0.0f);

	if (effect->flags & effFlagsIsSynth) {
		stop_all_notes = true;
		_reset_midi();
	}
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
#ifdef WINDOWS_ENABLED
void AudioEffectVST2::open_user_interface(void *p_window_ptr) {

	effect->dispatcher(effect, effEditOpen, 0, 0, p_window_ptr, 0);
}
#else
void AudioEffectVST2::open_user_interface(long p_longint, void *p_window_ptr) {

	effect->dispatcher(effect, effEditOpen, 0, p_longint, p_window_ptr, 0);
}

#endif
void AudioEffectVST2::resize_user_interface(int p_width, int p_height) {

	//fst->amc (fst->plugin, 15 /*audioMasterSizeWindow */, width, height, NULL, 0);
	effect->dispatcher(effect, audioMasterSizeWindow, p_width, p_height, NULL, 0);
}

void AudioEffectVST2::close_user_interface() {
	effect->dispatcher(effect, effEditClose, 0, 0, NULL, 0);
}

#if 1
#define DEBUG_CALLBACK(m_text)
#else
#define DEBUG_CALLBACK(m_text) printf("VST Callback: %s\n", m_text)
#endif

intptr_t VESTIGECALLBACK AudioEffectVST2::host(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt) {

	//simple host for exploring plugin
	AudioEffectVST2 *vst_effect = effect ? (AudioEffectVST2 *)effect->resvd1 : 0;

	switch (opcode) {
		//VST 1.0 opcodes
		case audioMasterAutomate:
			DEBUG_CALLBACK("audioMasterAutomate");
			// index, value, returns 0
			if (vst_effect) {
				if (index >= 0 && index < vst_effect->control_ports.size() && !vst_effect->control_ports[index].setting) {
					vst_effect->control_ports[index].ui_changed_notify();
				}
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

		case audioMasterWantMidi:
			DEBUG_CALLBACK("audioMasterWantMidi");
			return 1;

		case audioMasterGetTime:
			return (intptr_t)NULL; //no time
#if 0
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

				return 1;
			}

		case audioMasterGetSampleRate:
			DEBUG_CALLBACK("audioMasterGetSampleRate");
			if (vst_effect) {
				return vst_effect->sampling_rate;
			}
			return 0;

		case audioMasterGetBlockSize:
			DEBUG_CALLBACK("audioMasterGetBlockSize");
			if (vst_effect) {
				return vst_effect->process_block_size;
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
			if (index >= 0 && index < vst_effect->control_ports.size()) {
				vst_effect->control_ports[index].editing = true;
			}
			// begin of automation session (when mouse down), parameter index in <index>
			return 0;

		case audioMasterEndEdit:
			DEBUG_CALLBACK("audioMasterEndEdit");
			// end of automation session (when mouse up),     parameter index in <index>
			if (index >= 0 && index < vst_effect->control_ports.size()) {
				vst_effect->control_ports[index].editing = false;
				vst_effect->control_ports[index].ui_changed_notify();
			}

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

JSON::Node AudioEffectVST2::_internal_to_json() const {
	JSON::Node node = JSON::object();

	node.add("type", "vst2");
	//save the program first, if programs exist
	if (effect->numPrograms > 0) {
		int program_number = effect->dispatcher(effect, effGetProgram, 0, 0, NULL, 0.0f);
		node.add("program", program_number);
	}
	//check whether to use chunks or params
	if (effect->flags & effFlagsProgramChunks) {
		unsigned char *data;
		int data_size = effect->dispatcher(effect, effGetChunk, 1, 0, &data, 0);
		if (data_size) {
			Vector<uint8_t> datav;
			datav.resize(data_size);
			memcpy(&datav[0], data, data_size);
			node.add("chunk", base64_encode(datav));
		}
	} else {
		//I guess VST warrants that these keep their indices
		JSON::Node states = JSON::array();
		for (int i = 0; i < control_ports.size(); i++) {
			states.append(control_ports[i].get());
		}
		node.add("param_states", states);
	}

	return node;
}
Error AudioEffectVST2::_internal_from_json(const JSON::Node &node) {

	ERR_FAIL_COND_V(!node.has("type"), ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V(node.get("type").toString() != "vst2", ERR_FILE_CORRUPT);

	if (node.has("program")) {
		int program = node.get("program").toInt();
		effect->dispatcher(effect, effSetProgram, 0, program, NULL, 0.0f);
	}

	if (node.has("chunk")) {
		std::string chunk_str = node.get("chunk").toString();
		Vector<uint8_t> data = base64_decode(chunk_str);
		effect->dispatcher(effect, effSetChunk, 1, data.size(), &data[0], 0);
	} else if (node.has("param_states")) {
		JSON::Node states = node.get("param_states");

		for (int i = 0; i < states.getCount(); i++) {
			if (i >= control_ports.size()) {
				break;
			}
			float value = states.get(i).toFloat();
			control_ports[i].set(value);
		}
	}
	return OK;
}

String AudioEffectVST2::get_path() const {
	return path;
}

float AudioEffectVST2::ControlPortVST2::get() const {
	return effect->getParameter(effect, index);
}
void AudioEffectVST2::ControlPortVST2::set(float p_val) {
	if (editing) { //being edited
		return;
	}
	setting = true; //lock used to avoid rogue effects emitting automated when being set.
	effect->setParameter(effect, index, p_val);
	setting = false;
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
#ifdef WINDOWS_ENABLED
	libhandle = LoadLibraryW(p_path.c_str());
#else
	libhandle = dlopen(p_path.utf8().get_data(), RTLD_LOCAL | RTLD_LAZY);
#endif

	effect = AudioEffectProviderVST2::open_vst_from_lib_handle(libhandle, host);
	if (!effect) {
		return ERR_CANT_OPEN;
	}
	effect->resvd1 = this;
	effect->dispatcher(effect, effOpen, 0, 0, NULL, 0.0f);
	vst_version = effect->dispatcher(effect, effGetVstVersion, 0, 0, NULL, 0.0f);
	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);

	control_ports.resize(effect->numParams);
	for (int i = 0; i < effect->numParams; i++) {
		ControlPortVST2 *cp = &control_ports[i];
		cp->visible = true; //bleh just allow all - i < 100; //the first 50 are visible, the rest are not
		cp->index = i;
		cp->effect = effect;
		cp->setting = false;
		cp->editing = false;
		char label[kVstMaxLabelLen + 1];
		effect->dispatcher(effect, effGetParamLabel, i, 0, label, 0); //just crap
		label[kVstMaxLabelLen] = 0;
		cp->label.parse_utf8(label);
		cp->label = " " + cp->label;
		effect->dispatcher(effect, effGetParamName, i, 0, label, 0); //just crap
		label[kVstMaxLabelLen] = 0;
		cp->name.parse_utf8(label);
		cp->identifier = cp->name.to_lower();
		cp->identifier.replace(" ", "_");
		cp->identifier = "vst_param_" + cp->identifier;
		float value = effect->getParameter(effect, i);
		cp->value = value;
	}
	effect->dispatcher(effect, effSetSampleRate, 0, 0, NULL, sampling_rate);
	effect->dispatcher(effect, effSetBlockSize, 0, process_block_size, NULL, 0.0f);
	_update_buffers();
	effect->dispatcher(effect, effSetProcessPrecision, 0, 0, NULL, 0.0f);
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
	process_block_size = 128;
	sampling_rate = 44100;
	has_side_input = false;
	stop_all_notes = false;
	event_pointer_data = new unsigned char[sizeof(int32_t) + sizeof(intptr_t) + sizeof(VstEvent *) * MAX_INPUT_EVENTS];
	event_pointers = (VstEvents *)event_pointer_data;
	last_midi_channel = 0;

	event_pointers->numEvents = 0;
	event_pointers->reserved = 0;

	for (int i = 0; i < MAX_INPUT_EVENTS; i++) {

		event_array[i].type = kVstMidiType;
		event_array[i].byteSize = 24;
		event_array[i].deltaFrames = 0;
		event_array[i].flags = 0; ///< @see VstMidiEventFlags
		event_array[i].noteLength = 0; ///< (in sample frames) of entire note, if available,
		event_array[i].noteOffset = 0; ///< offset (in sample frames) into note from note
		event_array[i].midiData[0] = 0;
		event_array[i].midiData[1] = 0;
		event_array[i].midiData[2] = 0;
		event_array[i].midiData[3] = 0;
		event_array[i].detune = 0;
		event_array[i].noteOffVelocity = 0;
		event_array[i].reserved1 = 0;
		event_pointers->events[i] = (VstEvent *)&event_array[i];
	}
}

AudioEffectVST2::~AudioEffectVST2() {
	_clear_buffers();
	delete[] event_pointer_data;
	if (effect) {
		effect->dispatcher(effect, effClose, 0, 0, NULL, 0.0f);
	}
	if (libhandle) {
#ifdef WINDOWS_ENABLED
		FreeLibrary(libhandle);
#else
		dlclose(libhandle);
#endif
	}
}
