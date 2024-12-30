#include "effects.h"

#include "effects/internal/effect_amplifier.h"
#include "effects/internal/effect_chorus.h"
#include "effects/internal/effect_compressor.h"
#include "effects/internal/effect_delay.h"
#include "effects/internal/effect_equalizer.h"
#include "effects/internal/effect_filter.h"
#include "effects/internal/effect_note_puncher.h"
#include "effects/internal/effect_panner.h"
#include "effects/internal/effect_phaser.h"
#include "effects/internal/effect_reverb.h"
#include "effects/internal/effect_stereo_enhancer.h"
#include "effects/internal/effect_midi_device.h"
#include "effects/internal/effect_distortion.h"
#include "effects/internal/synth_sinewave.h"
#include "effects/sf2/synth_sf2.h"
#include "gui/interface.h"

class AudioEffectProviderInternal : public AudioEffectProvider {

public:
	virtual AudioEffect *instantiate_effect(const AudioEffectInfo *p_info) {
		if (p_info->unique_ID == "reverb") {
			return new AudioEffectReverb;
		}
		if (p_info->unique_ID == "chorus") {
			return new AudioEffectChorus;
		}

		if (p_info->unique_ID == "compressor") {
			return new AudioEffectCompressor(false);
		}
		if (p_info->unique_ID == "sc_compressor") {
			return new AudioEffectCompressor(true);
		}

		if (p_info->unique_ID == "bpm_delay") {
			return new AudioEffectDelay(true);
		}
		if (p_info->unique_ID == "delay") {
			return new AudioEffectDelay(false);
		}
		if (p_info->unique_ID == "eq_6") {
			return new AudioEffectEqualizer(EQ::PRESET_6_BANDS);
		}
		if (p_info->unique_ID == "eq_10") {
			return new AudioEffectEqualizer(EQ::PRESET_10_BANDS);
		}
		if (p_info->unique_ID == "eq_21") {
			return new AudioEffectEqualizer(EQ::PRESET_21_BANDS);
		}
		if (p_info->unique_ID == "panner") {
			return new AudioEffectPanner;
		}
		if (p_info->unique_ID == "amplifier") {
			return new AudioEffectAmplifier;
		}
		if (p_info->unique_ID == "distortion") {
			return new AudioEffectDistortion;
		}
		if (p_info->unique_ID == "stereo_enhancer") {
			return new AudioEffectStereoEnhancer;
		}
		if (p_info->unique_ID == "phaser") {
			return new AudioEffectPhaser;
		}

		if (p_info->unique_ID == "filter_band_pass") {
			return new AudioEffectFilter(Filter::BANDPASS);
		}
		if (p_info->unique_ID == "filter_high_pass") {
			return new AudioEffectFilter(Filter::HIGHPASS);
		}
		if (p_info->unique_ID == "filter_low_pass") {
			return new AudioEffectFilter(Filter::LOWPASS);
		}
		if (p_info->unique_ID == "filter_notch") {
			return new AudioEffectFilter(Filter::NOTCH);
		}
		if (p_info->unique_ID == "filter_peak") {
			return new AudioEffectFilter(Filter::PEAK);
		}
		if (p_info->unique_ID == "filter_band_limit") {
			return new AudioEffectFilter(Filter::BANDLIMIT);
		}
		if (p_info->unique_ID == "filter_low_shelf") {
			return new AudioEffectFilter(Filter::LOWSHELF);
		}
		if (p_info->unique_ID == "filter_high_shelf") {
			return new AudioEffectFilter(Filter::HIGHSHELF);
		}
		if (p_info->unique_ID == "synth_sinewave") {
			return new SynthSinewave;
		}
		if (p_info->unique_ID == "sf2") {
			return new AudioSynthSF2;
		}
		if (p_info->unique_ID == "midi_device") {
			return new AudioEffectMIDIDevice;
		}

		return NULL;
	}
	virtual void scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata) {
		//these are not scanned
	}
	virtual String get_id() const {
		return "internal";
	}
	String get_name() const {
		return "Internal";
	}
};

AudioEffectProviderInternal internal_provider;

