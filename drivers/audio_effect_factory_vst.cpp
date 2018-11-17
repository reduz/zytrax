#include "audio_effect_factory_vst.h"
#include <dirent.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void AudioEffectProviderVST2::set_paths(const String& p_paths) {
	paths=p_paths;
}

String AudioEffectProviderVST2::get_paths() const {
	return paths;
}

AudioEffect *AudioEffectProviderVST2::creation_func(const AudioEffectInfo *) {

	return NULL;
}

void AudioEffectProviderVST2::scan_effects(AudioEffectFactory *p_factory)  {

	for(int i=0;i<paths.get_slice_count(",");i++) {

		String p = paths.get_slice(",",i).strip_edges();
		printf("scanning path: %s\n",p.utf8().get_data());
		if (p=="") {
			continue;
		}

		DIR *dir;
		struct dirent *dirent;

		printf("about to open dir\n");
		dir= opendir(p.utf8().get_data());
		if (dir == NULL) {
			printf("failed?\n");
			return;
		}

		printf("opened dir\n");

		while ((dirent= readdir(dir))) {


			char lib_name[PATH_MAX];
			snprintf(lib_name, PATH_MAX, "%s\\%s", p.utf8().get_data(), dirent->d_name);

			printf("trying lib: %s\n",lib_name);

			//printf("Scanning %s\n",lib_name);

			HINSTANCE libhandle=LoadLibrary(lib_name);

			if (libhandle==NULL) {
				//printf("invalid file: %s\n",lib_name);
				continue;
			}

			AEffect* (__cdecl* getNewPlugInstance)(audioMasterCallback);

			getNewPlugInstance=(AEffect*(__cdecl*)(audioMasterCallback))GetProcAddress(libhandle, "VSTPluginMain");
			if (!getNewPlugInstance) {
				getNewPlugInstance=(AEffect*(__cdecl*)(audioMasterCallback))GetProcAddress(libhandle, "main");
			}

			if (getNewPlugInstance==NULL) {
				FreeLibrary(libhandle);
				WARN_PRINT("Can't find symbol 'main'");
				continue;
			}
			AEffect* ptrPlug=getNewPlugInstance(&host);

			if (ptrPlug==NULL) {
				WARN_PRINT("Can't instance plugin.");

				FreeLibrary(libhandle);
				continue;
			}

			if (ptrPlug->magic!=kEffectMagic) {
				WARN_PRINT("Can't instance plugin, corrupted");
				FreeLibrary(libhandle);
				continue;

			}
			ptrPlug->dispatcher(ptrPlug,effOpen,0,0,NULL,0.0f);

			VST_Struct plugin_data;
			plugin_data.path=lib_name;
			plugin_data.dir=p;
			AudioEffectInfo info;

			String name=dirent->d_name;
			name=name.substr(0,name.find("."));
			info.caption=name;
			printf("plugin name: %s\n",info.caption.utf8().get_data());
			info.description="VST Plugin";
			info.long_description="VST Info:\n Name: "+info.caption +"\n ID: "+ String::num(ptrPlug->uniqueID) + "\n Version: " + String(ptrPlug->version);
			info.unique_ID="VST_"+String::num(ptrPlug->uniqueID);
			info.synth=/*(ptrPlug->dispatcher(ptrPlug,effGetVstVersion,0,0,NULL,0.0f)==2  */ptrPlug->flags & effFlagsIsSynth;
			info.category=info.synth?"VST Instruments":"VST Effects";
			info.has_ui=(ptrPlug->flags & effFlagsHasEditor);

			info.provider=this;
			info.creation_func=creation_func;
			info.version=ptrPlug->version;

			if (ptrPlug->flags & effFlagsProgramChunks) {

				info.description+=" (CS)";
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
			plugins[info.unique_ID]=plugin_data;
			p_factory->add_audio_effect(info);

			ptrPlug->dispatcher(ptrPlug,effClose,0,0,NULL,0.0f);
			FreeLibrary(libhandle);

		}

	}
}


VstIntPtr VSTCALLBACK AudioEffectProviderVST2::host(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt) {
	long retval=0;

	switch (opcode)
	{
		//VST 1.0 opcodes
		case audioMasterVersion:
			//Input values:
			//none

			//Return Value:
			//0 or 1 for old version
			//2 or higher for VST2.0 host?
			retval=2;
			break;
		case audioMasterGetSampleRate:

			effect->dispatcher(effect,effSetSampleRate,0,0,NULL,44100); //just crap
			break;
		case audioMasterGetBlockSize:
			//Input Values:
			//None

			//Return Value:
			//not tested, always return 0

			//NB - Host must despatch effSetBlockSize to the plug in response
			//to this call
			//Check despatcher notes for any return codes from effSetBlockSize
			effect->dispatcher(effect,effSetBlockSize,0,1024,NULL,0.0f);

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

			if (strcmp((char*)ptr,"sendVstEvents")==0 ||
						 strcmp((char*)ptr,"sendVstMidiEvent")==0 ||
						 strcmp((char*)ptr,"supplyIdle")==0)
			{
				retval=1;
			}
			else
			{
				retval=0;
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

			retval=kVstLangEnglish;
			break;


	}
	return retval; //stupid plugin, i'm just reading stuff, dont annoy me with questions!
}

String AudioEffectProviderVST2::get_name() const {
	return "VST2";
}

AudioEffectProviderVST2::AudioEffectProviderVST2() {
	paths="C:\\Program Files\\Synister64";
}

AudioEffectProviderVST2::~AudioEffectProviderVST2() {
	printf("erased provider\n");
}
