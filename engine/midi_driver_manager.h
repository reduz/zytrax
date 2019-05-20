#ifndef MIDI_DRIVER_MANAGER_H
#define MIDI_DRIVER_MANAGER_H

#include "dsp/midi_event.h"
#include "globals/error_macros.h"
#include "globals/rstring.h"

class MIDIInputDriver {
protected:
	void event(double p_stamp, const MIDIEvent &p_event);

public:
	virtual void lock() = 0; ///< Lock called from UI,game,etc (non-audio) thread, to access audio variables
	virtual void unlock() = 0; ///< UnLock called from UI,game,etc (non-audio) thread, to access audio variables

	virtual String get_name() const = 0;
	virtual String get_id() const = 0;

	virtual bool is_active() = 0;
	virtual bool init() = 0;
	virtual void finish() = 0;

	MIDIInputDriver() {}
	virtual ~MIDIInputDriver() {}
};

class MIDIDriverManager {

	enum {
		MAX_MIDI_DRIVERS = 64
	};

public:
	typedef void (*EventCallback)(double p_stamp, const MIDIEvent &);

private:
	static MIDIInputDriver *input_drivers[MAX_MIDI_DRIVERS];
	static int input_driver_count;
	static int input_current_driver;

	friend class MIDIInputDriver;
	static EventCallback event_callback;

public:
	static void lock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads
	static void unlock_driver(); ///< Protect audio thread variables from ui,game,etc (non-audio) threads

	static bool init_input_driver(int p_driver = -1); ///< -1 is current
	static void finish_input_driver();
	static bool is_input_driver_active();
	static int get_input_driver_count();
	static MIDIInputDriver *get_input_driver(int p_which = -1); ///< -1 is current

	static int get_current_input_driver_index();

	static void add_input_driver(MIDIInputDriver *p_driver);
	static void set_event_callback(EventCallback p_callback);
};

#endif // MIDI_DRIVER_MANAGER_H
