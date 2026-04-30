#include "synth_chionic.h"
#include <algorithm>
#include "dsp/db.h"

void SynthChionic::ChionicVoice::note_on(int8_t p_note, float p_velocity,int p_offset) {
	base_note = p_note;
	velocity = p_velocity;
	status = STATUS_ON;

	for(int i=0;i<MAX_GENERATORS;i++) {
		Generator &gen = patch->generators[i];

		if (gen.type == GEN_TYPE_DISABLED) {
			continue;
		}

		GenData &gd = gendata[i];

		gd.position = 0;
		gd.shape = gen.oscillator_shape;
		gd.vol_l = 0;
		gd.vol_r = 0;
		gd.punch_time = 0;
		gd.filter_count = 0;
		gd.first_mix = true;
		for(int j=0;j<MAX_FILTERS;j++) {
			if (gen.filters[j].enabled) {
				gd.filters[gd.filter_count].set_mode(gen.filters[j].mode);
				gd.filters[gd.filter_count].set_cutoff(gen.filters[j].cutoff_hz);
				gd.filters[gd.filter_count].set_resonance(gen.filters[j].resonance);
				gd.filters[gd.filter_count].set_stages(gen.filter_stages);
				gd.filters[gd.filter_count].set_sampling_rate(patch->get_sampling_rate());
				gd.filter_count++;
			}
		}

		for(int j=0;j<ENVELOPE_LFO_TYPE_MAX;j++) {

			GenData::EnvelopeProcess &ep = gd.envelope[j];

			ep.position = 0;
			ep.value = gen.envelopes[j].get_height_at_offset(0);

			gd.randomness[j] = gen.randomness[j] * (rand() % 65535) / 65535.0;
			if (j != ENVELOPE_LFO_TYPE_AMPLITUDE) {
				if (rand() & 1) {
					gen.randomness[j]*=-1.0;
				}
			}

			gd.lfo_process[j].position = 0;
			gd.lfo_process[j].value = 0;
		}

		gd.write_out = true;

		for(int j=i+1;j<MAX_GENERATORS;j++) {
			if (patch->generators[j].modulation_source == i) {
				gd.write_out = false;
				break;
			}
		}

	}


}

void SynthChionic::ChionicVoice::note_off(float p_velocity,int p_offset) {
	status = STATUS_OFF;
}

void SynthChionic::ChionicVoice::note_aftertouch(float p_velocity,int p_offset) {

}

void SynthChionic::ChionicVoice::set_volume(float p_volume) {

}

void SynthChionic::ChionicVoice::set_pan(float p_pan) {

}

void SynthChionic::ChionicVoice::set_pitch_offset(float p_notes) {

}

void SynthChionic::ChionicVoice::kill() {

	status = STATUS_DISABLED;
}


SynthBase::Voice::Status SynthChionic::ChionicVoice::get_status() const {

	return status;
}

int8_t SynthChionic::ChionicVoice::get_note() const {
	return int8_t(base_note);
}

template<OscillatorType osc_type>
void mix_oscillator(AudioFrame *p_to_buffer,double &position, double delta,int p_frames, float &vol_l, float &vol_r, float vol_adv_l, float vol_adv_r, float &shape, float shape_adv) {

	for(int i=0;i<p_frames;i++) {
		float s = oscillator_eval<osc_type>(position - floor(position),shape);
		p_to_buffer[i].l = vol_l * s;
		p_to_buffer[i].r = vol_r * s;

		position += delta;
		vol_l += vol_adv_l;
		vol_r += vol_adv_r;
		shape += shape_adv;
	}
}

