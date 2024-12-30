#include "synth_base.h"

bool SynthBase::has_secondary_input() const {
	return false;
}

void SynthBase::process(const Event *p_events, int p_event_count, const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {


	int midi_event_count;
	float time = float(process_block_size) / mix_rate;
	const MIDIEventStamped *midi_events = _process_midi_events(p_events, p_event_count, time, midi_event_count);

	for(int i=0;i<process_block_size;i++) {
		p_out[i] = AudioFrame(0,0);
	}

	// Process events

	bool update_volumes = false;
	bool update_pans = false;
	bool update_data = false;
	bool update_pitch = false;

	for (int i = 0; i < midi_event_count; i++) {
		const MIDIEvent &e = midi_events[i].event;
		switch (e.type) {
			case MIDIEvent::SEQ_TEMPO: {
				_tempo_changed(e.tempo.tempo,midi_events[i].frame);
			} break;
			case MIDIEvent::SEQ_SIGNATURE: {

			} break;
			case MIDIEvent::SEQ_BAR: {
				_bar(e.bar.bar,midi_events[i].frame);
			} break;
			case MIDIEvent::SEQ_BEAT: {
				_beat(e.beat.beat,midi_events[i].frame);
			} break;
			case MIDIEvent::SEQ_SCALE: {

			} break;
			case MIDIEvent::STREAM_TAIL: {

			} break;
			case MIDIEvent::MIDI_NOTE_OFF: {
				for(int j=0;j<active_voices;j++) {
					if (voices[j]->get_status() == Voice::STATUS_ON && voices[j]->get_note() == e.note.note) {
						voices[j]->note_off(_velocity_to_volume(e.note.velocity),midi_events[i].frame);
					}
				}
			} break;
			case MIDIEvent::MIDI_NOTE_ON: {
				// off a current note if active (may not have received note off.
				int voice_index = -1;
				for(int j=0;j<active_voices;j++) {
					Voice::Status s = voices[j]->get_status();
					if (s == Voice::STATUS_ON && voices[j]->get_note() == e.note.note) {
						voices[j]->note_off(_velocity_to_volume(e.note.velocity),midi_events[i].frame);
					}
					if (voice_index == -1 && s == Voice::STATUS_OFF) {
						voice_index = j;
					} else if (s== Voice::STATUS_DISABLED) {
						voice_index = j;
					}
				}

				if (active_voices < MAX_VOICES) {
					voice_index = active_voices;
					active_voices++;
				} else {
					if (voice_index == -1) {
						voice_index = 0;
					}
				}

				if (voices[voice_index]->get_status() != Voice::STATUS_DISABLED) {
					// Had to reuse a voice, mix buffer down, then kill it.
					//voices[voice_index]-> set volume to zero
					voices[voice_index]->add_to_mix(p_out);
					voices[voice_index]->kill();
				}

				voices[voice_index]->set_volume( volume_final );
				voices[voice_index]->set_pan( pan_final );
				voices[voice_index]->set_pitch_offset( pitch_bend_final * pitch_range );
				voices[voice_index]->note_on(e.note.note, _velocity_to_volume(e.note.velocity),midi_events[i].frame);

			} break;
			case MIDIEvent::MIDI_NOTE_PRESSURE: {
				for(int j=0;j<active_voices;j++) {
					if (voices[j]->get_status() == Voice::STATUS_ON && voices[j]->get_note() == e.note.note) {
						voices[j]->note_aftertouch(_velocity_to_volume(e.note.velocity),midi_events[i].frame);
					}
				}
			} break;
			case MIDIEvent::MIDI_CONTROLLER: {

				int controller = e.control.index;
				int control_value = e.control.parameter;

				switch (controller) {
					case   7 /*VOLUME_MSB*/      : {
						midi_volume = (uint16_t)((midi_volume & 0x7F) | (control_value << 7));
						update_volumes = true;
					} break;
					case  39 /*VOLUME_LSB*/      : {
						midi_volume = (uint16_t)((midi_volume & 0x3F80) |  control_value);
						update_volumes = true;
					} break;
					case  11 /*EXPRESSION_MSB*/  :  {
						midi_expression = (uint16_t)((midi_expression & 0x7F) | (control_value << 7));
						update_volumes = true;
					} break;
					case  43 /*EXPRESSION_LSB*/  : {
						midi_expression = (uint16_t)((midi_expression & 0x3F80) |  control_value);
						update_volumes = true;
					} break;
					case  10 /*PAN_MSB*/         : {
						midi_pan = (uint16_t)((midi_pan & 0x7F) | (control_value << 7));
						update_pans = true;
					} break;
					case  42 /*PAN_LSB*/         : {
						midi_pan = (uint16_t)((midi_pan & 0x3F80) |  control_value);
						update_pans = true;
					} break;
					case   6 /*DATA_ENTRY_MSB*/  :
					case  38 /*DATA_ENTRY_LSB*/  : {
						if (controller == 6) {
							midi_data       = (uint16_t)((midi_data       & 0x7F)   | (control_value << 7));;
						} else {
							midi_data       = (uint16_t)((midi_data       & 0x3F80) |  control_value);
						}
						update_data = true;
					} break;
					case 101 /*RPN_MSB*/         : {
						midi_rpn = (uint16_t)(((midi_rpn == 0xFFFF ? 0 : midi_rpn) & 0x7F) | (control_value << 7));
					} break;
					case 100 /*RPN_LSB*/         : {
						midi_rpn = (uint16_t)(((midi_rpn == 0xFFFF ? 0 : midi_rpn) & 0x3F80) |  control_value);
					} break;
					case  98 /*NRPN_LSB*/        :
					case  99 /*NRPN_MSB*/        : {
						midi_rpn = 0xFFFF;
					} break;
					case 123 /*ALL_NOTES_OFF*/   : {
						for(int j=0;j<active_voices;j++) {
							if (voices[j]->get_status() == Voice::STATUS_ON) {
								voices[j]->note_off(1.0,midi_events[i].frame);
							}
						}
					} break;
					case 121 /*ALL_CTRL_OFF*/    : {
						midi_volume = midi_expression = 16383;
						midi_pan = 8192;
						midi_rpn = 0xFFFF;
						midi_data = 0;
						update_volumes = true;
						update_pitch = true;
						update_pans = true;
					} // fallthrough
					case 120 /*ALL_SOUND_OFF*/   : {
						for(int j=0;j<active_voices;j++) {
							if (voices[j]->get_status() == Voice::STATUS_DISABLED) {
								voices[j]->kill();
							}
						}
						active_voices = 0;
						_reset_sound();
					} break;

				}
			} break;
			case MIDIEvent::MIDI_PATCH_SELECT: {
				// Ignored.
			} break;
			case MIDIEvent::MIDI_AFTERTOUCH: {
				// Ignored.
			} break;
			case MIDIEvent::MIDI_PITCH_BEND: {
				pitch_wheel = e.pitch_bend.bend;
				update_pitch = true;
			} break;
			case MIDIEvent::MIDI_SYSEX: {

			} break;
		}
	}

	if (update_volumes) {
		volume_final = powf((midi_volume / 16383.0f) * (midi_expression / 16383.0f), 2.0);
		for(int i=0;i<active_voices;i++) {
			voices[i]->set_volume(volume_final);
		}
	}

	if (update_pans) {
		pan_final = midi_pan / 16383.0f;
		for(int i=0;i<active_voices;i++) {
			voices[i]->set_pan(pan_final);
		}
	}

	if (update_data) {
		if (midi_rpn == 0) {
			pitch_range = (midi_data >> 7) + 0.01f * (midi_data & 0x7F);
			update_pitch = true;
		}
	}

	if (update_pitch) {
		pitch_bend_final = (float(pitch_wheel) - 8192.0) / 8192.0;
		for(int i=0;i<active_voices;i++) {
			voices[i]->set_pitch_offset( pitch_bend_final * pitch_range );
		}
	}


	// Process audio

	for(int i=0;i<active_voices;i++) {
		voices[i]->add_to_mix(p_out);

		if (voices[i]->get_status() == Voice::STATUS_DISABLED) {
			if (i < active_voices -1) {
				Voice * v = voices[i];
				voices[i] = voices[active_voices -1 ];
				voices[active_voices -1 ] = v;
				i--;
			}
			active_voices--;
		}
	}

	_post_process(p_out);
}

void SynthBase::process_with_secondary(const Event *p_events, int p_event_count, const AudioFrame *p_in, const AudioFrame *p_secondary, AudioFrame *p_out, bool p_prev_active) {

}

void SynthBase::set_process_block_size(int p_size) {
	process_block_size = p_size;
}

void SynthBase::set_sampling_rate(int p_hz) {
	mix_rate = p_hz;
}
void SynthBase::_setup() {

	for(int i=0;i<MAX_VOICES;i++) {
		voices[i] = create_voice();
	}
}

SynthBase::SynthBase() {

}

