#ifndef SONG_FILE_H
#define SONG_FILE_H

#include "engine/audio_effect.h"
#include "engine/song.h"

class SongFile {
	Song *song;
	AudioEffectFactory *fx_factory;

public:
	struct MissingPlugin {
		String provider;
		String id;
	};

	Error save(const String &p_path);
	Error load(const String &p_path, List<MissingPlugin> *r_missing_plugins = NULL);
	SongFile(Song *p_song, AudioEffectFactory *p_fx_factory);
};

#endif // SONG_FILE_H