template<OscillatorType osc_type,SynthChionic::ModulationMode p_modulation>
void mix_oscillator_modulate(const AudioFrame *p_mod_source, AudioFrame *p_to_buffer,double &position, double delta,int p_frames, float &vol_l, float &vol_r, float vol_adv_l, float vol_adv_r, float &shape, float shape_adv) {

	for(int i=0;i<p_frames;i++) {
		float s;
		switch(p_modulation) {
			case SynthChionic::MODULATION_ADD: {
				s = oscillator_eval<osc_type>(position - floor(position),shape);
				p_to_buffer[i].l = vol_l * (s + p_mod_source[i].l);
				p_to_buffer[i].r = vol_r * (s + p_mod_source[i].r);
			} break;
			case SynthChionic::MODULATION_RING: {
				s = oscillator_eval<osc_type>(position - floor(position),shape);
				p_to_buffer[i].l = vol_l * s * p_mod_source[i].l;
				p_to_buffer[i].r = vol_r * s * p_mod_source[i].r;
			} break;
			case SynthChionic::MODULATION_FM: {
				p_to_buffer[i].l = oscillator_eval<osc_type>(position - floor(position) + p_mod_source[i].l,shape) * vol_l;
				p_to_buffer[i].r = oscillator_eval<osc_type>(position - floor(position) + p_mod_source[i].r,shape) * vol_r;
			} break;
		}


		position += delta;
		vol_l += vol_adv_l;
		vol_r += vol_adv_r;
		shape += shape_adv;
	}
}

template<int p_stages,bool p_add>
_FORCE_INLINE_ static void process_filter(const AudioFrame *p_in, AudioFrame *p_out, int p_frames,Filter::Processor filter_processors[SynthChionic::MAX_FILTER_STAGES][2]) {

	for(int i=0;i<p_frames;i++) {
		AudioFrame s = p_in[i];
		for(int j=0;j<p_stages;j++) {
			filter_processors[j][0].process_one_interp(s.l);
			filter_processors[j][1].process_one_interp(s.r);
		}
		if (p_add) {
			p_out[i] += s;
		} else {
			p_out[i] = s;
		}
	}
}

