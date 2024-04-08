#ifndef LOADER_IT_H
#define LOADER_IT_H

#include "song.h"
#include <map>

enum {

	LOADER_LOAD_SUCCESS,
	LOADER_ERROR_LOADER_IN_USE,
	LOADER_ERROR_CANNOT_OPEN_FILE,
	LOADER_ERROR_FILE_ERROR,
	LOADER_ERROR_FILE_FORMAT_NOT_RECOGNIZED,
	LOADER_ERROR_HEADER_CORRUPT
};


/**
  *@author Juan Linietsky
  */

#define ITENVCNT 25
#define ITNOTECNT 120

class Loader_IT {

	FILE *f;

	void get_byte_array(uint8_t *p_where, uint32_t p_bytes);
	void get_byte(uint8_t *p_where);
	void get_word(uint16_t *p_where);
	void get_dword(uint32_t *p_where);
	void seek(uint32_t p_offset);

	bool error_loading();
	bool eof_reached();

	static constexpr const int FUNCTION_SUCCESS = 0;

	struct IT_Header {
		char    identifier[5]; // 4 bytes;
		char	songname[27]; // 26 bytes
		uint8_t	blank01[2];
		uint16_t	ordnum;
		uint16_t	insnum;
		uint16_t	smpnum;
		uint16_t	patnum;
		uint16_t	cwt;		/* Created with tracker (y.xx = 0x0yxx) */
		uint16_t	cmwt;		/* Compatible with tracker ver > than val. */
		uint16_t	flags;
		uint16_t	special;	/* bit 0 set = song message attached */
		uint8_t	globvol;
		uint8_t	mixvol;		/* mixing volume [ignored] */
		uint8_t	initspeed;
		uint8_t	inittempo;
		uint8_t	pansep;		/* panning separation between channels */
		uint8_t	zerobyte;
		uint16_t	msglength;
		uint32_t	msgoffset;
		uint8_t	blank02[4];
		uint8_t	pantable[64];
		uint8_t	voltable[64];
	
	};

	struct IT_Sample {
		char	header[5];
		char	filename[13];
		uint8_t	zerobyte;
		uint8_t	globvol;
		uint8_t	flag;
		uint8_t	volume;
		uint8_t	panning;
		char	sampname[29];
		uint8_t	convert;	/* sample conversion flag */
		uint32_t	length;
		uint32_t	loopbeg;
		uint32_t	loopend;
		uint32_t	c5spd;
		uint32_t	susbegin;
		uint32_t	susend;
		uint32_t	sampoffset;
		uint8_t	vibspeed;
		uint8_t	vibdepth;
		uint8_t	vibrate;
		uint8_t	vibwave;	/* 0=sine, 1=rampdown, 2=square, 3=random (speed ignored) */

		/* pointer to sampledata! */

		void *sample_data;
	};

	struct IT_Instrument {
		char	header[5];
		uint32_t	size;			/* (dword) Instrument size */
		char	filename[13];	/* (char) Instrument filename */
		uint8_t	zerobyte;		/* (byte) Instrument type (always 0) */
		uint8_t	volflg;
		uint8_t	volpts;
		uint8_t	volbeg;			/* (byte) Volume loop start (node) */
		uint8_t	volend;			/* (byte) Volume loop end (node) */
		uint8_t	volsusbeg;		/* (byte) Volume sustain begin (node) */
		uint8_t	volsusend;		/* (byte) Volume Sustain end (node) */
		uint8_t	panflg;
		uint8_t	panpts;
		uint8_t	panbeg;			/* (byte) channel loop start (node) */
		uint8_t	panend;			/* (byte) channel loop end (node) */
		uint8_t	pansusbeg;		/* (byte) channel sustain begin (node) */
		uint8_t	pansusend;		/* (byte) channel Sustain end (node) */
		uint8_t	pitflg;
		uint8_t	pitpts;
		uint8_t	pitbeg;			/* (byte) pitch loop start (node) */
		uint8_t	pitend;			/* (byte) pitch loop end (node) */
		uint8_t	pitsusbeg;		/* (byte) pitch sustain begin (node) */
		uint8_t	pitsusend;		/* (byte) pitch Sustain end (node) */
		uint16_t	blank;
		uint8_t	globvol;
		uint8_t	chanpan;
		uint16_t	fadeout;		/* Envelope end / NNA volume fadeout */
		uint8_t	dnc;			/* Duplicate note check */
		uint8_t	dca;			/* Duplicate check action */
		uint8_t	dct;			/* Duplicate check type */
		uint8_t	nna;			/* New Note Action [0,1,2,3] */
		uint8_t	IFC;
		uint8_t	IFR;
		uint8_t	midichan;
		uint8_t	midiprog;
		uint16_t	midibank;

