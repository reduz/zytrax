#include "audio_effect_provider_vst2.h"
#include "vestige.h"
#include <dirent.h>
/////////////////////////////////////////////

AEffect *AudioEffectProviderVST2::open_vst_from_lib_handle(HINSTANCE libhandle, audioMasterCallback p_master_callback) {

	if (libhandle == NULL) {
		//printf("invalid file: %s\n",lib_name);
		return NULL;
	}

	AEffect *(__cdecl * getNewPlugInstance)(audioMasterCallback);

	getNewPlugInstance = (AEffect * (__cdecl *)(audioMasterCallback)) GetProcAddress(libhandle, "VSTPluginMain");
	if (!getNewPlugInstance) {
		getNewPlugInstance = (AEffect * (__cdecl *)(audioMasterCallback)) GetProcAddress(libhandle, "main");
	}

	if (getNewPlugInstance == NULL) {
		FreeLibrary(libhandle);
		//WARN_PRINT("Can't find symbol 'main'");
		return NULL;
	}
	return getNewPlugInstance(p_master_callback);
}

String AudioEffectProviderVST2::get_id() const {
	return "VST2";
}

AudioEffect *AudioEffectProviderVST2::instantiate_effect(const AudioEffectInfo *p_info) {

	AudioEffectVST2 *fx_vst2 = new AudioEffectVST2;
	if (fx_vst2->open(p_info->path, p_info->unique_ID, p_info->caption, p_info->provider_id) != OK) {
		return NULL;
	}

	return fx_vst2;
}

void AudioEffectProviderVST2::scan_effects(AudioEffectFactory *p_factory, ScanCallback p_callback, void *p_userdata) {

	for (int i = 0; i < MAX_SCAN_PATHS; i++) {

		String p = get_scan_path(i).strip_edges();
		if (p == String()) {
			continue;
		}
		printf("scanning path: %s\n", p.utf8().get_data());

		_WDIR *dir;
		struct _wdirent *dirent;

		printf("about to open dir\n");
		dir = _wopendir(p.c_str());
		if (dir == NULL) {
			printf("failed?\n");
			return;
		}

		printf("opened dir\n");

		while ((dirent = _wreaddir(dir))) {

			String lib_name = p + "/" + String(dirent->d_name);

			if (lib_name.get_extension() != "dll") {
				continue;
			}
			printf("opening plugin: %s\n", lib_name.utf8().get_data());

			HINSTANCE libhandle = LoadLibraryW(lib_name.c_str());

			AEffect *ptrPlug = open_vst_from_lib_handle(libhandle, &host);

			if (ptrPlug == NULL) {
				FreeLibrary(libhandle);
				continue;
			}

			if (ptrPlug->magic != kEffectMagic) {
				FreeLibrary(libhandle);
				continue;
			}

			ptrPlug->dispatcher(ptrPlug, effOpen, 0, 0, NULL, 0.0f);

			if (ptrPlug->numOutputs >= 2) { //needs to have at least 2 outputs
				AudioEffectInfo info;

				String name = dirent->d_name;
				name = name.substr(0, name.find("."));
				info.caption = name;
				printf("plugin name: %s\n", info.caption.utf8().get_data());
				info.description = "VST Info:\n Name: " + info.caption + "\n ID: " + String::num(ptrPlug->uniqueID) + "\n Version: " + String(ptrPlug->version);
				info.unique_ID = "VST_" + String::num(ptrPlug->uniqueID);
				info.synth = /*(ptrPlug->dispatcher(ptrPlug,effGetVstVersion,0,0,NULL,0.0f)==2  */ ptrPlug->flags & effFlagsIsSynth;
				info.category = info.synth ? "VST Instruments" : "VST Effects";
				info.has_ui = (ptrPlug->flags & effFlagsHasEditor);

				info.provider_caption = "VST2";
				info.version = String::num(ptrPlug->version);

				info.provider_id = get_id();
				info.path = lib_name;

				if (ptrPlug->flags & effFlagsProgramChunks) {

					info.description += " (CS)";
				}

				/* Perform the "write only" test */

				//plugin_data->write_only=true;	//i cant really be certain of anything with VST plugins, so this is always true
				/*
				if (ptrPlug->numParams) {

					ptrPlug->setParameter(ptrPlug,0,1.0); //set 1.0
					float res=ptrPlug->getParameter(ptrPlug,0);
					if (res<0.8) { //try if it's not near 1.0, with some threshold, then no reading (far most of the ones that dont support this will just return 0)


					}
			} */
				p_factory->add_audio_effect(info);
				if (p_callback) {
					p_callback(info.caption, p_userdata);
				}
			}
			ptrPlug->dispatcher(ptrPlug, effClose, 0, 0, NULL, 0.0f);
			FreeLibrary(libhandle);
		}
	}
}

intptr_t VESTIGECALLBACK AudioEffectProviderVST2::host(AEffect *effect, int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt) {
	long retval = 0;
	//simple host for exploring plugin
	switch (opcode) {
		//VST 1.0 opcodes
		case audioMasterVersion:
			//Input values:
			//none

			//Return Value:
			//0 or 1 for old version
			//2 or higher for VST2.0 host?
			retval = 2;
			break;
		case audioMasterGetSampleRate:

			effect->dispatcher(effect, effSetSampleRate, 0, 0, NULL, 44100); //just crap
			break;
		case audioMasterGetBlockSize:
			//Input Values:
			//None

			//Return Value:
			//not tested, always return 0

			//NB - Host must despatch effSetBlockSize to the plug in response
			//to this call
			//Check despatcher notes for any return codes from effSetBlockSize
			effect->dispatcher(effect, effSetBlockSize, 0, 1024, NULL, 0.0f);

			break;
		case audioMasterCanDo:
			//Input Values:
			//<ptr> predefined "canDo" string

			//Return Value:
			//0 = Not Supported
			//non-zero value if host supports that feature

			//NB - Possible Can Do strings are:
			//"sendVstEvents",
			//"sendVstMidiEvent",
			//"sendVstTimeInfo",
			//"receiveVstEvents",
			//"receiveVstMidiEvent",
			//"receiveVstTimeInfo",
			//"reportConnectionChanges",
			//"acceptIOChanges",
			//"sizeWindow",
			//"asyncProcessing",
			//"offline",
			//"supplyIdle",
			//"supportShell"
			if (strcmp((char *)ptr, "supplyIdle") == 0 ||
					strcmp((char *)ptr, "sendVstTimeInfo") == 0 ||
					strcmp((char *)ptr, "sendVstEvents") == 0 ||
					strcmp((char *)ptr, "sendVstMidiEvent") == 0 ||
					strcmp((char *)ptr, "sizeWindow") == 0) {
				retval = 1;
			} else {
				retval = 0;
			}

			break;
		case audioMasterGetLanguage:
			//Input Values:
			//None

			//Return Value:
			//kVstLangEnglish
			//kVstLangGerman
			//kVstLangFrench
			//kVstLangItalian
			//kVstLangSpanish
			//kVstLangJapanese

			retval = kVstLangEnglish;
			break;
	}
	return retval;
}

String AudioEffectProviderVST2::get_name() const {
	return "VST2";
}

AudioEffectProviderVST2 *AudioEffectProviderVST2::singleton = NULL;

AudioEffectProviderVST2::AudioEffectProviderVST2() {
	//paths = "C:\\Program Files\\Synister64";
	//paths = "C:\\Program Files\\Synister64";
	//paths = "C:\\Program Files\\Common Files\\VST2\\SonicCat";
	singleton = this;
}

AudioEffectProviderVST2::~AudioEffectProviderVST2() {
}