void SynthChionic::ChionicVoice::add_to_mix(AudioFrame *p_out) {

	int active_count = 0;
	for(int i=0;i<MAX_GENERATORS;i++) {
		GenData &gd = gendata[i];
		Generator &gen = patch->generators[i];
		if (gen.type == GEN_TYPE_DISABLED) {
			continue;
		}

		bool active = true;
		// process envelopes
		float env_inc = float(patch->mix_buffer_size) / patch->get_sampling_rate();
		for(int j=0;j<ENVELOPE_LFO_TYPE_MAX;j++) {
			GenData::EnvelopeProcess &ep = gd.envelope[j];

			// process envelope
			ep.position += env_inc;

			if (status == STATUS_ON) {

				if (gen.envelopes[j].has_sustain_loop()) {
					float loop_end_pos = gen.envelopes[j].get_point(gen.envelopes[j].get_sustain_loop_end()).offset;
					if (ep.position > loop_end_pos) {
						float loop_begin_pos = gen.envelopes[j].get_point(gen.envelopes[j].get_sustain_loop_begin()).offset;
						if (loop_begin_pos == loop_end_pos) {
							ep.position = loop_begin_pos;
						} else {
							ep.position = loop_begin_pos + fmodf(ep.position - loop_begin_pos, loop_end_pos - loop_begin_pos);
						}
					}
				}
			}

			if (gen.envelopes[j].has_loop()) {
				float loop_end_pos = gen.envelopes[j].get_point(gen.envelopes[j].get_loop_end()).offset;
				if (ep.position > loop_end_pos) {
					float loop_begin_pos = gen.envelopes[j].get_point(gen.envelopes[j].get_loop_begin()).offset;
					if (loop_begin_pos == loop_end_pos) {
						ep.position = loop_begin_pos;
					} else {
						ep.position = loop_begin_pos + fmodf(ep.position - loop_begin_pos, loop_end_pos - loop_begin_pos);
					}
				}
			}

			ep.value = gen.envelopes[j].get_height_at_offset(ep.position);

			if (status == STATUS_OFF && j == ENVELOPE_LFO_TYPE_AMPLITUDE && ep.position > gen.envelopes[ENVELOPE_LFO_TYPE_AMPLITUDE].get_length() && ep.value < 0.001) {
				active=false;
			}

			gd.lfo_process[j].position+= env_inc;

			if (gen.lfos[j].depth > 0) {
				double real_pos = gd.lfo_process[j].position - gen.lfos[j].delay;
				if (real_pos > 0) {
					float phase = real_pos * gen.lfos[j].rate;
					phase -= floorf(phase);

#define LFO_OSC(m_osc) if (gen.lfos[j].type == m_osc) { gd.lfo_process[j].value = oscillator_eval<m_osc>(phase,0); } else
					LFO_OSC(OSC_TYPE_SINE)
					LFO_OSC(OSC_TYPE_SQUARE)
					LFO_OSC(OSC_TYPE_TRIANGLE)
					LFO_OSC(OSC_TYPE_SAW)
					{}
#undef LFO_OSC

					gd.lfo_process[j].value *= gen.lfos[j].depth;
				} else {
					gd.lfo_process[j].value = 0;
				}
			} else {
				gd.lfo_process[j].value = 0;
			}
			// process LFO
		}

		float punch = 1.0;
		if (gen.punch_amount_db >= 0.0 && gd.punch_time < gen.punch_time_sec) {
			float pa = MAX(0.0, 1.0 - gd.punch_time / gen.punch_time_sec);
			punch = db2linear(gen.punch_amount_db * pa);
			gd.punch_time += env_inc;
		}

		float mono_vol = velocity * db2linear( (1.0-gd.envelope[ENVELOPE_LFO_TYPE_AMPLITUDE].value) * -80 + gd.lfo_process[ENVELOPE_LFO_TYPE_AMPLITUDE].value * 24) * db2linear( gen.volume_db ) * db2linear(gd.randomness[ENVELOPE_LFO_TYPE_AMPLITUDE] * -60) * punch;

		float pan = gen.pan + gd.envelope[ENVELOPE_LFO_TYPE_PAN].value + gd.lfo_process[ENVELOPE_LFO_TYPE_PAN].value + gd.randomness[ENVELOPE_LFO_TYPE_PAN];
		pan = CLAMP(pan, -1.0, 1.0) * 0.5 + 0.5;
		float new_vol_l = mono_vol * (1.0-pan);
		float new_vol_r = mono_vol * pan;

		float mb_div = 1.0 / float(patch->mix_buffer_size);
		float vol_adv_l = (new_vol_l - gd.vol_l) * mb_div;
		float vol_adv_r = (new_vol_r - gd.vol_r) * mb_div;

		float pitch_note_offset = gd.pitch_offset + gd.envelope[ENVELOPE_LFO_TYPE_PITCH].value + gd.lfo_process[ENVELOPE_LFO_TYPE_PITCH].value * gd.lfo_process[ENVELOPE_LFO_TYPE_PITCH].value * 12.0 + gen.fine_tune + gen.coarse_tune;
		float pitch_scale = powf(2.0, pitch_note_offset / 12 );

		switch(gen.type) {
			case GEN_TYPE_OSCILLATOR: {

				float new_shape = CLAMP(gen.oscillator_shape + gd.envelope[ENVELOPE_LFO_TYPE_SHAPE].value + gd.randomness[ENVELOPE_LFO_TYPE_SHAPE] + velocity * gen.shape_velocity_ratio + gd.lfo_process[ENVELOPE_LFO_TYPE_SHAPE].value, 0.0, 1.0);
				float shape_adv = (new_shape - gd.shape) * mb_div;
				double freq = 440.0 * powf(2.0, (base_note - 69.0) / 12.0) * pitch_scale;
				float increment = freq / patch->get_sampling_rate();

				if (gen.modulation_source != -1 && patch->generators[gen.modulation_source].type != GEN_TYPE_DISABLED) {
					// use modulation
					const AudioFrame * mod_frames = patch->voice_mix_buffer[gen.modulation_source];

#define MIX_TEST_OSC(m_osc,m_mod) if (gen.oscillator_type == m_osc) mix_oscillator_modulate<m_osc,m_mod>(mod_frames,patch->voice_mix_buffer[i],gd.position,increment,patch->mix_buffer_size, gd.vol_l,gd.vol_r,vol_adv_l,vol_adv_r,gd.shape,shape_adv); else

#define MIX_MOD(m_mod) if( m_mod == gen.modulation_mode) { MIX_TEST_OSC(OSC_TYPE_SINE,m_mod) MIX_TEST_OSC(OSC_TYPE_SQUARE,m_mod) MIX_TEST_OSC(OSC_TYPE_TRIANGLE,m_mod) MIX_TEST_OSC(OSC_TYPE_SAW,m_mod) {} } else
					MIX_MOD(MODULATION_ADD)
					MIX_MOD(MODULATION_RING)
					MIX_MOD(MODULATION_FM)
					{}
#undef MIX_TEST_OSC
#undef MIX_MOD
				} else {

#define MIX_TEST_OSC(m_osc) if (gen.oscillator_type == m_osc) mix_oscillator<m_osc>(patch->voice_mix_buffer[i],gd.position,increment,patch->mix_buffer_size, gd.vol_l,gd.vol_r,vol_adv_l,vol_adv_r,gd.shape,shape_adv); else
					MIX_TEST_OSC(OSC_TYPE_SINE)
					MIX_TEST_OSC(OSC_TYPE_SQUARE)
					MIX_TEST_OSC(OSC_TYPE_TRIANGLE)
					MIX_TEST_OSC(OSC_TYPE_SAW)
					{}
#undef MIX_TEST_OSC
				}
			} break;
			default: {}
		}

		// filter comes later

		if (gd.filter_count > 0) {
			// need to mix from filter buffer to actual buffer now.
			SWAP(patch->filter_buffer,patch->voice_mix_buffer[i]);

			float filter_cutoff_offset = gd.envelope[ENVELOPE_LFO_TYPE_FILTER].value * 10.0 + (SIGN(gd.randomness[ENVELOPE_LFO_TYPE_FILTER]) * gd.randomness[ENVELOPE_LFO_TYPE_FILTER] * gd.randomness[ENVELOPE_LFO_TYPE_FILTER] * 10) + gen.filter_velocity_ratio * velocity * 10.0 + gd.lfo_process[ENVELOPE_LFO_TYPE_FILTER] .value * 5.0;
			float filter_cutoff_scale = powf(2.0, filter_cutoff_offset );

			for(int j=0;j<gd.filter_count;j++) {
				::Filter f = gd.filters[j];
				f.set_cutoff( CLAMP( gd.filters[j].get_cutoff() * filter_cutoff_scale, 20, 16000) );

				for(int k=0;k<f.get_stages();k++) {

					if (gd.first_mix) {
						for(int l=0;l<2;l++) {
							gd.filter_processor[j][k][l].set_filter(&f,true);
							gd.filter_processor[j][k][l].update_coeffs(0);
						}
					} else {
						for(int l=0;l<2;l++) {
							gd.filter_processor[j][k][l].set_filter(&f,false);
							gd.filter_processor[j][k][l].update_coeffs(patch->mix_buffer_size);
						}
					}
				}

				if (j==0) {
					switch(f.get_stages()) {
						case 1: process_filter<1,false>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 2: process_filter<2,false>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 3: process_filter<3,false>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 4: process_filter<4,false>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
					}
				} else {
					switch(f.get_stages()) {
						case 1: process_filter<1,true>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 2: process_filter<2,true>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 3: process_filter<3,true>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
						case 4: process_filter<4,true>(patch->filter_buffer,patch->voice_mix_buffer[i],patch->mix_buffer_size,gd.filter_processor[j]); break;
					}
				}
			}
		}

		if (gd.write_out) {
			for(int j=0;j<patch->mix_buffer_size;j++) {
				p_out[j] += patch->voice_mix_buffer[i][j];
			}
		}

		if (active) {
			active_count++;
		}

		gd.first_mix = false;
	}

	if (active_count == 0) {
		status = STATUS_DISABLED;
	}
}