		uint16_t	trkvers;		/* tracker version used to save [files only] */
		uint8_t	ppsep;			/* Pitch-pan Separation */
		uint8_t	ppcenter;		/* Pitch-pan Center */
		uint8_t	rvolvar;		/* random volume varations */
		uint8_t	rpanvar;		/* random panning varations */
		uint16_t	numsmp;			/* Number of samples in instrument [files only] */
		char	name[27];		/* Instrument name */
		uint8_t	blank01[4];
		uint16_t	samptable[ITNOTECNT];/* sample for each note [note / samp pairs] */
		uint8_t	volenv[200];	     /* volume envelope (IT 1.x stuff) */
		uint8_t	oldvoltick[ITENVCNT];/* volume tick position (IT 1.x stuff) */
		uint8_t	volnode[ITENVCNT];   /* amplitude of volume nodes */
		uint16_t	voltick[ITENVCNT];   /* tick value of volume nodes */
		uint8_t	pannode[ITENVCNT];   /* panenv - node points */
		uint16_t	pantick[ITENVCNT];   /* tick value of panning nodes */
		uint8_t	pitnode[ITENVCNT];   /* pitchenv - node points */
		uint16_t	pittick[ITENVCNT];   /* tick value of pitch nodes */

	};

	

	struct Note {

		enum {

			NOTES=120,
			OFF=254,
			CUT=253,
			EMPTY=255,
			SCRIPT=252,
		};


		uint8_t note;
		uint8_t instrument;
		uint8_t volume;
		uint8_t command;
		uint8_t parameter;

		void clear() {

			note=EMPTY;
			instrument=EMPTY;
			volume=EMPTY;
			command=EMPTY;
			parameter=0;
		}

		void raise() {

			if (note<(NOTES-1))
			    note++;
			else if (note==SCRIPT && parameter<0xFF)
			    parameter++;
		}

		void lower() {

			if ((note>0) && (note<NOTES))
			    note--;
			else if (note==SCRIPT && parameter>0)
			    parameter--;

		}

		bool operator== (const Note &rvalue) {

			return (
				 (note==rvalue.note) &&
				 (instrument==rvalue.instrument) &&
				 (volume==rvalue.volume) &&
				 (command==rvalue.command) &&
				 (parameter==rvalue.parameter)
				);
		}

		bool is_empty() const { return (note==EMPTY && instrument==EMPTY && volume==EMPTY && command==EMPTY && parameter==0); }
		Note() {

			clear();
		}
	};

	static const char * IT_Version[];


	struct IT_Pattern {
		std::vector< std::map<int,Note> > notes;
		int rows = 0;
		Note &get_note_ref(int channel, int row) {
			if (channel >= notes.size()) {
				notes.resize(channel+1);
			}
			if (notes[channel].find(row)==notes[channel].end()) {
				notes[channel][row]=Note();
			}
			return notes[channel][row];
		}

	};

	IT_Header header;
	IT_Sample **sample;
	IT_Instrument **instrument;
	IT_Pattern **pattern;
	int sample_count;
	int instrument_count;
	int pattern_count;

	enum {
		ORDER_EMPTY=255,
		ORDER_BREAK=254
	};

	int orders[256]={};

	/* IT sample decompression helper functions */

	uint32_t *source_buffer; 	/* source buffer */
	uint32_t *source_position; 		/* actual reading position */
	uint8_t source_remaining_bits;		/* bits remaining in read dword */

	uint32_t read_n_bits_from_IT_compressed_block(uint8_t p_bits_to_read);
	int read_IT_compressed_block ();
	void free_IT_compressed_block ();
	int load_sample_8bits_IT_compressed(void *p_dest_buffer,int p_buffsize);
	int load_sample_16bits_IT_compressed(void *p_dest_buffer,int p_buffsize);

	/* end of IT sample decompression functions */
	int load_samples();
	int load_sampledata();
	int load_instruments();
	int load_patterns();

	void clear_structs(bool p_clear_shared_data);

	//overrided virtual functions
	int load_header();
	int load_body();
	int transfer_data_to_song();
	int end_load();
	//giving up! error loading
	int give_up_and_clear_structs(int p_reason);

	Song *song;
public:

	int import_it(String p_filename);
	void link_to_song(Song* p_song) {song=p_song;};
	
	Loader_IT();
	~Loader_IT();
};

#endif
