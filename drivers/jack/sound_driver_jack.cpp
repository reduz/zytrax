
#include "sound_driver_jack.h"
#include "engine/sound_driver_manager.h"
#include <mutex>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstring>

class SoundDriverJack : public SoundDriver {
public:
	
	jack_port_t *output_port1 = nullptr, *output_port2 = nullptr;
	jack_client_t *client = nullptr;
	
	struct MidiOutputPort {
		MidiDeviceInfo device;
		jack_port_t *midi_local_port;
	};

	mutable Vector<MidiOutputPort> midi_devices;
	mutable uint32_t midi_devices_hash = 0;
	mutable bool midi_devices_dirty = true;


	enum {
		MIDI_EVENT_BUFFER_SIZE = 0xFFFFF
	};

	MIDIEventRouted midi_events[MIDI_EVENT_BUFFER_SIZE];

	std::recursive_mutex mutex;
	int mix_rate = 44100;
	unsigned int buffer_frames = 1024;
	int step_frames = 1024;

	bool active = false;

	virtual String get_id() const {
		return "jack";
	}
	
	static void _jack_shutdown (void *arg) {
		SoundDriverJack *sdjack = (SoundDriverJack *)arg;
		sdjack->active=false;
	}
	
	static int _jack_process (jack_nframes_t nframes, void *arg) {
		SoundDriverJack *sdjack = (SoundDriverJack *)arg;
		return sdjack->jack_process(nframes);
	}
	
	int jack_process (jack_nframes_t nframes) {

		int i;
		
		jack_default_audio_sample_t *out1 = (jack_default_audio_sample_t*)jack_port_get_buffer(output_port1, nframes);
		jack_default_audio_sample_t *out2 = (jack_default_audio_sample_t*)jack_port_get_buffer(output_port2, nframes);
		
		AudioFrame *audio = (AudioFrame*)alloca(sizeof(AudioFrame)*nframes);
		
		int midi_events_written=0;
		if (mutex.try_lock()) {
			mix(audio, nframes,midi_events,MIDI_EVENT_BUFFER_SIZE,midi_events_written);
			mutex.unlock();
		} else {
			memset(audio,0,sizeof(AudioFrame)*nframes);
			midi_events[0]=MIDIEventRouted();
		}

		
		for( i=0; i<nframes; i++ ) {
			out1[i] = audio[i].l;
			out2[i] = audio[i].r;
		}

		uint32_t midi_output_count = midi_devices.size();

		if (midi_output_count>0) {
			const MidiOutputPort *midi_outputs = &midi_devices[0];
			void ** buffers = (void**)alloca(sizeof(void*) * midi_output_count);

			for(uint32_t i=0;i<midi_output_count;i++) {
				buffers[i]=jack_port_get_buffer(midi_outputs[i].midi_local_port, nframes);
				jack_midi_clear_buffer(buffers[i]);
			}

			for(uint32_t i=0;i<midi_events_written;i++) {
				//printf("JACK:-> %s\n",String(midi_events[i].event).utf8().get_data());
				for(uint32_t j=0;j<midi_output_count;j++) {
					if (midi_outputs[j].device.hash == midi_events[i].port_hash) {
						// Event belongs here.
						jack_midi_data_t midi_event[3];
						midi_events[i].event.write(midi_event);
						//printf("JACK-> %s\n",String(midi_events[i].event).utf8().get_data());
						jack_midi_event_write(buffers[j],midi_events[i].frame,midi_event,3);
						break;
					}
				}
			}
		}
		
		return 0;
	}

	static void _port_registration_callback(jack_port_id_t port, int reg, void *arg) {
		SoundDriverJack *sdjack = (SoundDriverJack *)arg;

		sdjack->lock();
		sdjack->midi_devices_dirty = true;
		sdjack->unlock();
	}

	virtual uint32_t get_midi_devices_hash() const {
		if (midi_devices_dirty) {
			_update_midi_devices();
		}
		return midi_devices_hash;
	}

	virtual Vector<MidiDeviceInfo> get_midi_devices() const {

		if (midi_devices_dirty) {
			_update_midi_devices();
		}


		Vector<MidiDeviceInfo> ret;
		for(int i=0;i<midi_devices.size();i++) {
			ret.push_back(midi_devices[i].device);
		}
		return ret;

	}

