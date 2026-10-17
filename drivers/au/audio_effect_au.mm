#ifdef OSX_ENABLED

#include "audio_effect_au.h"
#include "globals/base64.h"

#import <Cocoa/Cocoa.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioUnitUtilities.h>
#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

// ── helpers ──────────────────────────────────────────────────────────────────

static void setup_asbd(AudioStreamBasicDescription &asbd, int sample_rate) {
	memset(&asbd, 0, sizeof(asbd));
	asbd.mSampleRate       = sample_rate;
	asbd.mFormatID         = kAudioFormatLinearPCM;
	asbd.mFormatFlags      = kAudioFormatFlagIsFloat |
	                         kAudioFormatFlagIsNonInterleaved |
	                         kAudioFormatFlagIsPacked;
	asbd.mBitsPerChannel   = 32;
	asbd.mChannelsPerFrame = 2;
	asbd.mFramesPerPacket  = 1;
	asbd.mBytesPerFrame    = sizeof(float);
	asbd.mBytesPerPacket   = sizeof(float);
}

// Input render callback — provides audio to effect AUs.
// Handles whatever buffer layout the AU asks for so we never write past the
// AudioBufferList the AU hands us.
static OSStatus au_input_callback(
		void *inRefCon,
		AudioUnitRenderActionFlags *ioActionFlags,
		const AudioTimeStamp *inTimeStamp,
		UInt32 inBusNumber,
		UInt32 inNumberFrames,
		AudioBufferList *ioData) {

	AudioEffectAU *self = (AudioEffectAU *)inRefCon;

	float *l, *r;
	self->_get_input_buffers(l, r);

	if (ioData->mNumberBuffers >= 2) {
		// Non-interleaved: one buffer per channel.
		if (ioData->mBuffers[0].mData)
			memcpy(ioData->mBuffers[0].mData, l, inNumberFrames * sizeof(float));
		else
			ioData->mBuffers[0].mData = l;

		if (ioData->mBuffers[1].mData)
			memcpy(ioData->mBuffers[1].mData, r, inNumberFrames * sizeof(float));
		else
			ioData->mBuffers[1].mData = r;
	} else if (ioData->mNumberBuffers == 1) {
		// Interleaved (or mono): fill the single buffer the AU provided.
		float *dst = (float *)ioData->mBuffers[0].mData;
		UInt32 ch = ioData->mBuffers[0].mNumberChannels;
		if (dst && ch >= 2) {
			for (UInt32 i = 0; i < inNumberFrames; i++) {
				dst[i * 2]     = l[i];
				dst[i * 2 + 1] = r[i];
			}
		} else if (dst && ch == 1) {
			for (UInt32 i = 0; i < inNumberFrames; i++)
				dst[i] = 0.5f * (l[i] + r[i]);
		}
	}
	return noErr;
}

// ── ControlPortAU ────────────────────────────────────────────────────────────

float AudioEffectAU::ControlPortAU::get() const {
	AudioUnitParameterValue val = 0;
	AudioUnitGetParameter((AudioUnit)unit, (AudioUnitParameterID)param_id,
			kAudioUnitScope_Global, 0, &val);
	return (float)val;
}

void AudioEffectAU::ControlPortAU::set(float p_val) {
	if (setting) return;
	setting = true;
	AudioUnitSetParameter((AudioUnit)unit, (AudioUnitParameterID)param_id,
			kAudioUnitScope_Global, 0, (AudioUnitParameterValue)p_val, 0);
	setting = false;
}

String AudioEffectAU::ControlPortAU::get_value_as_text() const {
	AudioUnitParameterValue val = 0;
	AudioUnitGetParameter((AudioUnit)unit, (AudioUnitParameterID)param_id,
			kAudioUnitScope_Global, 0, &val);

	// Ask the AU for a formatted string (handles indexed names, units, etc.).
	AudioUnitParameterStringFromValue sfv;
	sfv.inParamID = (AudioUnitParameterID)param_id;
	sfv.inValue   = &val;
	sfv.outString = nullptr;
	UInt32 sz = sizeof(sfv);
	if (AudioUnitGetProperty((AudioUnit)unit,
				kAudioUnitProperty_ParameterStringFromValue,
				kAudioUnitScope_Global, (AudioUnitParameterID)param_id,
				&sfv, &sz) == noErr && sfv.outString) {
		NSString *ns = (__bridge_transfer NSString *)sfv.outString;
		String s;
		s.parse_utf8([ns UTF8String]);
		return s;
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "%.4g", (double)val);
	String s;
	s.parse_utf8(buf);
	return s;
}

