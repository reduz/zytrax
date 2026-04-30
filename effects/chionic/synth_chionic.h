#ifndef SYNTH_CHIONIC_H
#define SYNTH_CHIONIC_H

#include "effects/internal/synth_base.h"
#include "dsp/oscillator.h"
#include "dsp/filter.h"

class SynthChionic : public SynthBase {
public:
	enum {
		MAX_GENERATORS = 4,
		MAX_FILTERS = 4,
		MAX_FILTER_STAGES = 4,
		MAX_ENVELOPE_POINTS = 32,
		MAX_COARSE_TUNE = 12,
		MIN_COARSE_TUNE = -12,

	};

	enum GeneratorType {
		GEN_TYPE_DISABLED,
		GEN_TYPE_OSCILLATOR,
		GEN_TYPE_SAMPLER,
		GEN_TYPE_NOISE
	};

	enum ModulationMode {
		MODULATION_ADD,
		MODULATION_RING,
		MODULATION_FM,
	};

	enum EnvelopeLFOType {
		ENVELOPE_LFO_TYPE_AMPLITUDE,
		ENVELOPE_LFO_TYPE_PAN,
		ENVELOPE_LFO_TYPE_FILTER,
		ENVELOPE_LFO_TYPE_PITCH,
		ENVELOPE_LFO_TYPE_SHAPE,
		ENVELOPE_LFO_TYPE_MAX,
	};

	struct EnvelopePoint {
		float offset = 0;
		float value = 0;
	};

	class Envelope {
		EnvelopePoint points[MAX_ENVELOPE_POINTS];
		int point_count = 1;
		bool sustain_loop = false;
		int sustain_loop_begin = 0;
		int sustain_loop_end = 0;
		bool loop = false;
		int loop_begin = 0;
		int loop_end = 0;		
	public:

		_FORCE_INLINE_ int get_point_count() const { return point_count; }
		_FORCE_INLINE_ const EnvelopePoint &get_point(int p_idx) const {
			ERR_FAIL_INDEX_V(p_idx, point_count, points[0]);
			return points[p_idx];
		}


		_FORCE_INLINE_ float get_height_at_offset(float p_offset) const {

			if (point_count<1) return 0;

			if (p_offset == 0.0 || point_count == 1) {
				return points[0].value;
			}

			if (p_offset>points[point_count-1].offset) {
				return points[point_count-1].value;
			}

			float begin_x,begin_y;
			float end_x,end_y,xdif;
			int count=0;
			int limit=-1;


			while ((count<point_count) && (limit==-1)) {

				if (points[count].offset>=p_offset) limit=count;
				count++;
			}

			if (limit==-1) return 0;

			begin_x=points[limit-1].offset;
			end_x=points[limit].offset;
			begin_y=points[limit-1].value;
			end_y=points[limit].value;


			xdif=end_x-begin_x;
			float res = begin_y+((p_offset-begin_x)*(end_y-begin_y))/(xdif?xdif:1);
			//printf("%i -> %i interp (%f,%f) -> (%f,%f): (%f,%f)\n",limit-1,limit,points[limit-1].value,points[limit-1].offset,points[limit].value,points[limit].offset,p_offset,res);
			return res;
		}

		void set_position(int p_point,float p_x,float p_y) {

			if (p_point>=point_count) return;

			if (p_point==0) {

				p_x=0;

			} else if (p_x<=points[p_point-1].offset) {

				p_x=points[p_point-1].offset+0.001;

			} else if ((p_point<(point_count-1)) && (p_x>=points[p_point+1].offset)) {

				p_x=points[p_point+1].offset-0.001;
			}

			if (p_x>=9999) p_x=9999;

			points[p_point].offset=p_x;
			points[p_point].value=p_y;

		}

