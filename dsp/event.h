#ifndef EVENT_H
#define EVENT_H



enum CC {

	CC_BANK_SELECT_MSB=0,
	CC_MODULATION=1,
	CC_BREATH=2,
	CC_FOOT=4,
	CC_PORTAMENTO_TIME=5,
	CC_DATA_ENTRY_MSB=6,
	CC_MAIN_VOLUME=7,
	CC_PAN=10,
	CC_EXPRESSION=11,
	CC_BANK_SELECT_LSB=32,
	CC_DATA_ENTRY_LSB=38,
	CC_DAMPER_PEDAL_TOGGLE=64,
	CC_PORTAMENTO_TOGGLE=65,
	CC_SOSTENUTO_TOGGLE=66,
	CC_SOFT_PEDAL_TOGGLE=67,
	CC_FILTER_CUTOFF=71,
	CC_RELEASE_TIME=72,
	CC_ATTACK_TIME=73,
	CC_FILTER_RESONANCE=74,
	CC_DECAY_TIME=75,
	CC_VIBRATO_DEPTH=76,
	CC_VIBRATO_RATE=77,
	CC_VIBRATO_DELAY=78,
	CC_PORTAMENTO_CONTROL=84,
	CC_REVERB_SEND=91,
	CC_FX2_SEND=92,
	CC_CHORUS_SEND=93,
	CC_FX4_SEND=94,
	CC_DATA_INCREMENT=96,
	CC_DATA_DECREMENT=97,
	CC_NRPN_LSB=98,
	CC_NRPN_MSB=99,
	CC_RPN_LSB=98,
	CC_RPN_MSB=99,
	CC_ALL_SOUNDS_OFF_CMD=120,
	CC_RESET_ALL_CC_CMD=121,
	CC_LOCAL_CTRL_TOGGLE=122,
	CC_ALL_NOTES_OFF=123,
	CC_MAX=128

};


struct Event {
public:

	enum Type {

		NONE=0x0,
		SEQ_TEMPO=0x1,
		SEQ_SIGNATURE=0x2,
		SEQ_BAR=0x3,
		SEQ_BEAT=0x4,
		SEQ_SCALE=0x5,
		STREAM_TAIL=0x7, // end of stream, for stream buffers
		MIDI_NOTE_OFF=0x8,
		MIDI_NOTE_ON=0x9,
		MIDI_NOTE_PRESSURE=0xA,
		MIDI_CONTROLLER=0xB,
		MIDI_PATCH_SELECT=0xC,
		MIDI_AFTERTOUCH=0xD, //channel pressure
		MIDI_PITCH_BEND=0xE,
		MIDI_SYSEX=0xF, //this will not be used here for now anway
	};

	unsigned int frame;

	unsigned char type;
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

			unsigned char index;
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

	Event();
	Event(Type p_type, unsigned char p_chan,unsigned char data1, unsigned char data2 );

	~Event();

};
#endif // EVENT_H
