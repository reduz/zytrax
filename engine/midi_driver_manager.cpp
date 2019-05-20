#include "midi_driver_manager.h"

void MIDIInputDriver::event(double p_stamp, const MIDIEvent &p_event) {
	if (MIDIDriverManager::event_callback) {
		MIDIDriverManager::event_callback(p_stamp, p_event);
	}
}

///////////////////////

MIDIInputDriver *MIDIDriverManager::input_drivers[MIDIDriverManager::MAX_MIDI_DRIVERS];
int MIDIDriverManager::input_driver_count = 0;
int MIDIDriverManager::input_current_driver = -1;

MIDIDriverManager::EventCallback MIDIDriverManager::event_callback = NULL;

void MIDIDriverManager::lock_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->lock();
	}
}
void MIDIDriverManager::unlock_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->unlock();
	}
}

bool MIDIDriverManager::init_input_driver(int p_driver) {
	ERR_FAIL_COND_V(p_driver != -1 && p_driver < 0 && p_driver >= input_driver_count, false);

	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->finish();
	}

	input_current_driver = p_driver;

	if (input_current_driver != -1) {
		return input_drivers[input_current_driver]->init();
	}

	return false;
}
void MIDIDriverManager::finish_input_driver() {
	if (input_current_driver != -1 && input_drivers[input_current_driver]->is_active()) {
		input_drivers[input_current_driver]->finish();
	}
}
bool MIDIDriverManager::is_input_driver_active() {
	return (input_current_driver != -1 && input_drivers[input_current_driver]->is_active());
}
int MIDIDriverManager::get_input_driver_count() {
	return input_driver_count;
}
MIDIInputDriver *MIDIDriverManager::get_input_driver(int p_which) {
	ERR_FAIL_INDEX_V(p_which, input_driver_count, NULL);
	return input_drivers[p_which];
}

int MIDIDriverManager::get_current_input_driver_index() {
	return input_current_driver;
}

void MIDIDriverManager::add_input_driver(MIDIInputDriver *p_driver) {
	ERR_FAIL_COND(input_driver_count == MAX_MIDI_DRIVERS);
	input_drivers[input_driver_count++] = p_driver;
}
void MIDIDriverManager::set_event_callback(EventCallback p_callback) {
	event_callback = p_callback;
}