		int add_position(float p_x,float p_y,bool p_move_loops = true) {

			if (point_count==MAX_ENVELOPE_POINTS) return -1;

			int i,new_point;

			// if this is assigning an existing point, let's quit.
			for (i=0;i<point_count;i++) {
				if (p_x==points[i].offset) {
					return -1;
				}
			}

			i=0;
			while ((i<point_count) && (p_x>=points[i].offset)) i++;

			new_point=i;
			point_count++;

			if (p_move_loops) {
				if (loop_begin>=new_point) loop_begin++;
				if (loop_end>=new_point) loop_end++;
				if (sustain_loop_begin>=new_point) sustain_loop_begin++;
				if (sustain_loop_end>=new_point) sustain_loop_end++;
			}
			for (i=point_count-1;i>new_point;i--) points[i]=points[i-1];


			set_position(new_point,p_x,p_y);

			return new_point;

		}

		void set_use_loop(bool p_loop) {
			loop = p_loop;
		}

		void set_use_sustain_loop(bool p_loop) {
			sustain_loop = p_loop;
		}

		_FORCE_INLINE_ bool has_loop() const {
			return loop;
		}

		_FORCE_INLINE_ bool has_sustain_loop() const {
			return sustain_loop;
		}

		_FORCE_INLINE_ int get_loop_begin() const {
			return loop_begin;
		}

		_FORCE_INLINE_ int get_loop_end() const {
			return loop_end;
		}

		_FORCE_INLINE_ int get_sustain_loop_begin() const {
			return sustain_loop_begin;
		}

		_FORCE_INLINE_ int get_sustain_loop_end() const {
			return sustain_loop_end;
		}

		void set_loop_begin(int pos) {

			if ((pos<0) || (pos>=point_count)) return;

			loop_begin=pos;

			if (loop_end<loop_begin) loop_end=loop_begin;


		}

		void set_loop_end(int pos) {

			if ((pos<0) || (pos>=point_count)) return;

			loop_end=pos;

			if (loop_end<loop_begin) loop_begin=loop_end;


		}


		void set_sustain_loop_begin(int pos) {

			if ((pos<0) || (pos>=point_count)) return;

			sustain_loop_begin=pos;

			if (sustain_loop_end<sustain_loop_begin) sustain_loop_end=sustain_loop_begin;

		}

		void set_sustain_loop_end(int pos) {

			if ((pos<0) || (pos>=point_count)) return;

			sustain_loop_end=pos;

			if (sustain_loop_end<sustain_loop_begin) sustain_loop_begin=sustain_loop_end;

		}

		void del_position(int p_node) {

			if ((point_count<2) || (p_node<=0) || (p_node>=point_count)) return;

			int i;

			if (loop_begin>=p_node) loop_begin--;
			if (loop_end>=p_node) loop_end--;
			if (sustain_loop_begin>=p_node) sustain_loop_begin--;
			if (sustain_loop_end>=p_node) sustain_loop_end--;

			for (i=p_node;i<point_count-1;i++) points[i]=points[i+1];

			point_count--;

		}

		_FORCE_INLINE_ float get_length() const {
			return points[point_count-1].offset;
		}
	};

	struct LFO {
		OscillatorType type = OSC_TYPE_SINE;
		float delay = 0.0;
		float rate = 1.0;
		float depth = 0.0;
	};

	struct Filter {
		bool enabled = false;
		::Filter::Mode mode = ::Filter::LOWPASS;
		float cutoff_hz = 1000.0;
		float resonance = 0.0;
	};

private:

	struct Generator {

		GeneratorType type = GEN_TYPE_DISABLED;

		int modulation_source = -1;
		ModulationMode modulation_mode = MODULATION_ADD;

		OscillatorType oscillator_type = OSC_TYPE_SINE;

		float oscillator_shape = 0.0;

		int coarse_tune = 0;
		float fine_tune = 0.0;

		float volume_db = 0;
		float pan = 0.0;

		float max_velocity = 1; // for layering
		float min_velocity = 0;

		float filter_velocity_ratio = 0; // -1 to 1
		float shape_velocity_ratio = 0; // 0 to 1

