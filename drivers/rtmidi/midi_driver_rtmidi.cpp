#include "midi_driver_rtmidi.h"

#include "engine/midi_driver_manager.h"
#include "globals/vector.h"
//
#include "drivers/rtmidi/rtmidi/RtMidi.h"

static RtMidiIn *midiin = NULL;

class MIDIInputDriverRtMidi : public MIDIInputDriver {
public:
	static void midi_callback(double deltatime, std::vector<unsigned char> *message, void *userData) {
		MIDIInputDriverRtMidi *driver = (MIDIInputDriverRtMidi *)userData;
		MIDIEvent ev;
		if (ev.parse(&(*message)[0]) == OK) {
			driver->event(deltatime, ev);
		}
	}

	virtual void lock() {
	}
	virtual void unlock() {
	}

	String name;
	String id;
	int index;
	bool active;

	virtual String get_name() const {
		return name;
	}
	virtual String get_id() const {
		return id;
	}

	virtual bool is_active() {
		return active;
	}
	virtual bool init() {
		ERR_FAIL_COND_V(active, false);
		midiin->openPort(index);
		// Set our callback function.  This should be done immediately after
		// opening the port to avoid having incoming messages written to the
		// queue.
		midiin->setCallback(&midi_callback, this);
		// Don't ignore sysex, timing, or active sensing messages.
		midiin->ignoreTypes(false, false, false);
		active = true;
		return true;
	}
	virtual void finish() {
		midiin->closePort();
		active = false;
	}

	MIDIInputDriverRtMidi() {
		index = -1;
		active = false;
	}
	~MIDIInputDriverRtMidi() {
	}
};

static Vector<MIDIInputDriverRtMidi *> midi_drivers;

void register_rtmidi_driver() {

	midiin = new RtMidiIn();

	unsigned int nPorts = midiin->getPortCount();
	for (int i = 0; i < nPorts; i++) {
		MIDIInputDriverRtMidi *driver = new MIDIInputDriverRtMidi;
		driver->name.parse_utf8(midiin->getPortName(i).c_str());
		driver->id = "RtMidi:" + driver->name;
		driver->index = i;
		midi_drivers.push_back(driver);
		MIDIDriverManager::add_input_driver(driver);
	}
}

void unregister_rtmidi_driver() {
	for (int i = 0; i < midi_drivers.size(); i++) {
		delete midi_drivers[i];
	}
	delete midiin;
}
