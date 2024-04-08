#include "loader_it.h"


const char * Loader_IT::IT_Version[]={
		
	"ImpulseTracker  .  ",
	"Compressed ImpulseTracker  .  ",
	"ImpulseTracker 2.14p3",
	"Compressed ImpulseTracker 2.14p3",
	"ImpulseTracker 2.14p4",
	"Compressed ImpulseTracker 2.14p4",
};



Loader_IT::Loader_IT(){


	// char arrays to cstrings
	header.identifier[4]=0;
	header.songname[26]=0;

}

Loader_IT::~Loader_IT(){
}


/*************************************************************
   L O A D    H E A D E R
**************************************************************/

void Loader_IT::get_byte_array(uint8_t *p_where, uint32_t p_bytes) {
	fread(p_where,1,p_bytes,f);
}

void Loader_IT::get_byte(uint8_t *p_where) {
	fread(p_where,1,1,f);
}

bool Loader_IT::error_loading() {
	return false; //lies.
}
void Loader_IT::get_word(uint16_t *p_where) {
	uint8_t byte_lo;
	get_byte(&byte_lo);
	uint8_t byte_hi;
	get_byte(&byte_hi);
	*p_where = byte_hi;
	(*p_where)<<=8;
	*p_where |= byte_lo;
}

void Loader_IT::get_dword(uint32_t *p_where) {
	uint16_t word_lo;
	get_word(&word_lo);
	uint16_t word_hi;
	get_word(&word_hi);
	*p_where = word_hi;
	(*p_where)<<=16;
	*p_where |= word_lo;
}

bool Loader_IT::eof_reached() {
	return feof(f);
}

void Loader_IT::seek(uint32_t p_offset) {
	fseek(f,p_offset,SEEK_SET);
}

int Loader_IT::load_header() {
	
	std::string aux_string;

	get_byte_array((uint8_t*)header.identifier,4);
	get_byte_array((uint8_t*)header.songname,26);
	get_byte_array((uint8_t*)header.blank01,2);
	get_word((uint16_t*)&header.ordnum);
	get_word((uint16_t*)&header.insnum);
	get_word((uint16_t*)&header.smpnum);
	get_word((uint16_t*)&header.patnum);
	get_word((uint16_t*)&header.cwt);		/* Created with tracker (y.xx = 0x0yxx) */
	get_word((uint16_t*)&header.cmwt);		/* Compatible with tracker ver > than val. */
	get_word((uint16_t*)&header.flags);
	get_word((uint16_t*)&header.special);
	get_byte((uint8_t*)&header.globvol);
	get_byte((uint8_t*)&header.mixvol);		/* mixing volume [ignored] */
	get_byte((uint8_t*)&header.initspeed);
	get_byte((uint8_t*)&header.inittempo);
	get_byte((uint8_t*)&header.pansep);		/* panning separation between channels */
	get_byte((uint8_t*)&header.zerobyte);
	get_word((uint16_t*)&header.msglength);
	get_dword((uint32_t*)&header.msgoffset);
	get_byte_array((uint8_t*)header.blank02,4);
	get_byte_array((uint8_t*)header.pantable,64);
	get_byte_array((uint8_t*)header.voltable,64);

	aux_string=(char*)header.identifier;

	if ( aux_string!="IMPM" ) return LOADER_ERROR_FILE_FORMAT_NOT_RECOGNIZED;
	if ( eof_reached() ) return LOADER_ERROR_HEADER_CORRUPT;
	if ( error_loading() ) return LOADER_ERROR_FILE_ERROR;

	sample_count=0;

	return FUNCTION_SUCCESS;
}

/*************************************************************
   L O A D    S A M P L E S
**************************************************************/

