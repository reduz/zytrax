#ifndef EFFECTEDITORLV2_H
#define EFFECTEDITORLV2_H

#include "drivers/lv2/audio_effect_lv2.h"
#include "gui/effect_editor_midi.h"
#include "suil/suil.h"

class EffectEditorLV2 : public Gtk::VBox {

	AudioEffectLV2 *lv2_effect;
	Gtk::Widget *widget = nullptr;
	EffectEditorMIDI effect_editor_midi;

	sigc::connection init_timer;
	bool initialize();

	static SuilHost *ui_host;
	SuilInstance *ui_instance;
	static void _port_write_func(void *ud, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer);
	static uint32_t _port_index_func(void *ud, const char *port_symbol);

public:
	static void initialize_lv2_editor();
	static void finalize_lv2_editor();

	EffectEditorLV2(AudioEffectLV2 *p_vst, EffectEditor *p_editor, const LilvUI *p_ui, const LilvNode *p_ui_type, const LilvNode *p_native_ui_type);
	~EffectEditorLV2();
};

EffectEditorPluginFunc get_lv2_editor_function();

#endif // EFFECTEDITORLV2_H