void register_effects(AudioEffectFactory *p_factory) {
	p_factory->add_provider(&internal_provider);
	{
		//Reverb
		AudioEffectInfo info;
		info.caption = "Reverb";
		info.description = "Standard Comb/Allpass filter based reverb.";
		info.author = "Juan Linietsky";
		info.unique_ID = "reverb";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Chorus
		AudioEffectInfo info;
		info.caption = "Chorus";
		info.description = "Standard LFO-Based Multi-Voice chorus.";
		info.author = "Juan Linietsky";
		info.unique_ID = "chorus";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//Compressor
		AudioEffectInfo info;
		info.caption = "Compressor";
		info.description = "Standard Threshold/Ratio envelope based compressor";
		info.author = "Juan Linietsky";
		info.unique_ID = "compressor";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Sidechain Compressor
		AudioEffectInfo info;
		info.caption = "Compressor (Sidechain)";
		info.description = "Standard Threshold/Ratio envelope based sidechain compressor";
		info.author = "Juan Linietsky";
		info.unique_ID = "sc_compressor";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Delay
		AudioEffectInfo info;
		info.caption = "Delay";
		info.description = "Standard delay with 4 taps";
		info.author = "Juan Linietsky";
		info.unique_ID = "delay";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Delay
		AudioEffectInfo info;
		info.caption = "Delay (BPM)";
		info.description = "Standard BPM-synced delay with 4 taps";
		info.author = "Juan Linietsky";
		info.unique_ID = "bpm_delay";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Equalizer
		AudioEffectInfo info;
		info.caption = "Equalizer (6 Bands)";
		info.description = "Standard Equalizer";
		info.author = "Juan Linietsky";
		info.unique_ID = "eq_6";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Equalizer
		AudioEffectInfo info;
		info.caption = "Equalizer (10 Bands)";
		info.description = "Standard Equalizer";
		info.author = "Juan Linietsky";
		info.unique_ID = "eq_10";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Equalizer
		AudioEffectInfo info;
		info.caption = "Equalizer (21 Bands)";
		info.description = "Standard Equalizer";
		info.author = "Juan Linietsky";
		info.unique_ID = "eq_21";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Panner
		AudioEffectInfo info;
		info.caption = "Panner";
		info.description = "Change panning";
		info.author = "Juan Linietsky";
		info.unique_ID = "panner";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Amplifier
		AudioEffectInfo info;
		info.caption = "Amplifier";
		info.description = "Standard amplifier";
		info.author = "Juan Linietsky";
		info.unique_ID = "amplifier";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Distortion
		AudioEffectInfo info;
		info.caption = "Distortion";
		info.description = "Distortion";
		info.author = "Juan Linietsky";
		info.unique_ID = "distortion";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Stereo Enhancer
		AudioEffectInfo info;
		info.caption = "Stereo Enhancer";
		info.description = "Stereo Enhancer using multiple techniques";
		info.author = "Juan Linietsky";
		info.unique_ID = "stereo_enhancer";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//Phaser
		AudioEffectInfo info;
		info.caption = "Phaser";
		info.description = "Simpler phaser effect";
		info.author = "Juan Linietsky";
		info.unique_ID = "phaser";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//puncher
		AudioEffectInfo info;
		info.caption = "Note Puncher";
		info.description = "Adds a small punch envelope to notes";
		info.author = "Juan Linietsky";
		info.unique_ID = "note_puncher";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//filter
		AudioEffectInfo info;
		info.caption = "Low Pass Filter";
		info.description = "Standard low pass filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_low_pass";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//filter
		AudioEffectInfo info;
		info.caption = "High Pass Filter";
		info.description = "Standard high pass filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_high_pass";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//filter
		AudioEffectInfo info;
		info.caption = "Band Pass Filter";
		info.description = "Standard band pass filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_band_pass";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//filter
		AudioEffectInfo info;
		info.caption = "Notch Filter";
		info.description = "Standard notch filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_notch";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//filter
		AudioEffectInfo info;
		info.caption = "Peak Filter";
		info.description = "Standard peak filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_peak";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//filter
		AudioEffectInfo info;
		info.caption = "Band Limit Filter";
		info.description = "Standard band limit filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_band_limit";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//filter
		AudioEffectInfo info;
		info.caption = "Low Shelf Filter";
		info.description = "Standard low shelf filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_low_shelf";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}

	{
		//filter
		AudioEffectInfo info;
		info.caption = "High Shelf Filter";
		info.description = "Standard high shelf filter";
		info.author = "Juan Linietsky";
		info.unique_ID = "filter_high_shelf";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = false;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//midi
		AudioEffectInfo info;
		info.caption = "MIDI Device";
		info.description = "Output to a MIDI port";
		info.author = "Juan Linietsky";
		info.unique_ID = "midi_device";
		info.provider_caption = "Internal";
		info.category = "Internal MIDI";
		info.version = "1.0";
		info.synth = true;
		info.has_ui = true;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//midi
		AudioEffectInfo info;
		info.caption = "Sinewave Synth";
		info.description = "Play a sinewave";
		info.author = "Juan Linietsky";
		info.unique_ID = "synth_sinewave";
		info.provider_caption = "Internal";
		info.category = "Internal MIDI";
		info.version = "1.0";
		info.synth = true;
		info.has_ui = false;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	{
		//sf2
		AudioEffectInfo info;
		info.caption = "SoundFont Player";
		info.description = "SF2 SoundFont Player";
		info.author = "Bernhard Schelling";
		info.unique_ID = "sf2";
		info.provider_caption = "Internal";
		info.category = "Internal Effects";
		info.version = "1.0";
		info.synth = true;
		info.has_ui = true;
		info.internal = true;
		info.provider_id = "internal";
		p_factory->add_audio_effect(info);
	}
	
}