	void _update_midi_devices() const {

		const_cast<SoundDriverJack*>(this)->lock();
		for(int i=0;i<midi_devices.size();i++) {
			jack_port_unregister(client,midi_devices[i].midi_local_port);
		}
		midi_devices.clear();
		if (active) {
			const char **ports = nullptr;

			String hash_all;

			ports = jack_get_ports(client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
			int i=0;
			const char * port_name = nullptr;;
			if (ports == nullptr) {

			} else {
				while ((port_name = ports[i++]) != NULL) {

					MidiOutputPort p;
					p.device.name = port_name;
					p.device.hash = p.device.name.strip_parentheticals().hash(); // Jack and ALSA add parenthesis with process and port ID all over the place.
					hash_all+=" -> "+p.device.name;

					String name = "out_"+String::num(i);
					p.midi_local_port = jack_port_register (client, name.utf8().get_data(),
									   JACK_DEFAULT_MIDI_TYPE,
									   JackPortIsOutput, 0);

					if (jack_connect (client, jack_port_name (p.midi_local_port ), port_name)) {
						fprintf (stderr, "cannot connect MIDI output port\n");
					}

					midi_devices.push_back(p);
				}
			}

			jack_free(ports);
			midi_devices_hash = hash_all.hash();
		}
		midi_devices_dirty = false;
		const_cast<SoundDriverJack*>(this)->unlock();

	}

	virtual void lock() {
		mutex.lock();
	}
	virtual void unlock() {
		mutex.unlock();
	}

	virtual String get_name() const {
		return "Jack";
	}

	virtual float get_max_level_l() {
		return 0;
	}
	virtual float get_max_level_r() {
		return 0;
	}

	virtual bool is_active() {
		return active;
	}
	virtual bool init() {
		
		active = false;
		
		jack_status_t status;
		const char *server_name = nullptr;
		client = jack_client_open ("ZyTrax",JackNullOption, &status, server_name);
		
		if (client == NULL) {
			fprintf (stderr, "jack_client_open() failed, "
					 "status = 0x%2.0x\n", status);
			if (status & JackServerFailed) {
				fprintf (stderr, "Unable to connect to JACK server\n");
			}
			return false;
		}
		
		if (status & JackServerStarted) {
			//fprintf (stderr, "JACK server started\n");
		}
		if (status & JackNameNotUnique) {
			//client_name = jack_get_client_name(client);
			//fprintf (stderr, "unique name `%s' assigned\n", client_name);
		}

		jack_set_process_callback (client, _jack_process, this);
		
		/* tell the JACK server to call `jack_shutdown()' if
		 it ever shuts down, either entirely, or if it
		 just decides to stop calling us.
		*/
		
		jack_on_shutdown (client, _jack_shutdown, this);
		
		/* create two ports */
		
		output_port1 = jack_port_register (client, "output1",
						   JACK_DEFAULT_AUDIO_TYPE,
						   JackPortIsOutput, 0);
		
		output_port2 = jack_port_register (client, "output2",
						   JACK_DEFAULT_AUDIO_TYPE,
						   JackPortIsOutput, 0);
		
		if ((output_port1 == NULL) || (output_port2 == NULL)) {
			fprintf(stderr, "no more JACK ports available\n");
			jack_client_close(client);
			return false;
		}
		
		/* Tell the JACK server that we are ready to roll.  Our
		 * process() callback will start running now. */
		
		if (jack_activate (client)) {
			fprintf (stderr, "cannot activate client");
			jack_client_close(client);
			return false;
		}
		
		/* Connect the ports.  You can't do this before the client is
		* activated, because we can't make connections to clients
		* that aren't running.  Note the confusing (but necessary)
		* orientation of the driver backend ports: playback ports are
		* "input" to the backend, and capture ports are "output" from
		* it.
		*/
		
		const char **ports = nullptr;
		ports = jack_get_ports (client, NULL, NULL,
					JackPortIsPhysical|JackPortIsInput);
		if (ports == NULL) {
			fprintf(stderr, "no physical playback ports\n");
			jack_client_close(client);
			return false;
		}
		
		if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
			fprintf (stderr, "cannot connect output ports\n");
			jack_client_close(client);
			return false;
		}
		
		if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
			fprintf (stderr, "cannot connect output ports\n");
			jack_client_close(client);
			return false;
		}
		
		jack_free (ports);
		
		mix_rate = jack_get_sample_rate(client);
		buffer_frames = jack_get_buffer_size(client);
		jack_set_port_registration_callback(client, _port_registration_callback, nullptr);

		step_frames = buffer_frames;
		active = true;
		_update_midi_devices();

		return true;
	}
	
	virtual void finish() {
		ERR_FAIL_COND(!active);
		active = false;
		jack_client_close(client);
	}

	virtual int get_mix_rate() const {
		return mix_rate;
	}

	virtual int get_buffer_size() const {
		return buffer_frames;
	}

	virtual int get_step_size() const {
		return step_frames;
	}

	SoundDriverJack() {
	}
	~SoundDriverJack() {}
};


static SoundDriverJack *sound_driver_jack = nullptr;

void register_jack_driver() {
	
	sound_driver_jack = new SoundDriverJack;
	SoundDriverManager::register_driver(sound_driver_jack);
}

void cleanup_jack_driver() {
	delete sound_driver_jack;
}
