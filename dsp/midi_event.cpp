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