int Loader_IT::load_samples() {

	int tmp_result = FUNCTION_SUCCESS;
	uint32_t *sample_offset;
	std::string aux_string;
	int i;


	sample_offset=(uint32_t*)malloc(header.smpnum*4);
	sample=(IT_Sample**)malloc(sizeof(IT_Sample*)*header.smpnum);

	seek(0xC0+header.ordnum+header.insnum*4);

	for (i=0;i<header.smpnum;i++) {

		get_dword((uint32_t*)&sample_offset[i]);
		sample[i]=new IT_Sample;

		sample[i]->header[4]=0;
		sample[i]->filename[13]=0;
		sample[i]->sampname[29]=0;
	}



	for (i=0;i<header.smpnum;i++) {
	
		seek(sample_offset[i]);

		get_byte_array((uint8_t*)sample[i]->header,4);
		get_byte_array((uint8_t*)sample[i]->filename,12);
		get_byte((uint8_t*)&sample[i]->zerobyte);
		get_byte((uint8_t*)&sample[i]->globvol);
		get_byte((uint8_t*)&sample[i]->flag);
		get_byte((uint8_t*)&sample[i]->volume);
		get_byte_array((uint8_t*)sample[i]->sampname,26);
		get_byte((uint8_t*)&sample[i]->convert);	/* sample conversion flag */
		get_byte((uint8_t*)&sample[i]->panning);
		get_dword((uint32_t*)&sample[i]->length);
		get_dword((uint32_t*)&sample[i]->loopbeg);
		get_dword((uint32_t*)&sample[i]->loopend);
		get_dword((uint32_t*)&sample[i]->c5spd);
		get_dword((uint32_t*)&sample[i]->susbegin);
		get_dword((uint32_t*)&sample[i]->susend);
		get_dword((uint32_t*)&sample[i]->sampoffset);
		get_byte((uint8_t*)&sample[i]->vibspeed);
		get_byte((uint8_t*)&sample[i]->vibdepth);
		get_byte((uint8_t*)&sample[i]->vibrate);
		get_byte((uint8_t*)&sample[i]->vibwave);	/* 0=sine, 1=rampdown, 2=square, 3=random (speed ignored) */

		sample[i]->sample_data=NULL;

		sample_count++;
	
		std::cout << i << "- " << (char*) sample[i]->sampname << std::endl;

	}

	free(sample_offset);

	if (eof_reached() || error_loading()) {

		//[[todo]] Clean up the mess (samples loaded)

		tmp_result = LOADER_ERROR_FILE_ERROR;

	}

	return tmp_result;
}

/* The following sample decompression code is based on xmp's code.(http://xmp.helllabs.org) which is based in openCP's code. */

/*************************************************************
   L O A D    I N S T R U M E N T S
**************************************************************/