// ── AudioEffectAU ─────────────────────────────────────────────────────────────

int AudioEffectAU::_get_internal_control_port_count() const {
	return control_ports.size();
}

ControlPort *AudioEffectAU::_get_internal_control_port(int p_index) {
	return &control_ports[p_index];
}

void AudioEffectAU::_alloc_buffers() {
	_free_buffers();
	in_buf_l  = new float[process_block_size]();
	in_buf_r  = new float[process_block_size]();
	out_buf_l = new float[process_block_size]();
	out_buf_r = new float[process_block_size]();
	if (is_interleaved_out)
		interleaved_buf = new float[process_block_size * 2]();
}

void AudioEffectAU::_free_buffers() {
	delete[] in_buf_l;       in_buf_l       = nullptr;
	delete[] in_buf_r;       in_buf_r       = nullptr;
	delete[] out_buf_l;      out_buf_l      = nullptr;
	delete[] out_buf_r;      out_buf_r      = nullptr;
	delete[] interleaved_buf; interleaved_buf = nullptr;
}

void AudioEffectAU::_get_input_buffers(float *&l, float *&r) {
	l = in_buf_l;
	r = in_buf_r;
}

// ── parameter-change listener ──────────────────────────────────────────────────
// Mirrors VST2's audioMasterAutomate: when the plugin's own UI changes a
// parameter, notify the host so automation capture stays in sync.

void AudioEffectAU::_notify_param_changed(unsigned int p_param_id) {
	for (int i = 0; i < control_ports.size(); i++) {
		if (control_ports[i].param_id == p_param_id) {
			control_ports[i].ui_changed_notify();
			return;
		}
	}
}

static void au_event_listener_proc(void *inRefCon, void *inObject,
		const AudioUnitEvent *inEvent, UInt64 inEventHostTime,
		AudioUnitParameterValue inParameterValue) {
	(void)inObject; (void)inEventHostTime; (void)inParameterValue;
	if (inEvent->mEventType != kAudioUnitEvent_ParameterValueChange) return;
	AudioEffectAU *self = (AudioEffectAU *)inRefCon;
	self->_notify_param_changed((unsigned int)inEvent->mArgument.mParameter.mParameterID);
}

void AudioEffectAU::_create_event_listener() {
	_destroy_event_listener();
	if (!unit || control_ports.size() == 0) return;

	AUEventListenerRef listener = nullptr;
	OSStatus st = AUEventListenerCreate(au_event_listener_proc, this,
			CFRunLoopGetMain(), kCFRunLoopDefaultMode,
			0.05f /*notification interval*/, 0.05f /*value-change granularity*/,
			&listener);
	if (st != noErr || !listener) return;

	for (int i = 0; i < control_ports.size(); i++) {
		AudioUnitEvent ev;
		memset(&ev, 0, sizeof(ev));
		ev.mEventType = kAudioUnitEvent_ParameterValueChange;
		ev.mArgument.mParameter.mAudioUnit   = (AudioUnit)unit;
		ev.mArgument.mParameter.mParameterID = (AudioUnitParameterID)control_ports[i].param_id;
		ev.mArgument.mParameter.mScope       = kAudioUnitScope_Global;
		ev.mArgument.mParameter.mElement     = 0;
		AUEventListenerAddEventType(listener, this, &ev);
	}
	event_listener = (void *)listener;
}

void AudioEffectAU::_destroy_event_listener() {
	if (event_listener) {
		AUListenerDispose((AUEventListenerRef)event_listener);
		event_listener = nullptr;
	}
}