		Filter filters[MAX_FILTERS];
		int filter_stages = 1;

		Envelope envelopes[ENVELOPE_LFO_TYPE_MAX];
		LFO lfos[ENVELOPE_LFO_TYPE_MAX];
		float randomness[ENVELOPE_LFO_TYPE_MAX] = {};

		float punch_amount_db = 0.0;
		float punch_time_sec = 0.1;

		Generator() {
			envelopes[ENVELOPE_LFO_TYPE_AMPLITUDE].set_position(0,0,1);
		}

	};


	Generator generators[MAX_GENERATORS];

	class ChionicVoice : public Voice{

		float velocity = 0;

		Status status = STATUS_DISABLED;
		float base_note = 0;
		struct GenData {
			float vol_l = 0;
			float vol_r = 0;
			double position = 0;
			float shape = 0;
			float pitch_offset = 0;
			float punch_time = 0;

			float randomness[ENVELOPE_LFO_TYPE_MAX];
			::Filter filters[MAX_FILTERS];
			::Filter::Processor filter_processor[MAX_FILTERS][MAX_FILTER_STAGES][2]; //left and right.
			bool first_mix = true;

			int filter_count = 0;
			bool write_out = true;

			struct EnvelopeProcess {
				float position = 0;
				float value = 0;
			} envelope[ENVELOPE_LFO_TYPE_MAX];

			struct LFOProcess {
				double position = 0;
				float value = 0;
			}  lfo_process[ENVELOPE_LFO_TYPE_MAX];

		} gendata[MAX_GENERATORS];

	friend class SynthChionic;
		SynthChionic *patch = nullptr;

	public:
		virtual void note_on(int8_t p_note, float p_velocity,int p_offset);
		virtual void note_off(float p_velocity,int p_offset);
		virtual void note_aftertouch(float p_velocity,int p_offset);

		virtual void set_volume(float p_volume);
		virtual void set_pan(float p_pan); //0 - 1
		virtual void set_pitch_offset(float p_notes); // multiplier

		virtual void kill();

		virtual Status get_status() const;
		virtual int8_t get_note() const;
		virtual void add_to_mix(AudioFrame *p_out);
		virtual ~ChionicVoice();
	};


	AudioFrame *voice_mix_buffer[MAX_GENERATORS] = {};
	AudioFrame *filter_buffer = nullptr;
	int mix_buffer_size = 0;
	virtual void _block_size_changed(int p_block_size);


	virtual Voice *create_voice() override;

public:

	static float envelope_type_get_min(EnvelopeLFOType p_type) {
		switch(p_type) {
			case ENVELOPE_LFO_TYPE_AMPLITUDE: {
				return 0;
			} break;
			case ENVELOPE_LFO_TYPE_PAN: {
				return -1;
			} break;
			case ENVELOPE_LFO_TYPE_FILTER: {
				return -1;
			} break;
			case ENVELOPE_LFO_TYPE_PITCH: {
				return -12;
			} break;
			case ENVELOPE_LFO_TYPE_SHAPE: {
				return 0;
			} break;
		}
		return 0;
	}

	static float envelope_type_get_max(EnvelopeLFOType p_type) {
		switch(p_type) {
			case ENVELOPE_LFO_TYPE_AMPLITUDE: {
				return 1;
			} break;
			case ENVELOPE_LFO_TYPE_PAN: {
				return 1;
			} break;
			case ENVELOPE_LFO_TYPE_FILTER: {
				return 1;
			} break;
			case ENVELOPE_LFO_TYPE_PITCH: {
				return 12;
			} break;
			case ENVELOPE_LFO_TYPE_SHAPE: {
				return 1;
			} break;
		}
		return 0;
	}