int Loader_IT::load_instruments() {

	int tmp_result = FUNCTION_SUCCESS;
	uint32_t *instrument_offset;
	std::string aux_string;
	int i;

	instrument_offset=(uint32_t*)malloc(header.insnum*4);
	instrument=(IT_Instrument**)malloc(sizeof(IT_Instrument*)*header.insnum);

	seek(0xC0+header.ordnum);

	for (i=0;i<header.insnum;i++) {

		get_dword((uint32_t*)&instrument_offset[i]);
		instrument[i]=new IT_Instrument;

		instrument[i]->header[4]=0;
		instrument[i]->filename[13]=0;
		instrument[i]->name[29]=0;
	}



	for (i=0;i<header.insnum;i++) {

		int j;
	
		seek(instrument_offset[i]);

		get_byte_array((uint8_t*)instrument[i]->header,4);	/* (char) Instrument filename */
		get_byte_array((uint8_t*)instrument[i]->filename,12);	/* (char) Instrument filename */
		get_byte((uint8_t*)&instrument[i]->zerobyte);		/* (byte) Instrument type (always 0) */
		get_byte((uint8_t*)&instrument[i]->nna);			/* New Note Action [0,1,2,3] */
		get_byte((uint8_t*)&instrument[i]->dct);			/* Duplicate check type */
		get_byte((uint8_t*)&instrument[i]->dca);			/* Duplicate check action */
		get_word((uint16_t*)&instrument[i]->fadeout);		/* Envelope end / NNA volume fadeout */
		get_byte((uint8_t*)&instrument[i]->ppsep);			/* Pitch-pan Separation */
		get_byte((uint8_t*)&instrument[i]->ppcenter);		/* Pitch-pan Center */
		get_byte((uint8_t*)&instrument[i]->globvol);
		get_byte((uint8_t*)&instrument[i]->chanpan);
		get_byte((uint8_t*)&instrument[i]->rvolvar);		/* random volume varations */
		get_byte((uint8_t*)&instrument[i]->rpanvar);		/* random panning varations */
		get_byte_array((uint8_t*)instrument[i]->blank01,4);
		get_byte_array((uint8_t*)instrument[i]->name,26);
		get_byte((uint8_t*)&instrument[i]->IFC);
		get_byte((uint8_t*)&instrument[i]->IFR);
		get_byte((uint8_t*)&instrument[i]->midichan);
		get_byte((uint8_t*)&instrument[i]->midiprog);
		get_word((uint16_t*)&instrument[i]->midibank);

		for (j=0;j<ITNOTECNT;j++) get_word((uint16_t*)&instrument[i]->samptable[j]); /* sample for each note [note / samp pairs] */

		get_byte((uint8_t*)&instrument[i]->volflg);
		get_byte((uint8_t*)&instrument[i]->volpts);
		get_byte((uint8_t*)&instrument[i]->volbeg);			/* (byte) Volume loop start (node) */
		get_byte((uint8_t*)&instrument[i]->volend);			/* (byte) Volume loop end (node) */
		get_byte((uint8_t*)&instrument[i]->volsusbeg);		/* (byte) Volume sustain begin (node) */
		get_byte((uint8_t*)&instrument[i]->volsusend);		/* (byte) Volume Sustain end (node) */

		for (j=0;j<ITENVCNT;j++) {

			get_byte((uint8_t*)&instrument[i]->volnode[j]);
			get_word((uint16_t*)&instrument[i]->voltick[j]);
		}


		get_byte((uint8_t*)&instrument[i]->panflg);
		get_byte((uint8_t*)&instrument[i]->panpts);
		get_byte((uint8_t*)&instrument[i]->panbeg);			/* (byte) channel loop start (node) */
		get_byte((uint8_t*)&instrument[i]->panend);			/* (byte) channel loop end (node) */
		get_byte((uint8_t*)&instrument[i]->pansusbeg);		/* (byte) channel sustain begin (node) */
		get_byte((uint8_t*)&instrument[i]->pansusend);		/* (byte) channel Sustain end (node) */

		for (j=0;j<ITENVCNT;j++) {

			get_byte((uint8_t*)&instrument[i]->pannode[j]);
			get_word((uint16_t*)&instrument[i]->pantick[j]);
		}


		get_byte((uint8_t*)&instrument[i]->pitflg);
		get_byte((uint8_t*)&instrument[i]->pitpts);
		get_byte((uint8_t*)&instrument[i]->pitbeg);			/* (byte) pitch loop start (node) */
		get_byte((uint8_t*)&instrument[i]->pitend);			/* (byte) pitch loop end (node) */
		get_byte((uint8_t*)&instrument[i]->pitsusbeg);		/* (byte) pitch sustain begin (node) */
		get_byte((uint8_t*)&instrument[i]->pitsusend);		/* (byte) pitch Sustain end (node) */

		for (j=0;j<ITENVCNT;j++) {

			get_byte((uint8_t*)&instrument[i]->pitnode[j]);
			get_word((uint16_t*)&instrument[i]->pittick[j]);
		}


//		uint8_t	volenv[200];	     /* volume envelope (IT 1.x stuff) */
//		uint8_t	oldvoltick[ITENVCNT];/* volume tick position (IT 1.x stuff) */

		instrument_count++;
		std::cout << i << "- " << (char*) instrument[i]->name << std::endl;

	}

	free(instrument_offset);

	if (eof_reached() || error_loading()) {

		//[[todo]] Clean up the mess (samples loaded)
		tmp_result = LOADER_ERROR_FILE_ERROR;
	}

	return tmp_result;
}

/*************************************************************
   L O A D    P A T T E R N D A T A
**************************************************************/

