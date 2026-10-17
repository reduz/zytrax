#ifdef OSX_ENABLED

#include "audio_effect_provider_au.h"

#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

AudioEffectProviderAU *AudioEffectProviderAU::singleton = nullptr;

String AudioEffectProviderAU::get_id() const { return "AU"; }
String AudioEffectProviderAU::get_name() const { return "Audio Unit"; }

// ── unique ID ─────────────────────────────────────────────────────────────────

static String make_unique_id(const AudioComponentDescription &desc) {
	// Format: AU_TTTTTTTT_SSSSSSSS_MMMMMMMM (type/subtype/manufacturer as hex).
	// Hex keeps the id pure-ASCII so it round-trips through the wchar_t String
	// and JSON save/load, and is lossless for any 4-char code (printable or not).
	char buf[40];
	snprintf(buf, sizeof(buf), "AU_%08X_%08X_%08X",
			(unsigned)desc.componentType,
			(unsigned)desc.componentSubType,
			(unsigned)desc.componentManufacturer);
	String id;
	id.parse_utf8(buf);
	return id;
}

// ── scan ─────────────────────────────────────────────────────────────────────

static void scan_component_type(OSType type, AudioEffectFactory *p_factory,
		AudioEffectProvider::ScanCallback p_callback, void *p_userdata,
		bool is_synth, const char *category) {

	AudioComponentDescription desc;
	memset(&desc, 0, sizeof(desc));
	desc.componentType         = type;
	desc.componentSubType      = 0;
	desc.componentManufacturer = 0;
	desc.componentFlags        = 0;
	desc.componentFlagsMask    = 0;

	AudioComponent comp = nullptr;
	while ((comp = AudioComponentFindNext(comp, &desc)) != nullptr) {
		AudioComponentDescription compDesc;
		if (AudioComponentGetDescription(comp, &compDesc) != noErr) continue;

		CFStringRef cfName = nullptr;
		if (AudioComponentCopyName(comp, &cfName) != noErr || !cfName) continue;

		NSString *nsName = (__bridge_transfer NSString *)cfName;

		AudioEffectInfo info;
		info.synth            = is_synth;
		info.has_ui           = true; // checked lazily at instantiation
		info.internal         = false;
		info.unique_ID        = make_unique_id(compDesc);
		info.provider_id      = "AU";
		info.provider_caption = "Audio Unit";

		info.caption.parse_utf8([nsName UTF8String]);

		// Store descriptor encoded in the path field so we can re-find the
		// component on instantiation without re-scanning.
		char pathBuf[64];
		snprintf(pathBuf, sizeof(pathBuf), "%u_%u_%u",
				(unsigned)compDesc.componentType,
				(unsigned)compDesc.componentSubType,
				(unsigned)compDesc.componentManufacturer);
		info.path.parse_utf8(pathBuf);

		info.category.parse_utf8(category);

		if (p_callback) {
			p_callback(info.caption, p_userdata);
		}
		p_factory->add_audio_effect(info);
	}
}

void AudioEffectProviderAU::scan_effects(AudioEffectFactory *p_factory,
		ScanCallback p_callback, void *p_userdata) {

	scan_component_type(kAudioUnitType_MusicDevice,  p_factory, p_callback, p_userdata,
			true,  "AU Instruments");
	scan_component_type(kAudioUnitType_Effect,        p_factory, p_callback, p_userdata,
			false, "AU Effects");
	scan_component_type(kAudioUnitType_MusicEffect,   p_factory, p_callback, p_userdata,
			false, "AU Effects");
}

// ── instantiate ───────────────────────────────────────────────────────────────

AudioEffect *AudioEffectProviderAU::instantiate_effect(const AudioEffectInfo *p_info) {
	// Reconstruct descriptor from unique_ID: "AU_TTTTTTTT_SSSSSSSS_MMMMMMMM" (hex).
	const String &uid = p_info->unique_ID;
	std::string uid_str = uid.utf8().get_data();
	printf("AU instantiate: uid='%s' name='%s'\n",
			uid_str.c_str(), p_info->caption.utf8().get_data());

	unsigned t = 0, s = 0, m = 0;
	if (sscanf(uid_str.c_str(), "AU_%08X_%08X_%08X", &t, &s, &m) != 3) {
		printf("AU instantiate: bad unique_ID format\n");
		return nullptr;
	}

	AudioComponentDescription desc;
	memset(&desc, 0, sizeof(desc));
	desc.componentType         = t;
	desc.componentSubType      = s;
	desc.componentManufacturer = m;

	AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
	if (!comp) {
		printf("AU instantiate: component not found (type=%u sub=%u mfr=%u)\n", t, s, m);
		return nullptr;
	}

	AudioEffectAU *fx = new AudioEffectAU();
	Error err = fx->open((void *)comp, p_info->unique_ID,
			p_info->caption, p_info->provider_id, p_info->synth);
	if (err != OK) {
		printf("AU instantiate: open() failed with error %d\n", (int)err);
		delete fx;
		return nullptr;
	}
	printf("AU instantiate: success\n");
	return fx;
}

AudioEffectProviderAU::AudioEffectProviderAU() {
	singleton = this;
}

AudioEffectProviderAU::~AudioEffectProviderAU() {
	if (singleton == this) singleton = nullptr;
}

#endif // OSX_ENABLED