void AudioEffectAU::_send_midi_events(const Event *p_events, int p_event_count, float p_time) {
	if (!unit) return;

	int midi_event_count;
	const MIDIEventStamped *midi_events = _process_midi_events(
			p_events, p_event_count, p_time, midi_event_count);

	if (stop_all_notes) {
		for (int i = 0; i < 128; i++) {
			MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
					0x80 | last_midi_channel, i, 0, 0);
		}
		// All notes off
		MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
				0xB0 | last_midi_channel, 0x7B, 0, 0);
		stop_all_notes = false;
	}

	for (int i = 0; i < midi_event_count; i++) {
		const MIDIEvent &ev = midi_events[i].event;
		UInt32 offset = (UInt32)midi_events[i].frame;

		switch (ev.type) {
			case MIDIEvent::MIDI_NOTE_ON:
				last_midi_channel = ev.channel;
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0x90 | ev.channel, ev.note.note, ev.note.velocity, offset);
				break;
			case MIDIEvent::MIDI_NOTE_OFF:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0x80 | ev.channel, ev.note.note, ev.note.velocity, offset);
				break;
			case MIDIEvent::MIDI_CONTROLLER:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0xB0 | ev.channel, ev.control.index, ev.control.parameter, offset);
				break;
			case MIDIEvent::MIDI_PITCH_BEND:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0xE0 | ev.channel,
						ev.pitch_bend.bend & 0x7F, ev.pitch_bend.bend >> 7, offset);
				break;
			case MIDIEvent::MIDI_AFTERTOUCH:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0xD0 | ev.channel, ev.aftertouch.pressure, 0, offset);
				break;
			case MIDIEvent::MIDI_NOTE_PRESSURE:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0xA0 | ev.channel, ev.note.note, ev.note.velocity, offset);
				break;
			case MIDIEvent::MIDI_PATCH_SELECT:
				MusicDeviceMIDIEvent((MusicDeviceComponent)unit,
						0xC0 | ev.channel, ev.note.note, 0, offset);
				break;
			default:
				break;
		}
	}
}

void AudioEffectAU::process(const Event *p_events, int p_event_count,
		const AudioFrame *p_in, AudioFrame *p_out, bool p_prev_active) {

	if (!unit) return;

	// Deinterleave input (only needed when the AU actually consumes audio input)
	if (has_input_bus) {
		for (int i = 0; i < process_block_size; i++) {
			in_buf_l[i] = p_in[i].l;
			in_buf_r[i] = p_in[i].r;
		}
	}

	float time = (float)process_block_size / (float)sampling_rate;
	if (accepts_midi)
		_send_midi_events(p_events, p_event_count, time);

	AudioTimeStamp ts;
	memset(&ts, 0, sizeof(ts));
	ts.mFlags      = kAudioTimeStampSampleTimeValid;
	ts.mSampleTime = (Float64)sample_time;

	AudioUnitRenderActionFlags flags = 0;

	if (is_interleaved_out) {
		// AU wants a single interleaved stereo buffer
		AudioBufferList abl;
		abl.mNumberBuffers              = 1;
		abl.mBuffers[0].mNumberChannels = 2;
		abl.mBuffers[0].mDataByteSize   = process_block_size * 2 * sizeof(float);
		abl.mBuffers[0].mData           = interleaved_buf;

		AudioUnitRender((AudioUnit)unit, &flags, &ts, 0,
				(UInt32)process_block_size, &abl);

		for (int i = 0; i < process_block_size; i++) {
			p_out[i].l = interleaved_buf[i * 2];
			p_out[i].r = interleaved_buf[i * 2 + 1];
		}
	} else {
		// Non-interleaved: one buffer per channel
		AudioBufferList abl;
		abl.mNumberBuffers              = 2;
		abl.mBuffers[0].mNumberChannels = 1;
		abl.mBuffers[0].mDataByteSize   = process_block_size * sizeof(float);
		abl.mBuffers[0].mData           = out_buf_l;
		abl.mBuffers[1].mNumberChannels = 1;
		abl.mBuffers[1].mDataByteSize   = process_block_size * sizeof(float);
		abl.mBuffers[1].mData           = out_buf_r;

		AudioUnitRender((AudioUnit)unit, &flags, &ts, 0,
				(UInt32)process_block_size, &abl);

		for (int i = 0; i < process_block_size; i++) {
			p_out[i].l = out_buf_l[i];
			p_out[i].r = out_buf_r[i];
		}
	}

	sample_time += process_block_size;
}

void AudioEffectAU::process_with_secondary(const Event *p_events, int p_event_count,
		const AudioFrame *p_in, const AudioFrame *p_secondary,
		AudioFrame *p_out, bool p_prev_active) {
	// AU doesn't support side-chain in our current implementation; treat as normal
	process(p_events, p_event_count, p_in, p_out, p_prev_active);
}