int Loader_IT::load_patterns() {

	int tmp_result = FUNCTION_SUCCESS;

	uint32_t *pattern_offset;
	int i;

	pattern_offset=(uint32_t*)malloc(header.patnum*4);
	pattern=(IT_Pattern**)malloc(sizeof(IT_Pattern*)*header.patnum);

	seek(0xC0+header.ordnum+header.insnum*4+header.smpnum*4);

	for (i=0;i<header.patnum;i++) {

		get_dword((uint32_t*)&pattern_offset[i]);
		pattern[i]=new IT_Pattern;
	}



	for (i=0;i<header.patnum;i++) {

		uint16_t pat_size;
		uint16_t pat_length;
	
		if (pattern_offset[i]==0) {

			pattern[i]->rows = 64;

			std::cout << " found an empty pattern at pos " << i << std::endl;

		} else {
			
			int row=0,flag,channel,j;
			uint8_t aux_byte;
			uint32_t reserved;
			uint8_t chan_mask[64];
			Note last_value[64];
			
			for (j=0;j<64;j++) {
	
				chan_mask[j]=0;
				last_value[j].clear();
			}

			seek(pattern_offset[i]);

			get_word((uint16_t*)&pat_size);
			get_word((uint16_t*)&pat_length);
			get_dword((uint32_t*)&reserved);
			//[[TODO check for corrupt data
			std::cout << " found a " << pat_length << " rows pattern at pos " << i << std::endl;

			pattern[i]->rows = pat_length;

			do {

				get_byte((uint8_t*)&aux_byte);
				flag=aux_byte;

				if ( flag==0 ) {

					row++;
				} else {

					channel=(flag-1) & 63;
			
					if ( flag & 128 ) {

						get_byte((uint8_t*)&aux_byte);
						chan_mask[channel]=aux_byte;
					}
						
				
					if ( chan_mask[channel]&1 ) { // read note
				
						get_byte((uint8_t*)&aux_byte);
						
						if ( aux_byte<120 ) pattern[i]->get_note_ref(channel,row).note=aux_byte+1;
						else if ( aux_byte==255 ) pattern[i]->get_note_ref(channel,row).note=129;
						else if ( aux_byte==254 ) pattern[i]->get_note_ref(channel,row).note=129;
						else pattern[i]->get_note_ref(channel,row).note=0;

						last_value[channel].note=pattern[i]->get_note_ref(channel,row).note;
					}
						
	
					if ( chan_mask[channel]&2 ) {
	
						get_byte((uint8_t*)&aux_byte);
						if ( aux_byte<100 ) pattern[i]->get_note_ref(channel,row).command=aux_byte;
						else pattern[i]->get_note_ref(channel,row).command=0;

						last_value[channel].command=pattern[i]->get_note_ref(channel,row).command;
					}
					if ( chan_mask[channel]&4 ) {
	
						get_byte((uint8_t*)&aux_byte);
						if ( aux_byte<65 ) pattern[i]->get_note_ref(channel,row).volume=aux_byte;
						else pattern[i]->get_note_ref(channel,row).volume=65;

						last_value[channel].volume=pattern[i]->get_note_ref(channel,row).volume;
					}
					if ( chan_mask[channel]&8 ) {
	
						get_byte((uint8_t*)&aux_byte);
						if ( aux_byte<213 ) pattern[i]->get_note_ref(channel,row).command=aux_byte;
						else pattern[i]->get_note_ref(channel,row).command=0;
						last_value[channel].command=pattern[i]->get_note_ref(channel,row).command;

						get_byte((uint8_t*)&aux_byte);
						pattern[i]->get_note_ref(channel,row).parameter=aux_byte;
						last_value[channel].parameter=pattern[i]->get_note_ref(channel,row).parameter;
					}

					if ( chan_mask[channel]&16 ) {
	
						pattern[i]->get_note_ref(channel,row).note=last_value[channel].note;
					}

					if ( chan_mask[channel]&32 ) {
	
						pattern[i]->get_note_ref(channel,row).command=last_value[channel].command;
					}
					if ( chan_mask[channel]&64 ) {
	
						pattern[i]->get_note_ref(channel,row).volume=last_value[channel].volume;
					}
					if ( chan_mask[channel]&128 ) {
	
						pattern[i]->get_note_ref(channel,row).command=last_value[channel].command;
						pattern[i]->get_note_ref(channel,row).parameter=last_value[channel].parameter;
					}

				}
			} while(row<pat_length);

		}

		pattern_count++;
	}

	free(pattern_offset);

	if (eof_reached() || error_loading()) {

		//[[todo]] Clean up the mess (samples loaded)

		tmp_result = LOADER_ERROR_FILE_ERROR;

	}

	seek(0xC0);


	for (int i=0;i<header.ordnum;i++) {

		uint8_t aux_order;
		get_byte(&aux_order);

		if (aux_order==254)  {

			orders[i]=ORDER_BREAK;

		} else if (aux_order<200) {

			orders[i]=aux_order;
		}  else {
			orders[i]=ORDER_EMPTY;
		}

	}
	if (eof_reached() || error_loading()) {

		//[[todo]] Clean up the mess (samples loaded)

		tmp_result = LOADER_ERROR_FILE_ERROR;

	}

	return tmp_result;
}



