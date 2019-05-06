#ifndef AUDIOEFFECTFACTORYVST_H
#define AUDIOEFFECTFACTORYVST_H

#include "engine/audio_effect.h"
#include "globals/map.h"
#include "vst/aeffectx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class AudioEffectVST2 : public AudioEffect {

	const AudioEffectInfo *info;
	AEffect *effect;
	HINSTANCE libhandle;
	static VstIntPtr VSTCALLBACK host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);

public:
	virtual bool process(const Frame *p_in, Frame *p_out, const Event *p_events, bool p_prev_active);

	virtual const AudioEffectInfo *get_info() const;

	virtual int get_control_port_count() const;
	virtual ControlPort *get_control_port(int p_port);

	virtual void reset();

	/* Load/Save */

	virtual Error save(TreeSaver *p_tree);
	virtual Error load(TreeLoader *p_tree);

	Error open(const String &p_path);
	AudioEffectVST2(const AudioEffectInfo *p_info);
	~AudioEffectVST2();
};

class AudioEffectProviderVST2 : public AudioEffectProvider {

	struct VST_Struct {

		String path;
		String dir;
		String uniqueID;
		bool write_only;
	};

	Map<String, VST_Struct> plugins;

	String paths;
	static VstIntPtr VSTCALLBACK host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
	static AudioEffect *creation_func(const AudioEffectInfo *); ///< creation function for instancing this node
	friend class AudioEffectVST2;

	static AEffect *open_vst_from_lib_handle(HINSTANCE libhandle, audioMasterCallback p_master_callback);

	static AudioEffectProviderVST2 *singleton;

public:
	void set_paths(const String &p_paths);
	String get_paths() const;

	virtual void scan_effects(AudioEffectFactory *p_factory);
	virtual String get_name() const;

	AudioEffectProviderVST2();
	~AudioEffectProviderVST2();
};

#endif // AUDIOEFFECTFACTORYVST_H
