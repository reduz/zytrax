#ifndef MIDI_DRIVER_MANAGER_H
#define MIDI_DRIVER_MANAGER_H

#include "dsp/midi_event.h"
#include "globals/error_macros.h"
#include "globals/rstring.h"
#include "globals/vector.h"
#include <set>

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

class MIDIBankManager {
public:
	enum {
		MAX_DEVICE_FILES = 128
	};
private:

	struct DeviceFile {
		String name;
		String path;
	};

	static DeviceFile device_files[MAX_DEVICE_FILES];

	struct Bank {
		String device_name;
		String name;
		struct Patch {
			int msb;
			int lsb;
			int index;
			String name;
		};
		Vector<Patch> patch_list;
	};

	static Vector<Bank> banks;

	static void _load_device_file(int p_index, String p_file);

	static std::set<std::string> favorites;
public:

	static constexpr const char * GM_DEVICE_NAME = "General MIDI";
	static constexpr int GM_DEVICE_INDEX = -1;

	static void set_device_file_path(int p_bank_file,String p_path);
	static String get_device_file_path(int p_bank_file);
	static String get_device_file_name(int p_bank_file);

	static void reload_devices();

	static int get_bank_count();
	static String get_bank_device(int p_bank);
	static String get_bank_name(int p_bank);
	static int get_bank_patch_count(int p_bank);
	static String get_bank_patch_name(int p_bank,int p_patch);
	static int get_bank_patch_index(int p_bank,int p_patch);
	static int get_bank_patch_msb(int p_bank,int p_patch);
	static int get_bank_patch_lsb(int p_bank,int p_patch);

	static void set_favorite(std::string p_device,std::string p_bank,int p_msb, int p_lsb, int p_patch_index,bool p_favorite);
	static bool is_favorite(std::string p_device,std::string p_bank,int p_msb, int p_lsb, int p_patch_index);

	static std::set<std::string>& get_favorites() { return favorites; }

};

#endif // MIDI_DRIVER_MANAGER_H