void AudioEffectAU::set_process_block_size(int p_size) {
	if (process_block_size == p_size && (in_buf_l != nullptr))
		return;
	process_block_size = p_size;
	if (unit) {
		// MaximumFramesPerSlice can only be changed while uninitialized.
		AudioUnitUninitialize((AudioUnit)unit);
		UInt32 maxFrames = (UInt32)p_size;
		AudioUnitSetProperty((AudioUnit)unit,
				kAudioUnitProperty_MaximumFramesPerSlice,
				kAudioUnitScope_Global, 0, &maxFrames, sizeof(maxFrames));
		AudioUnitInitialize((AudioUnit)unit);
	}
	_alloc_buffers();
}

void AudioEffectAU::set_sampling_rate(int p_hz) {
	if (sampling_rate == p_hz)
		return;
	sampling_rate = p_hz;
	if (unit) {
		// Must uninitialize, change rate, re-initialize for most AUs
		AudioUnitUninitialize((AudioUnit)unit);

		AudioStreamBasicDescription asbd;
		setup_asbd(asbd, sampling_rate);
		AudioUnitSetProperty((AudioUnit)unit, kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Output, 0, &asbd, sizeof(asbd));
		if (has_input_bus) {
			AudioUnitSetProperty((AudioUnit)unit, kAudioUnitProperty_StreamFormat,
					kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));
		}

		// Re-apply max frames per slice while uninitialized.
		UInt32 maxFrames = (UInt32)process_block_size;
		AudioUnitSetProperty((AudioUnit)unit, kAudioUnitProperty_MaximumFramesPerSlice,
				kAudioUnitScope_Global, 0, &maxFrames, sizeof(maxFrames));

		// Re-check actual output format after rate change
		AudioStreamBasicDescription actual;
		UInt32 actualSz = sizeof(actual);
		if (AudioUnitGetProperty((AudioUnit)unit, kAudioUnitProperty_StreamFormat,
					kAudioUnitScope_Output, 0, &actual, &actualSz) == noErr) {
			bool was_interleaved = is_interleaved_out;
			is_interleaved_out = !(actual.mFormatFlags & kAudioFormatFlagIsNonInterleaved);
			if (was_interleaved != is_interleaved_out)
				_alloc_buffers(); // layout changed, reallocate
		}

		AudioUnitInitialize((AudioUnit)unit);
	}
}

void AudioEffectAU::reset() {
	if (unit) {
		AudioUnitReset((AudioUnit)unit, kAudioUnitScope_Global, 0);
	}
	sample_time = 0;
	if (accepts_midi) {
		stop_all_notes = true;
		_reset_midi(); // re-send bank/patch/CC defaults/pitch-bend range on next process
	}
}

// ── GUI ───────────────────────────────────────────────────────────────────────

bool AudioEffectAU::has_user_interface() const {
	if (!unit) return false;
	UInt32 dataSize = 0;
	Boolean writable = false;
	OSStatus r = AudioUnitGetPropertyInfo((AudioUnit)unit,
			kAudioUnitProperty_CocoaUI,
			kAudioUnitScope_Global, 0, &dataSize, &writable);
	return (r == noErr && dataSize >= sizeof(AudioUnitCocoaViewInfo));
}

void *AudioEffectAU::get_user_interface_view() {
	if (!unit) return nullptr;

	UInt32 dataSize = 0;
	Boolean writable = false;
	if (AudioUnitGetPropertyInfo((AudioUnit)unit,
				kAudioUnitProperty_CocoaUI,
				kAudioUnitScope_Global, 0, &dataSize, &writable) != noErr) {
		return nullptr;
	}
	if (dataSize < sizeof(AudioUnitCocoaViewInfo)) return nullptr;

	AudioUnitCocoaViewInfo *info = (AudioUnitCocoaViewInfo *)malloc(dataSize);
	if (AudioUnitGetProperty((AudioUnit)unit, kAudioUnitProperty_CocoaUI,
				kAudioUnitScope_Global, 0, info, &dataSize) != noErr) {
		free(info);
		return nullptr;
	}

	NSBundle *bundle = [NSBundle bundleWithURL:(__bridge NSURL *)info->mCocoaAUViewBundleLocation];
	NSString *className = (__bridge NSString *)info->mCocoaAUViewClass[0];
	free(info);

	if (!bundle || !className) return nullptr;

	Class factoryClass = [bundle classNamed:className];
	if (!factoryClass) return nullptr;

	id<AUCocoaUIBase> factory = [[factoryClass alloc] init];
	NSView *view = [factory uiViewForAudioUnit:(AudioUnit)unit withSize:NSZeroSize];
	return (void *)(__bridge_retained void *)view;
}