SynthChionic::ChionicVoice::~ChionicVoice() {

}

void SynthChionic::_block_size_changed(int p_block_size) {
	mix_buffer_size = p_block_size;

	if (filter_buffer) {
		free(filter_buffer);
	}
	for(int i=0;i<MAX_GENERATORS;i++) {
		if (voice_mix_buffer[i]) {
			free(voice_mix_buffer[i]);
		}

		voice_mix_buffer[i] = (AudioFrame*)malloc(p_block_size * sizeof(AudioFrame));
	}

	filter_buffer = (AudioFrame*)malloc(p_block_size * sizeof(AudioFrame));

}

void SynthChionic::set_generator_type(int p_generator,GeneratorType p_type) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	generators[p_generator].type = p_type;
}

SynthChionic::GeneratorType SynthChionic::get_generator_type(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, GEN_TYPE_DISABLED);
	return generators[p_generator].type;

}

void SynthChionic::set_modulation_source(int p_generator,int p_from) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_COND(p_from < -1 || p_from >= p_generator);
	generators[p_generator].modulation_source = p_from;
}

int SynthChionic::get_modulation_source(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, -1);
	return generators[p_generator].modulation_source;

}

void SynthChionic::set_modulation_mode(int p_generator,ModulationMode p_mode) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	generators[p_generator].modulation_mode = p_mode;
}

