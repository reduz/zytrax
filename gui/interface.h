#ifndef INTERFACE_H
#define INTERFACE_H

#include "engine/song.h"
#include "engine/song_file.h"
#include "gui/add_effect_dialog.h"
#include "gui/effect_editor.h"
#include "gui/orderlist_editor.h"
#include "gui/pattern_editor.h"
#include "gui/settings_dialog.h"
#include "gui/track_editor.h"
#include <gtkmm.h>

class Interface : public Gtk::ApplicationWindow {
private:
	enum {
		FILE_NEW,
		FILE_OPEN,
		FILE_SAVE,
		FILE_SAVE_AS,
		FILE_QUIT,
		SETTINGS_CONFIG,
		SETTINGS_ABOUT

	};

	//dear GTK, why all this for a simple combo?
	class ModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
	};

	ModelColumns zoom_model_columns;
	Glib::RefPtr<Gtk::ListStore> zoomlist_store;
	Vector<Gtk::TreeModel::Row> zoom_rows;

	Gtk::Image prev_pattern_icon;
	Gtk::Button prev_pattern;

	Gtk::Image play_icon;
	Gtk::Button play;

	Gtk::Image stop_icon;
	Gtk::Button stop;

	Gtk::Image next_pattern_icon;
	Gtk::Button next_pattern;

	//separator is broken in GTK, so using a Label :(
	Gtk::Label sep1, sep2, sep3;

	Gtk::Image play_pattern_icon;
	Gtk::Button play_pattern;

	Gtk::Image play_cursor_icon;
	Gtk::Button play_cursor;

	Gtk::Image add_track_icon;
	Gtk::Button add_track;

	Gtk::Label spacer1, spacer2;

	Glib::RefPtr<Gio::Menu> menu;
	Glib::RefPtr<Gio::Menu> file_menu;
	Glib::RefPtr<Gio::Menu> file_menu_file;
	Glib::RefPtr<Gio::Menu> file_menu_exit;

	Glib::RefPtr<Gio::Menu> play_menu;
	Glib::RefPtr<Gio::Menu> play_menu_play;
	Glib::RefPtr<Gio::Menu> play_menu_seek;
	Glib::RefPtr<Gio::Menu> play_menu_pattern;
	Glib::RefPtr<Gio::Menu> play_menu_extra;

	Glib::RefPtr<Gio::Menu> edit_menu;
	Glib::RefPtr<Gio::Menu> edit_menu_info;
	Glib::RefPtr<Gio::Menu> edit_menu_undo;
	Glib::RefPtr<Gio::Menu> edit_menu_focus;

	Glib::RefPtr<Gio::Menu> track_menu;
	Glib::RefPtr<Gio::Menu> track_menu_add;
	Glib::RefPtr<Gio::Menu> track_menu_column;
	Glib::RefPtr<Gio::Menu> track_menu_solo;
	Glib::RefPtr<Gio::Menu> track_menu_edit;
	Glib::RefPtr<Gio::Menu> track_menu_remove;

	Glib::RefPtr<Gio::SimpleAction> automation_action;
	Glib::RefPtr<Gio::Menu> automation_menu;
	Glib::RefPtr<Gio::MenuItem> automation_menu_item;
	Glib::RefPtr<Gio::Menu> automation_menu_visible;
	Glib::RefPtr<Gio::Menu> automation_menu_mode;
	Glib::RefPtr<Gio::Menu> automation_menu_move;
	Glib::RefPtr<Gio::Menu> automation_menu_remove;

	Glib::RefPtr<Gio::Menu> select_menu;
	Glib::RefPtr<Gio::Menu> select_menu_select;
	Glib::RefPtr<Gio::Menu> select_menu_clipboard;
	Glib::RefPtr<Gio::Menu> select_menu_transpose;
	Glib::RefPtr<Gio::Menu> select_menu_operations;
	Glib::RefPtr<Gio::Menu> select_menu_length;

	Glib::RefPtr<Gio::Menu> settings_menu;
	Glib::RefPtr<Gio::Menu> settings_menu_preferences;
	Glib::RefPtr<Gio::Menu> settings_menu_cheat;
	Glib::RefPtr<Gio::Menu> settings_menu_about;

	AddEffectDialog add_effect_dialog;

	Gtk::MenuItem menu_item_file_open;

	Vector<Gtk::MenuItem *> menu_items;

	/* Boxes */
	Gtk::Grid grid;
	Gtk::VBox main_vbox;
	Gtk::HBox play_hbox;
	Gtk::HBox pattern_hbox;
	Gtk::VBox pattern_vbox;
	Gtk::HBox main_hbox;
	/* Labels */
	Gtk::Label pattern_label;
	Gtk::Label pattern_length_label;
	Gtk::Label octave_label;
	Gtk::Label step_label;
	Gtk::Label zoom_label;
	Gtk::CheckButton volume_mask;
	Gtk::Label tempo_label;
	Gtk::Label swing_label;
	/* Scrolls */
	Gtk::VScrollbar pattern_vscroll;
	Gtk::HScrollbar pattern_hscroll;
	Gtk::VScrollbar orderlist_vscroll;

	Gtk::SpinButton pattern;
	Gtk::Image pattern_settings_icon;
	Gtk::Button pattern_settings;
	Gtk::SpinButton pattern_length;
	Gtk::Button pattern_length_set_next;
	Gtk::SpinButton octave;
	Gtk::SpinButton volume;
	Gtk::SpinButton tempo;
	Gtk::SpinButton swing;
	Gtk::SpinButton step;
	Gtk::ComboBox zoom;

	Gtk::VPaned main_split;
	Gtk::ScrolledWindow track_scroll;
	Gtk::HBox track_hbox;

	KeyBindings *key_bindings;

	/* Editors */

	UndoRedo undo_redo;
	Song song;
	SongFile song_file;

	String song_path;
	int save_version;

	void _update_title();

	Theme *theme;
	PatternEditor pattern_editor;
	OrderlistEditor orderlist_editor;
	AudioEffectFactory *fx_factory;

	struct TrackRacks {
		TrackRackVolume *volume;
		TrackRackEditor *rack;
		Gtk::VScrollbar *v_scroll;
	};

	Vector<TrackRacks> racks;
	TrackRackFiller *rack_filler;
	SettingsDialog settings_dialog;

	/* Data */

	void _add_track();

	void _pattern_changed();
	void _octave_changed();
	void _step_changed();
	void _volume_changed();
	void _tempo_changed();
	void _swing_changed();
	void _zoom_changed();

	bool updating_editors;
	void _update_editors();

	Gtk::Application *application;

	void _on_application_startup();
	void _on_action_activated(KeyBindings::KeyBind p_bind);

	void _update_selected_track();
	void _update_tracks();
	void _ensure_selected_track_visible();

	void _update_volume_mask();
	void _update_octave();
	void _update_pattern();
	void _update_step();
	void _update_zoom();

	void _redraw_track_edits();
	void _on_add_effect(int p_track);
	void _on_toggle_effect_skip(int p_track, int p_effect);
	void _on_toggle_send_mute(int p_track, int p_send);
	void _on_remove_effect(int p_track, int p_effect);
	void _on_remove_send(int p_track, int p_send);
	void _on_track_insert_send(int p_track, int p_to_track);
	void _on_track_send_amount_changed(int p_track, int p_send, float p_amount);
	void _on_track_swap_effects(int p_track, int p_effect, int p_with_effect);
	void _on_track_swap_sends(int p_track, int p_send, int p_with_send);
	void _on_effect_request_editor(int p_track, int p_effect);
	void _update_editor_automations_for_effect(AudioEffect *p_effect);
	void _on_toggle_automation_visibility(Track *p_track, AudioEffect *p_effect, int p_automation, bool p_visible);

	enum {
		MAX_EFFECT_EDITOR_PLUGINS = 1024
	};

	EffectEditorPluginFunc plugin_editor_create_functions[MAX_EFFECT_EDITOR_PLUGINS];
	int plugin_editor_function_count;

	Map<AudioEffect *, EffectEditor *> active_effect_editors;

	Gtk::Popover pattern_settings_popover;
	Gtk::Grid pattern_settings_grid;
	Gtk::HSeparator pattern_settings_vsep;
	Gtk::SpinButton pattern_settings_length;
	Gtk::SpinButton bar_length;
	Gtk::SpinButton change_next;
	Gtk::Label pattern_settings_length_label;
	Gtk::Label bar_length_label;
	Gtk::Label change_next_label;
	Gtk::Button pattern_settings_change_button;

	void _on_pattern_settings_open();
	void _on_pattern_settings_change();

	void _update_colors();
	static void _undo_redo_action(const String &p_name, void *p_userdata);

	bool _close_request(GdkEventAny *event);

public:
	void add_editor_plugin_function(EffectEditorPluginFunc p_plugin);

	Interface(Gtk::Application *p_application, AudioEffectFactory *p_fx_factory, Theme *p_theme, KeyBindings *p_key_bindings);

	~Interface();
};

#endif // INTERFACE_H
