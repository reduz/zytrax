#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

struct MIDIEvent {
public:
	enum CC { //enum of those supported, use cc_indices to get actual number

		CC_BANK_SELECT_MSB,
		CC_MODULATION,
		CC_BREATH,
		CC_FOOT,
		CC_PORTAMENTO_TIME,
		CC_DATA_ENTRY_MSB,
		CC_MAIN_VOLUME,
		CC_PAN,
		CC_EXPRESSION,
		CC_BANK_SELECT_LSB,
		CC_DATA_ENTRY_LSB,
		CC_DAMPER_PEDAL_TOGGLE,
		CC_PORTAMENTO_TOGGLE,
		CC_SOSTENUTO_TOGGLE,
		CC_SOFT_PEDAL_TOGGLE,
		CC_FILTER_CUTOFF,
		CC_RELEASE_TIME,
		CC_ATTACK_TIME,
		CC_FILTER_RESONANCE,
		CC_DECAY_TIME,
		CC_VIBRATO_DEPTH,
		CC_VIBRATO_RATE,
		CC_VIBRATO_DELAY,
		CC_PORTAMENTO_CONTROL,
		CC_REVERB_SEND,
		CC_FX2_SEND,
		CC_CHORUS_SEND,
		CC_FX4_SEND,
		CC_DATA_INCREMENT,
		CC_DATA_DECREMENT,
		CC_NRPN_LSB,
		CC_NRPN_MSB,
		CC_RPN_LSB,
		CC_RPN_MSB,
		CC_ALL_SOUNDS_OFF_CMD,
		CC_RESET_ALL_CC_CMD,
		CC_LOCAL_CTRL_TOGGLE,
		CC_ALL_NOTES_OFF,
		CC_MAX
	};

	static const char *cc_names[CC_MAX];
	static const unsigned char cc_indices[CC_MAX];
	enum Type {

		NONE = 0x0,
		SEQ_TEMPO = 0x1,
		SEQ_SIGNATURE = 0x2,
		SEQ_BAR = 0x3,
		SEQ_BEAT = 0x4,
		SEQ_SCALE = 0x5,
		STREAM_TAIL = 0x7, // end of stream, for stream buffers
		MIDI_NOTE_OFF = 0x8,
		MIDI_NOTE_ON = 0x9,
		MIDI_NOTE_PRESSURE = 0xA,
		MIDI_CONTROLLER = 0xB,
		MIDI_PATCH_SELECT = 0xC,
		MIDI_AFTERTOUCH = 0xD, //channel pressure
		MIDI_PITCH_BEND = 0xE,
		MIDI_SYSEX = 0xF, //this will not be used here for now anway
	};

	unsigned char type; //see Type enum
	unsigned char channel; // 0 - 15

	union {

		struct {
			unsigned char param1;
			unsigned char param2;
		} raw;
		struct { /* raw, 2 bytes */
			unsigned short param;
		} raw2;

		struct { /* Note On / Note Off / Note Pressure */

			unsigned char note;
			unsigned char velocity;
		} note;

		struct { /* Controller */

			unsigned char index; //see cc_indices[CC] enum
			unsigned char parameter;
		} control;

		struct { /* Channel Pressure */

			unsigned char pressure;
		} aftertouch;

		struct { /* Patch */

			unsigned char index;
		} patch;

		struct { /* Pitch Bend */

			unsigned short bend; /* 0 - 0x3999 */
		} pitch_bend;

		struct {

			unsigned short tempo;
		} tempo;

		struct {

			unsigned char num;
			unsigned char denom;
		} signature;

		struct {

			unsigned short bar; // warning, max is  65535, It's a high number but may roll around
		} bar;
		struct {

			unsigned char beat;
		} beat;
		struct {

			enum ScaleType {
				SCALE_MAJOR,
				SCALE_MINOR,
				/* Will have add more later */
			};

			unsigned char scale_type;
			char key_note; /* 0 .. 11 */
		} scale;
	};

	MIDIEvent();
	MIDIEvent(Type p_type, unsigned char p_chan, unsigned char data1, unsigned char data2);
	MIDIEvent(Type p_type, unsigned char p_chan, unsigned short data);
};
#endif // EVENT_H