SynthChionic::ModulationMode SynthChionic::get_modulation_mode(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, MODULATION_ADD);
	return generators[p_generator].modulation_mode;
}

void SynthChionic::set_oscillator_type(int p_generator,OscillatorType p_type){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_INDEX(p_type, OSC_TYPE_MAX);
	generators[p_generator].oscillator_type= p_type;

}

OscillatorType SynthChionic::get_oscillator_type(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, OSC_TYPE_MAX);
	return generators[p_generator].oscillator_type;

}

void SynthChionic::set_oscillator_shape(int p_generator,float p_shape){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_shape = std::clamp(p_shape,0.0f, 1.0f);

	generators[p_generator].oscillator_shape= p_shape;

}

float SynthChionic::get_oscillator_shape(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].oscillator_shape;
}

void SynthChionic::set_coarse_tune(int p_generator,int p_coarse_tune) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	generators[p_generator].coarse_tune= p_coarse_tune;

}

int SynthChionic::get_coarse_tune(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].coarse_tune;
}

void SynthChionic::set_fine_tune(int p_generator,float p_fine_tune) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	generators[p_generator].fine_tune= p_fine_tune;

}

float SynthChionic::get_fine_tune(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].fine_tune;

}

void SynthChionic::set_volume_db(int p_generator,float p_volume_db){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_volume_db = std::clamp(p_volume_db,-60.0f, 24.0f);

	generators[p_generator].volume_db= p_volume_db;

}

float SynthChionic::get_volume_db(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].volume_db;

}


void SynthChionic::set_pan(int p_generator,float p_pan){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_pan = std::clamp(p_pan,-1.0f, 1.0f);
	generators[p_generator].pan= p_pan;

}

float SynthChionic::get_pan(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].pan;

}

void SynthChionic::set_max_velocity(int p_generator,float p_max_velocity){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_max_velocity = std::clamp(p_max_velocity,0.0f, 1.0f);
	generators[p_generator].max_velocity= p_max_velocity;

}

float SynthChionic::get_max_velocity(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].max_velocity;

}

void SynthChionic::set_min_velocity(int p_generator,float p_min_velocity){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_min_velocity = std::clamp(p_min_velocity,0.0f, 1.0f);
	generators[p_generator].min_velocity= p_min_velocity;

}

float SynthChionic::get_min_velocity(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].min_velocity;

}

void SynthChionic::set_envelope(int p_generator, EnvelopeLFOType p_type, const Envelope& p_envelope){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_INDEX(int(p_type), ENVELOPE_LFO_TYPE_MAX);
	generators[p_generator].envelopes[p_type] = p_envelope;

}

SynthChionic::Envelope SynthChionic::get_envelope(int p_generator, EnvelopeLFOType p_type) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, Envelope());
	ERR_FAIL_INDEX_V(int(p_type), ENVELOPE_LFO_TYPE_MAX, Envelope());
	return generators[p_generator].envelopes[p_type];
}

void SynthChionic::set_lfo(int p_generator, EnvelopeLFOType p_type, const LFO& p_lfo){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_INDEX(int(p_type), ENVELOPE_LFO_TYPE_MAX);
	generators[p_generator].lfos[p_type] = p_lfo;

}

SynthChionic::LFO SynthChionic::get_lfo(int p_generator, EnvelopeLFOType p_type) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, LFO());
	ERR_FAIL_INDEX_V(int(p_type), ENVELOPE_LFO_TYPE_MAX, LFO());
	return generators[p_generator].lfos[p_type];

}

void SynthChionic::set_randomness(int p_generator, EnvelopeLFOType p_type, float p_randomness){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_INDEX(int(p_type), ENVELOPE_LFO_TYPE_MAX);
	p_randomness = std::clamp(p_randomness,0.0f, 1.0f);

	generators[p_generator].randomness[p_type] = p_randomness;

}