/*************************************************************
   C L E A N   U P
**************************************************************/


void Loader_IT::clear_structs(bool p_clear_shared_data) {
	
	int i;

	for (i=0;i<sample_count;i++) {
	
		if ( p_clear_shared_data && (sample[i]->sample_data!=NULL) ) free(sample[i]->sample_data);
		delete sample[i];
	}

	for (i=0;i<instrument_count;i++) {
	
		delete instrument[i];
	}

	for (i=0;i<pattern_count;i++) {
	
		delete pattern[i];
	}

}


int Loader_IT::give_up_and_clear_structs(int p_reason) {

	clear_structs(true);
	fclose(f);

	return p_reason;
}

/*************************************************************
   O V E R R I D E N    F U N C T I O N S
**************************************************************/


int Loader_IT::transfer_data_to_song() {

	int i,j,k,l;
	bool found_note;
	int current_track=-1;
	int current_column,old_column;
	// Header
	song->clear();
       	// static variables
	
	song->set_name(header.songname);

	song->set_bpm(header.inittempo);
		
// [[todo]] read message		
//		uint16_t	special;	/* bit 0 set = song message attached */
//		uint16_t	msglength;
//		uint32_t	msgoffset;


	// Samples
/*
	for (i=0;i<sample_count;i++) {

		song->get_sample(i)->name=sample[i]->sampname;
		song->get_sample(i)->def_volume=sample[i]->volume;
		song->get_sample(i)->glb_volume=sample[i]->globvol;
		song->get_sample(i)->def_panning_on=sample[i]->panning & 128;
		song->get_sample(i)->def_panning=sample[i]->panning & 127;
		song->get_sample(i)->vibrato_type=sample[i]->vibwave;
		song->get_sample(i)->vibrato_speed=sample[i]->vibspeed;
		song->get_sample(i)->vibrato_depth=sample[i]->vibdepth;
		song->get_sample(i)->vibrato_rate=sample[i]->vibrate;
		song->get_sample(i)->base_speed=sample[i]->c5spd;
		song->get_sample(i)->size=sample[i]->length;
		song->get_sample(i)->loop_flags=sample[i]->flag >> 4;
		song->get_sample(i)->loop_begin=sample[i]->loopbeg;
		song->get_sample(i)->loop_end=sample[i]->loopend;
		song->get_sample(i)->sustain_loop_begin=sample[i]->susbegin;
		song->get_sample(i)->sustain_loop_end=sample[i]->susend;
		song->get_sample(i)->sample_ptr=sample[i]->sample_data;
		song->get_sample(i)->format=(sample[i]->flag >> 1) & 1;
		song->get_sample(i)->in_use=sample[i]->flag & IT_SAMPLE_EXISTS;

	}

	for (i=0;i<instrument_count;i++) {

		int j;

		song->get_instrument(i)->name=instrument[i]->name;

		song->get_instrument(i)->filename=instrument[i]->filename;
		song->get_instrument(i)->NNA_type=instrument[i]->nna;
		song->get_instrument(i)->duplicate_check_type=instrument[i]->dct;
		song->get_instrument(i)->duplicate_check_action=instrument[i]->dca;

		for (j=0;j<NOTE_NOTES;j++) {

			song->get_instrument(i)->note_number[j]=instrument[i]->samptable[j*2];
			song->get_instrument(i)->sample_number[j]=instrument[i]->samptable[j*2+1]-1;
		}

		// volume flags
		
		song->get_instrument(i)->volume.global_amount=instrument[i]->globvol;
		song->get_instrument(i)->volume.fadeout=instrument[i]->fadeout;
		song->get_instrument(i)->volume.random_variation=instrument[i]->rvolvar;

		// volume envelope flags

		song->get_instrument(i)->volume.envelope.on=instrument[i]->volflg & 0;
		song->get_instrument(i)->volume.envelope.loop_on=instrument[i]->volflg & 2;
		song->get_instrument(i)->volume.envelope.sustain_loop_on=instrument[i]->volflg & 4;
		song->get_instrument(i)->volume.envelope.loop_begin_node=instrument[i]->volbeg;
		song->get_instrument(i)->volume.envelope.loop_end_node=instrument[i]->volend;
		song->get_instrument(i)->volume.envelope.sustain_loop_begin_node=instrument[i]->volsusbeg;
		song->get_instrument(i)->volume.envelope.sustain_loop_end_node=instrument[i]->volsusend;

		for (j=0;j<instrument[i]->volpts;j++) {
		
			song->get_instrument(i)->volume.envelope.node[j].tick_offset=instrument[i]->voltick[j];
			song->get_instrument(i)->volume.envelope.node[j].value=instrument[i]->volnode[j];
		}

		// panning flags
		
		song->get_instrument(i)->panning.default_amount=instrument[i]->chanpan<65?instrument[i]->chanpan:32;
		song->get_instrument(i)->panning.use_default=instrument[i]->chanpan<65;

		song->get_instrument(i)->panning.random_variation=instrument[i]->rpanvar;
		song->get_instrument(i)->panning.pitch_separation=(Sint8)instrument[i]->ppsep;
		song->get_instrument(i)->panning.pitch_center=instrument[i]->ppcenter;

		// panning envelope flags

		song->get_instrument(i)->panning.envelope.on=instrument[i]->panflg & 0;
		song->get_instrument(i)->panning.envelope.loop_on=instrument[i]->panflg & 2;
		song->get_instrument(i)->panning.envelope.sustain_loop_on=instrument[i]->panflg & 4;
		song->get_instrument(i)->panning.envelope.loop_begin_node=instrument[i]->panbeg;
		song->get_instrument(i)->panning.envelope.loop_end_node=instrument[i]->panend;
		song->get_instrument(i)->panning.envelope.sustain_loop_begin_node=instrument[i]->pansusbeg;
		song->get_instrument(i)->panning.envelope.sustain_loop_end_node=instrument[i]->pansusend;

		for (j=0;j<instrument[i]->panpts;j++) {
		
			song->get_instrument(i)->panning.envelope.node[j].tick_offset=instrument[i]->pantick[j];
			song->get_instrument(i)->panning.envelope.node[j].value=instrument[i]->pannode[j];
		}

		// pitch envelope flags

		song->get_instrument(i)->pitch.envelope.on=instrument[i]->pitflg & 0;
		song->get_instrument(i)->pitch.envelope.loop_on=instrument[i]->pitflg & 2;
		song->get_instrument(i)->pitch.envelope.sustain_loop_on=instrument[i]->pitflg & 4;
		song->get_instrument(i)->pitch.envelope.loop_begin_node=instrument[i]->pitbeg;
		song->get_instrument(i)->pitch.envelope.loop_end_node=instrument[i]->pitend;
		song->get_instrument(i)->pitch.envelope.sustain_loop_begin_node=instrument[i]->pitsusbeg;
		song->get_instrument(i)->pitch.envelope.sustain_loop_end_node=instrument[i]->pitsusend;

		for (j=0;j<instrument[i]->pitpts;j++) {
		
			song->get_instrument(i)->pitch.envelope.node[j].tick_offset=instrument[i]->pittick[j];
			song->get_instrument(i)->pitch.envelope.node[j].value=instrument[i]->pitnode[j];
		}

// [[TODO]] midi stuff
//		song->get_instrument(i)-> =instrument[i]-> ;
		
	
	} */

	//patterndata



	std::cout << " set variables " << std::endl;

	int max_w = 1;
	for (j=0;j<pattern_count;j++) {
		song->pattern_get_beats(pattern[j]->rows / 4);
		if (pattern[j]->notes.size() > max_w) {
			max_w = pattern[j]->notes.size();
		}
	}

	for (j=0;j<max_w;j++) {
		char track_name[11]={'C','h','a','n','n','e','l',' ','0','0',0};
		Track * t = new Track;
		track_name[8]='0'+j/10;	
		track_name[9]='0'+j%10;	
		t->set_name(track_name);
		song->add_track(t);

	}

	for (i=0;i<pattern_count;i++) {

		for (k=0;k<pattern[i]->notes.size();k++) {
			int prev_cmd = -1;
			for (j=0;j<pattern[i]->rows;j++) {
				Note n = pattern[i]->get_note_ref(k,j);
				if (n.note == Note::EMPTY) {
					continue;
				}
				Track::Note tn;
				if (n.note < Note::NOTES) {
					tn.note = n.note;
					if (tn.note > 0) {
						tn.note--;
					}
				} else if (n.note == Note::CUT || n.note == Note::OFF) {
					tn.note = Track::Note::OFF;
				}

				if (n.volume <= 64) {
					tn.volume = int(n.volume) * 99 / 64;
					// volume column effects not supported
				}

				Tick ofs = j * TICKS_PER_BEAT / 4;

				if (tn != Track::Note()) {
					song->get_track(k)->set_note(i,ofs,tn);
				}

				int cmd = -1;
				if (n.command != Note::EMPTY) {
					cmd = n.command;
				}

				if (cmd != prev_cmd) {
					/*
					if (song->get_track(k)->get_command_column_count()==0) {
						song->get_track(k)->set_command_columns(1);
					}
					Track::Command c;
					c.command
					song->get_track(k)->set_command();*/
				}
			}
		}
	}

	for (int i=0;i<header.ordnum;i++) {
		switch(orders[i]) {
			case ORDER_EMPTY: {
			} break;
			case ORDER_BREAK: {
				song->order_set(i,Song::ORDER_SKIP);
			} break;
			default: {
				song->order_set(i,orders[i]);
			}
		}
	}





/* loading mode.. supposed to be smart.. but it's not!

	std::cout << " copy stuff " << std::endl;

	for (i=1;i<99;i++) { // all instruments

		found_note=false;

		for (k=0;k<64;k++) { // all columns
	
			for (j=0;j<pattern_count;j++) {

				for (l=0;l<pattern[j]->length();l++) {

					if (pattern[j]->column[k]->get_note(l).command==i) {

						if (!found_note) {

                                                 	song->add_track(1);
							current_column=0;
							current_track++;
							found_note=true;
							old_column=k;
						}

						if (old_column!=k) {

							song->get_track(current_track)->add_column();
							current_column++;
							old_column=k;
						}

						if (j<MAX_PATTERNS_PER_TRACK) {

							song->get_track(current_track)->pattern[j]->column[current_column]->get_ref_note(l)=pattern[j]->column[k]->get_note(l);
							song->get_track(current_track)->pattern[j]->column[current_column]->get_ref_note(l).command=0;
						}

					}		          		
				}
			}
		}
	}


*/

	// Instruments

	return FUNCTION_SUCCESS;
}

int Loader_IT::import_it(String p_filename) {

	if (song==NULL) return -1;

        int aux_result;

#ifdef WINDOWS_ENABLED
	f = _wfopen(p_filename.c_str(), L"rb");
#else
	f = fopen(p_filename.utf8().get_data(), "rb");
#endif

	if (!f) return LOADER_ERROR_CANNOT_OPEN_FILE;

       	sample_count=0;
	instrument_count=0;
	pattern_count=0;

	if ( (aux_result=load_header()) ) return give_up_and_clear_structs(aux_result);
	if ( (aux_result=load_samples()) ) return give_up_and_clear_structs(aux_result);
	if ( (aux_result=load_instruments()) ) return give_up_and_clear_structs(aux_result);
	if ( (aux_result=load_patterns()) ) return give_up_and_clear_structs(aux_result);
	
	transfer_data_to_song();

	clear_structs(true);

	fclose(f);

	return FUNCTION_SUCCESS;
}