	static String envelope_type_get_legend(EnvelopeLFOType p_type,float p_value) {
		switch(p_type) {
			case ENVELOPE_LFO_TYPE_AMPLITUDE: {
				if (p_value == 0.0) {
					return "Silence";
				} else {
					return String::num(p_value * 80.0 - 80.0,2) +" dB";
				}
			} break;
			case ENVELOPE_LFO_TYPE_PAN: {
				if (p_value < -0.97) {
					return "Left";
				} else if (ABS(p_value) < 0.03) {
					return "Center";
				} else if (p_value > 0.97) {
					return "Right";
				} else {
					return String::num(p_value,2);
				}
			} break;
			case ENVELOPE_LFO_TYPE_FILTER: {
				return String::num(p_value * 10,2) + " octaves";
			} break;
			case ENVELOPE_LFO_TYPE_PITCH: {
				return String::num(p_value,2) +" semitones";
			} break;
			case ENVELOPE_LFO_TYPE_SHAPE: {
				return String::num(p_value*100,0) + "%";
			} break;
		}
		return "";
	}


	void set_generator_type(int p_generator,GeneratorType p_type);
	GeneratorType get_generator_type(int p_generator) const;

	void set_modulation_source(int p_generator,int p_from);
	int get_modulation_source(int p_generator) const;

	void set_modulation_mode(int p_generator,ModulationMode p_mode);
	ModulationMode get_modulation_mode(int p_generator) const;

	void set_oscillator_type(int p_generator, OscillatorType p_type);
	OscillatorType get_oscillator_type(int p_generator) const;

	void set_oscillator_shape(int p_generator,float p_shape);
	float get_oscillator_shape(int p_generator) const;

	void set_coarse_tune(int p_generator,int p_coarse_tune);
	int get_coarse_tune(int p_generator) const;

	void set_fine_tune(int p_generator,float p_fine_tune);
	float get_fine_tune(int p_generator) const;

	void set_volume_db(int p_generator,float p_volume_db);
	float get_volume_db(int p_generator) const;

	void set_pan(int p_generator,float p_pan);
	float get_pan(int p_generator) const;

	void set_max_velocity(int p_generator,float p_max_velocity);
	float get_max_velocity(int p_generator) const;

	void set_min_velocity(int p_generator,float p_min_velocity);
	float get_min_velocity(int p_generator) const;

	void set_envelope(int p_generator, EnvelopeLFOType p_type, const Envelope& p_envelope);
	Envelope get_envelope(int p_generator, EnvelopeLFOType p_type) const;

	void set_lfo(int p_generator, EnvelopeLFOType p_type, const LFO& p_lfo);
	LFO get_lfo(int p_generator, EnvelopeLFOType p_type) const;

	void set_randomness(int p_generator, EnvelopeLFOType p_type, float p_randomness);
	float get_randomness(int p_generator, EnvelopeLFOType p_type) const;

	void set_filter(int p_generator, int p_index, const Filter& p_filter);
	Filter get_filter(int p_generator, int p_index) const;

	void set_filter_velocity_ratio(int p_generator, float p_ratio);
	float get_filter_velocity_ratio(int p_generator) const;

	void set_shape_velocity_ratio(int p_generator, float p_ratio);
	float get_shape_velocity_ratio(int p_generator) const;

	void set_filter_stages(int p_generator, int p_stages);
	int get_filter_stages(int p_generator) const;

	void set_punch_amount(int p_generator,float p_punch_amount);
	float get_punch_amount(int p_generator) const;

	void set_punch_time(int p_generator, float p_punch_time_sec);
	float get_punch_time(int p_generator) const;

	virtual void reset();

	virtual int _get_internal_control_port_count() const;
	virtual ControlPort *_get_internal_control_port(int p_index);

	virtual JSON::Node _internal_to_json() const;
	virtual Error _internal_from_json(const JSON::Node &node);

	/*info */
	virtual String get_name() const;
	virtual String get_unique_id() const;
	virtual String get_provider_id() const;

	SynthChionic();
	~SynthChionic();
};

#endif // SYNTH_CHIONIC_H