bool AudioEffectAU::get_user_interface_size(int &r_width, int &r_height) {
	NSView *view = (NSView *)get_user_interface_view();
	if (!view) return false;
	NSRect bounds = [view bounds];
	r_width  = (int)bounds.size.width;
	r_height = (int)bounds.size.height;
	CFRelease((__bridge CFTypeRef)view);
	return true;
}

// ── State ─────────────────────────────────────────────────────────────────────

JSON::Node AudioEffectAU::_internal_to_json() const {
	JSON::Node node = JSON::object();
	node.add("type", "au");

	if (!unit) return node;

	CFPropertyListRef classInfo = nullptr;
	UInt32 dataSize = sizeof(classInfo);
	if (AudioUnitGetProperty((AudioUnit)unit, kAudioUnitProperty_ClassInfo,
				kAudioUnitScope_Global, 0, &classInfo, &dataSize) == noErr && classInfo) {
		CFDataRef xmlData = CFPropertyListCreateData(kCFAllocatorDefault,
				classInfo, kCFPropertyListXMLFormat_v1_0, 0, nullptr);
		CFRelease(classInfo);

		if (xmlData) {
			Vector<uint8_t> v;
			v.resize((int)CFDataGetLength(xmlData));
			memcpy(&v[0], CFDataGetBytePtr(xmlData), v.size());
			CFRelease(xmlData);
			node.add("state", base64_encode(v));
		}
	}
	return node;
}

