#include "midi_event.h"

const char *MIDIEvent::cc_names[CC_MAX]{

	"BankSelectMSB",
	"Modulation",
	"Breath",
	"Foot",
	"PortamentoTime",
	"DataEntryMSB",
	"MainVolume",
	"Pan",
	"Expression",
	"BankSelectLSB",
	"DataEntryLSB",
	"DamperPedalToggle",
	"PortamentoToggle",
	"SostenutoToggle",
	"SoftPedalToggle",
	"FilterCutoff",
	"ReleaseTime",
	"AttackTime",
	"FilterResonance",
	"DecayTime",
	"VibratoDepth",
	"VibratoRate",
	"VibratoDelay",
	"PortamentoControl",
	"ReverbSend",
	"Fx2Send",
	"ChorusSend",
	"Fx4Send",
	"DataIncrement",
	"DataDecrement",
	"NRPN_LSB",
	"NRPN_MSB",
	"RPN_LSB",
	"RPN_MSB",
	"AllSoundsOffCmd",
	"ResetAllCmd",
	"LocalCtrlToggle",
	"AllNotesOff"
};

const unsigned char MIDIEvent::cc_indices[CC_MAX]{

	0,
	1,
	2,
	4,
	5,
	6,
	7,
	10,
	11,
	32,
	38,
	64,
	65,
	66,
	67,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	84,
	91,
	92,
	93,
	94,
	96,
	97,
	98,
	99,
	120,
	121,
	122,
	123,
	128
};

Error MIDIEvent::parse(unsigned char *p_raw) {

	switch (p_raw[0] >> 4) {
		case 0x8: {
			type = MIDI_NOTE_OFF;
			note.note = p_raw[1];
			note.velocity = p_raw[2];
		} break;
		case 0x9: {
			type = MIDI_NOTE_ON;
			note.note = p_raw[1];
			note.velocity = p_raw[2];
		} break;
		case 0xA: {
			type = MIDI_NOTE_PRESSURE;
			note.note = p_raw[1];
			note.velocity = p_raw[2];
		} break;
		case 0xB: {
			type = MIDI_CONTROLLER;
			control.index = p_raw[1];
			control.parameter = p_raw[2];
		} break;
		case 0xC: {
			type = MIDI_PATCH_SELECT;
			patch.index = p_raw[1];
		} break;
		case 0xD: {
			type = MIDI_AFTERTOUCH;
			aftertouch.pressure = p_raw[1];
		} break;
		case 0xE: {
			type = MIDI_PITCH_BEND;
			pitch_bend.bend = (short(p_raw[1]) << 7) | short(p_raw[2]);
		} break;
		default: {
			return ERR_INVALID_PARAMETER;
		}
	}
	channel = p_raw[0] & 0xF;
	return OK;
}

bool MIDIEvent::write(unsigned char *p_to) const {

	if (type == MIDI_NOTE_OFF) {
		p_to[0] = 0x8;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = note.note;
		p_to[2] = note.velocity;

	} else if (type == MIDI_NOTE_ON) {
		p_to[0] = 0x9;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = note.note;
		p_to[2] = note.velocity;
	} else if (type == MIDI_NOTE_PRESSURE) {
		p_to[0] = 0xA;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = note.note;
		p_to[2] = note.velocity;
	} else if (type == MIDI_CONTROLLER) {
		p_to[0] = 0xB;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = control.index;
		p_to[2] = control.parameter;
	} else if (type == MIDI_PATCH_SELECT) {
		p_to[0] = 0xC;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = patch.index;
		p_to[2] = 0;
	} else if (type == MIDI_AFTERTOUCH) {
		p_to[0] = 0xD;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = aftertouch.pressure;
		p_to[2] = 0;
	} else if (type == MIDI_PITCH_BEND) {
		p_to[0] = 0xD;
		p_to[0] <<= 4;
		p_to[0] |= channel;
		p_to[1] = pitch_bend.bend >> 7;
		p_to[2] = pitch_bend.bend & 0x7F;
	} else {
		return false;
	}

	return true;
}

MIDIEvent::MIDIEvent() {
	type = NONE;
	raw.param1 = 0;
	raw.param2 = 0;
}
MIDIEvent::MIDIEvent(Type p_type, unsigned char p_chan, unsigned char data1, unsigned char data2) {
	type = p_type;
	raw.param1 = data1;
	raw.param2 = data2;
}
MIDIEvent::MIDIEvent(Type p_type, unsigned char p_chan, unsigned short data) {

	type = p_type;
	raw2.param = data;
}