float SynthChionic::get_randomness(int p_generator, EnvelopeLFOType p_type) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	ERR_FAIL_INDEX_V(int(p_type), ENVELOPE_LFO_TYPE_MAX, 0);
	return generators[p_generator].randomness[p_type];

}

void SynthChionic::set_filter(int p_generator, int p_index, const Filter& p_filter){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_INDEX(p_index, MAX_FILTERS)
	generators[p_generator].filters[p_index] = p_filter;

}

SynthChionic::Filter SynthChionic::get_filter(int p_generator, int p_index) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, Filter());
	ERR_FAIL_INDEX_V(p_index, MAX_FILTERS, Filter());

	return generators[p_generator].filters[p_index];
}


void SynthChionic::set_filter_velocity_ratio(int p_generator, float p_ratio) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);

	generators[p_generator].filter_velocity_ratio = std::clamp(p_ratio,-1.0f,1.0f);
}

float SynthChionic::get_filter_velocity_ratio(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].filter_velocity_ratio;
}

void SynthChionic::set_shape_velocity_ratio(int p_generator, float p_ratio) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	generators[p_generator].shape_velocity_ratio = std::clamp(p_ratio,0.0f,1.0f);

}

float SynthChionic::get_shape_velocity_ratio(int p_generator) const {
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].shape_velocity_ratio;
}


void SynthChionic::set_filter_stages(int p_generator, int p_stages) {
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	ERR_FAIL_COND(p_stages <1 || p_stages > 4);
	generators[p_generator].filter_stages = p_stages;

}

int SynthChionic::get_filter_stages(int p_generator) const {
	return generators[p_generator].shape_velocity_ratio;
	return generators[p_generator].filter_stages;
}


void SynthChionic::set_punch_amount(int p_generator,float p_punch_amount){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_punch_amount = std::clamp(p_punch_amount,0.0f, 24.0f);
	generators[p_generator].punch_amount_db = p_punch_amount;

}

float SynthChionic::get_punch_amount(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);

	return generators[p_generator].punch_amount_db;

}

void SynthChionic::set_punch_time(int p_generator,float p_punch_time_sec){
	ERR_FAIL_INDEX(p_generator, MAX_GENERATORS);
	p_punch_time_sec = std::clamp(p_punch_time_sec,0.0f, 1.0f);
	generators[p_generator].punch_time_sec = p_punch_time_sec;

}

float SynthChionic::get_punch_time(int p_generator) const{
	ERR_FAIL_INDEX_V(p_generator, MAX_GENERATORS, 0);
	return generators[p_generator].punch_time_sec;
}



void SynthChionic::reset() {
	_reset_midi();
}

int SynthChionic::_get_internal_control_port_count() const {
	return 0;
}

ControlPort *SynthChionic::_get_internal_control_port(int p_index){

	return nullptr;
}

JSON::Node SynthChionic::_internal_to_json() const{
	JSON::Node node = JSON::object();


	return node;
}

Error SynthChionic::_internal_from_json(const JSON::Node &node){

	return OK;
}

SynthBase::Voice *SynthChionic::create_voice() {

	ChionicVoice *cv = new ChionicVoice;
	cv->patch = this;
	return cv;
}


/*info */
String SynthChionic::get_name() const{
	return "Chionic";
}

String SynthChionic::get_unique_id() const{
	return "chionic";
}

String SynthChionic::get_provider_id() const{
	return "internal";
}


SynthChionic::SynthChionic() {
	generators[0].type = GEN_TYPE_OSCILLATOR;
	for(int i=0;i<MAX_GENERATORS;i++) {
		generators[i].envelopes[ENVELOPE_LFO_TYPE_AMPLITUDE].add_position(1.0,0.0);
		generators[i].envelopes[ENVELOPE_LFO_TYPE_AMPLITUDE].set_use_sustain_loop(true);
	}
	_setup();
}

SynthChionic::~SynthChionic() {

	if (filter_buffer) {
		free(filter_buffer);
	}

	for(int i=0;i<MAX_GENERATORS;i++) {
		if (voice_mix_buffer[i]) {
			free(voice_mix_buffer[i]);
		}
	}

}