Error AudioEffectAU::_internal_from_json(const JSON::Node &node) {
	ERR_FAIL_COND_V(!node.has("type"), ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V(node.get("type").toString() != "au", ERR_FILE_CORRUPT);
	if (!node.has("state") || !unit) return OK;

	std::string state_str = node.get("state").toString();
	Vector<uint8_t> data  = base64_decode(state_str);
	if (!data.size()) return OK;

	CFDataRef xmlData = CFDataCreate(kCFAllocatorDefault, &data[0], data.size());
	if (!xmlData) return ERR_CANT_OPEN;

	CFPropertyListRef classInfo = CFPropertyListCreateWithData(
			kCFAllocatorDefault, xmlData, kCFPropertyListImmutable, nullptr, nullptr);
	CFRelease(xmlData);
	if (!classInfo) return ERR_FILE_CORRUPT;

	AudioUnitSetProperty((AudioUnit)unit, kAudioUnitProperty_ClassInfo,
			kAudioUnitScope_Global, 0, &classInfo, sizeof(classInfo));
	CFRelease(classInfo);
	return OK;
}

// ── open / close ──────────────────────────────────────────────────────────────

Error AudioEffectAU::open(void *p_component, const String &p_unique_id,
		const String &p_name, const String &p_provider_id, bool p_is_synth) {

	unique_id   = p_unique_id;
	name        = p_name;
	provider_id = p_provider_id;
	is_synth    = p_is_synth;
	component   = p_component;

	AudioUnit au;
	OSStatus st = AudioComponentInstanceNew((AudioComponent)p_component, &au);
	if (st != noErr) {
		printf("AU open: AudioComponentInstanceNew failed: %d\n", (int)st);
		return ERR_CANT_OPEN;
	}
	unit = (void *)au;
	printf("AU open: instance created\n");

	// Determine whether this AU consumes MIDI (instruments and music effects do).
	{
		AudioComponentDescription d;
		memset(&d, 0, sizeof(d));
		AudioComponentGetDescription((AudioComponent)p_component, &d);
		accepts_midi = (d.componentType == kAudioUnitType_MusicDevice ||
		                d.componentType == kAudioUnitType_MusicEffect);
	}

	// Check whether the AU has an input bus
	UInt32 inputBusCount = 0;
	UInt32 sz = sizeof(inputBusCount);
	AudioUnitGetProperty(au, kAudioUnitProperty_ElementCount,
			kAudioUnitScope_Input, 0, &inputBusCount, &sz);
	has_input_bus = (inputBusCount > 0);
	printf("AU open: has_input_bus=%d\n", (int)has_input_bus);

	// Set stream format; then query back to learn what the AU actually accepted.
	AudioStreamBasicDescription asbd;
	setup_asbd(asbd, sampling_rate);
	AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Output, 0, &asbd, sizeof(asbd));

	{
		AudioStreamBasicDescription actual;
		UInt32 actualSz = sizeof(actual);
		if (AudioUnitGetProperty(au, kAudioUnitProperty_StreamFormat,
					kAudioUnitScope_Output, 0, &actual, &actualSz) == noErr) {
			is_interleaved_out = !(actual.mFormatFlags & kAudioFormatFlagIsNonInterleaved);
		}
		printf("AU open: output format interleaved=%d channels=%u\n",
				(int)is_interleaved_out, (unsigned)actual.mChannelsPerFrame);
	}

	if (has_input_bus) {
		AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input, 0, &asbd, sizeof(asbd));

		// Register render callback to feed audio into the AU
		AURenderCallbackStruct cb;
		cb.inputProc    = au_input_callback;
		cb.inputProcRefCon = this;
		AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Input, 0, &cb, sizeof(cb));
	}

	UInt32 maxFrames = (UInt32)process_block_size;
	AudioUnitSetProperty(au, kAudioUnitProperty_MaximumFramesPerSlice,
			kAudioUnitScope_Global, 0, &maxFrames, sizeof(maxFrames));

	st = AudioUnitInitialize(au);
	if (st != noErr) {
		printf("AU open: AudioUnitInitialize failed: %d\n", (int)st);
		AudioComponentInstanceDispose(au);
		unit = nullptr;
		return ERR_CANT_OPEN;
	}
	printf("AU open: initialized ok\n");

	// Enumerate parameters
	UInt32 paramListSize = 0;
	AudioUnitGetPropertyInfo(au, kAudioUnitProperty_ParameterList,
			kAudioUnitScope_Global, 0, &paramListSize, nullptr);

	int paramCount = (int)(paramListSize / sizeof(AudioUnitParameterID));
	if (paramCount > 0) {
		Vector<AudioUnitParameterID> paramIDs;
		paramIDs.resize(paramCount);
		AudioUnitGetProperty(au, kAudioUnitProperty_ParameterList,
				kAudioUnitScope_Global, 0, &paramIDs[0], &paramListSize);

		control_ports.resize(paramCount);
		for (int i = 0; i < paramCount; i++) {
			AudioUnitParameterInfo info;
			memset(&info, 0, sizeof(info));
			UInt32 infoSize = sizeof(info);
			if (AudioUnitGetProperty(au, kAudioUnitProperty_ParameterInfo,
						kAudioUnitScope_Global, paramIDs[i], &info, &infoSize) != noErr) {
				continue;
			}

			ControlPortAU &cp = control_ports[i];
			cp.unit       = (void *)au;
			cp.param_id   = (unsigned int)paramIDs[i];
			cp.setting    = false;
			cp.min_val    = info.minValue;
			cp.max_val    = info.maxValue;
			cp.is_bool    = (info.unit == kAudioUnitParameterUnit_Boolean);
			cp.is_indexed = (info.unit == kAudioUnitParameterUnit_Indexed);

			if (info.flags & kAudioUnitParameterFlag_HasCFNameString) {
				NSString *ns = (__bridge NSString *)info.cfNameString;
				cp.name.parse_utf8([ns UTF8String]);
				CFRelease(info.cfNameString);
			} else {
				cp.name.parse_utf8(info.name);
			}

			cp.identifier = cp.name.to_lower();
			cp.identifier.replace(" ", "_");
			String idx;
			idx.parse_utf8(std::to_string(i).c_str());
			cp.identifier = "au_param_" + idx + "_" + cp.identifier;
		}
	}

	_alloc_buffers();
	_create_event_listener();
	return OK;
}

void AudioEffectAU::set_resize_callback(ResizeCallback p_callback, void *p_userdata) {
	resize_callback = p_callback;
	resize_userdata = p_userdata;
}

AudioEffectAU::AudioEffectAU() :
		component(nullptr),
		unit(nullptr),
		process_block_size(128),
		sampling_rate(44100),
		sample_time(0),
		in_buf_l(nullptr),
		in_buf_r(nullptr),
		out_buf_l(nullptr),
		out_buf_r(nullptr),
		interleaved_buf(nullptr),
		is_synth(false),
		has_input_bus(false),
		is_interleaved_out(false),
		accepts_midi(false),
		stop_all_notes(false),
		last_midi_channel(0),
		event_listener(nullptr),
		resize_callback(nullptr),
		resize_userdata(nullptr) {
}

AudioEffectAU::~AudioEffectAU() {
	_destroy_event_listener();
	_free_buffers();
	if (unit) {
		AudioUnitUninitialize((AudioUnit)unit);
		AudioComponentInstanceDispose((AudioUnit)unit);
	}
}

#endif // OSX_ENABLED
