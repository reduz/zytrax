#include "effect_editor_lv2.h"
#include "audio_effect_provider_lv2.h"

#define UI_TYPE "http://lv2plug.in/ns/extensions/ui#Gtk3UI"

static LV2_URI_Map_Feature uri_map = { NULL, &AudioEffectLV2::uri_to_id };

static LV2_Extension_Data_Feature ext_data = { NULL };

static LV2_Feature uri_map_feature = { NS_EXT "uri-map", NULL };
static LV2_Feature map_feature = { LV2_URID__map, NULL };
static LV2_Feature unmap_feature = { LV2_URID__unmap, NULL };
static LV2_Feature make_path_feature = { LV2_STATE__makePath, NULL };
static LV2_Feature sched_feature = { LV2_WORKER__schedule, NULL };
static LV2_Feature state_sched_feature = { LV2_WORKER__schedule, NULL };
static LV2_Feature safe_restore_feature = { LV2_STATE__threadSafeRestore, NULL };
static LV2_Feature log_feature = { LV2_LOG__log, NULL };
static LV2_Feature options_feature = { LV2_OPTIONS__options, NULL };
static LV2_Feature def_state_feature = { LV2_STATE__loadDefaultState, NULL };

bool EffectEditorLV2::initialize() {
	return false;
}

void EffectEditorLV2::_port_write_func(void *ud, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer) {
}
uint32_t EffectEditorLV2::_port_index_func(void *ud, const char *port_symbol) {
	return 0;
}

SuilHost *EffectEditorLV2::ui_host = nullptr;

EffectEditorLV2::EffectEditorLV2(AudioEffectLV2 *p_lv2, EffectEditor *p_editor, const LilvUI *p_ui, const LilvNode *p_ui_type, const LilvNode *p_native_ui_type) :
		effect_editor_midi(p_lv2, p_editor) {

	lv2_effect = p_lv2;

	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);

	const LV2_Feature parent_feature = {
		LV2_UI__parent, nullptr //?
	};
	const LV2_Feature instance_feature = {
		NS_EXT "instance-access", lilv_instance_get_handle(p_lv2->get_instance())
	};
	const LV2_Feature data_feature = {
		LV2_DATA_ACCESS_URI, &ext_data
	};
	const LV2_Feature idle_feature = {
		LV2_UI__idleInterface, NULL
	};
	const LV2_Feature *ui_features[] = {
		&uri_map_feature, &map_feature, &unmap_feature,
		&instance_feature,
		&data_feature,
		&log_feature,
		&parent_feature,
		&options_feature,
		&idle_feature,
		NULL
	};

	const char *bundle_uri = lilv_node_as_uri(lilv_ui_get_bundle_uri(p_ui));
	const char *binary_uri = lilv_node_as_uri(lilv_ui_get_binary_uri(p_ui));
	char *bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char *binary_path = lilv_file_uri_parse(binary_uri, NULL);

	ui_instance = suil_instance_new(
			ui_host,
			this,
			UI_TYPE,
			lilv_node_as_uri(lilv_plugin_get_uri(p_lv2->get_plugin())),
			lilv_node_as_uri(lilv_ui_get_uri(p_ui)),
			lilv_node_as_uri(p_ui_type),
			bundle_path,
			binary_path,
			ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);

	GtkWidget *w = (GtkWidget *)suil_instance_get_widget(
			ui_instance);

	widget = Glib::wrap(w);

	effect_editor_midi.prepend_page(*widget, "LV2 Plugin");

	//need window to be mapped, so wait
	init_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &EffectEditorLV2::initialize),
			50, Glib::PRIORITY_DEFAULT);

	show_all_children();
}

EffectEditorLV2::~EffectEditorLV2() {
	delete widget;
}

void EffectEditorLV2::initialize_lv2_editor() {

	ui_host = suil_host_new(_port_write_func, _port_index_func, NULL, NULL);
}

void EffectEditorLV2::finalize_lv2_editor() {
	suil_host_free(ui_host);
}

static Gtk::Widget *create_lv2_editor(AudioEffect *p_lv2, EffectEditor *p_editor) {

	if (p_lv2->get_provider_id() != AudioEffectProviderLV2::singleton->get_id()) {
		return NULL;
	}

	AudioEffectLV2 *effect = (AudioEffectLV2 *)p_lv2;

	const LilvNode *ui_type;
	const LilvNode *native_ui_type = lilv_new_uri(AudioEffectProviderLV2::get_world(), UI_TYPE);
	LilvUIs *supported_uis = lilv_plugin_get_uis(effect->get_plugin());
	const LilvUI *ui = nullptr;
	LILV_FOREACH(uis, u, supported_uis) {
		const LilvUI *ui = lilv_uis_get(supported_uis, u);
		if (lilv_ui_is_supported(ui,
					suil_ui_supported,
					native_ui_type,
					&ui_type)) {
			break;
		}
	}

	ERR_FAIL_COND_V(ui == nullptr, nullptr);

	return new EffectEditorLV2(effect, p_editor, ui, ui_type, native_ui_type);
}

EffectEditorPluginFunc get_lv2_editor_function() {
	return &create_lv2_editor;
}
