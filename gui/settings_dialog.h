#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include "engine/audio_effect.h"
#include "engine/sound_driver_manager.h"
#include "gui/color_theme.h"
#include "gui/key_bindings.h"
#include "vector.h"

#include <gtkmm.h>

class ThemeColorList : public Gtk::Widget {
protected:
	int font_height;
	int font_ascent;
	int selected;

	// Overrides:
	void get_preferred_width_vfunc(int &minimum_width,
			int &natural_width) const override;
	void get_preferred_height_for_width_vfunc(int width, int &minimum_height,
			int &natural_height) const override;
	void get_preferred_height_vfunc(int &minimum_height,
			int &natural_height) const override;
	void get_preferred_width_for_height_vfunc(int height, int &minimum_width,
			int &natural_width) const override;
	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	bool on_button_press_event(GdkEventButton *event);
	bool on_key_press_event(GdkEventKey *key_event);

	Glib::RefPtr<Gdk::Window> m_refGdkWindow;

	Theme *theme;

	void _draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, const String &p_text,
			const Gdk::RGBA &p_color, bool p_down);
	int _get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const;
	void _draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
			int x, int y, int w, int h,
			const Gdk::RGBA &p_color);
	void _draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
			int y, int w, int h, const Gdk::RGBA &p_color);

public:
	sigc::signal1<void, int> color_selected;

	void select_color(int p_selected) {
		selected = p_selected;
		queue_draw();
	}
	int get_selected() const { return selected; }
	ThemeColorList(Theme *p_theme);
	~ThemeColorList();
};

class SettingsDialog : public Gtk::MessageDialog {

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

	ModelColumns model_columns;

	Glib::RefPtr<Gtk::ListStore> driver_list_store;
	Vector<Gtk::TreeModel::Row> driver_rows;

	Glib::RefPtr<Gtk::ListStore> frequency_list_store;
	Vector<Gtk::TreeModel::Row> frequency_rows;

	Glib::RefPtr<Gtk::ListStore> buffer_list_store;
	Vector<Gtk::TreeModel::Row> buffer_rows;

	Glib::RefPtr<Gtk::ListStore> step_list_store;
	Vector<Gtk::TreeModel::Row> step_rows;

	Gtk::Notebook notebook;
	Gtk::VBox main_vbox;
	Gtk::Frame sound_settings_frame;
	Gtk::Grid sound_settings_grid;

	Gtk::ComboBox driver_combo;
	Gtk::Label driver_label;
	Gtk::ComboBox frequency_combo;
	Gtk::Label frequency_label;
	Gtk::ComboBox buffer_combo;
	Gtk::Label buffer_label;
	Gtk::ComboBox step_combo;
	Gtk::Label step_label;

	Gtk::Frame plugin_path_frame;
	Gtk::VBox plugin_path_vbox;

	void _driver_changed();
	void _driver_freq_changed();
	void _driver_buffer_changed();
	void _driver_step_changed();

	class PluginModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		PluginModelColumns() {
			add(label);
			add(text);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::ustring> text;
		Gtk::TreeModelColumn<int> index;
	};
	PluginModelColumns plugin_model_columns;
	Glib::RefPtr<Gtk::ListStore> plugin_list_store;
	Glib::RefPtr<Gtk::TreeSelection> plugin_tree_selection;
	Gtk::TreeViewColumn plugin_column;
	Gtk::CellRendererText plugin_column_text;
	Gtk::ScrolledWindow plugin_scroll;
	Gtk::TreeView plugin_tree;
	Gtk::HBox plugin_hbox;
	Gtk::Button plugin_browse_path;
	Gtk::Button scan_plugins;
	void _browse_plugin_path();
	void _scan_plugins();
	void _plugin_path_edited(const Glib::ustring &path, const Glib::ustring &text);
	////////////

	Gtk::VBox theme_vbox;
	Gtk::Frame theme_font_frame;
	Gtk::Grid theme_font_grid;
	Gtk::Label theme_font_label;
	Gtk::FontButton theme_font_button;
	Gtk::Frame theme_colors_frame;
	Gtk::Grid theme_colors_grid;
	Gtk::ScrolledWindow theme_color_list_scroll;
	ThemeColorList theme_color_list;
	Gtk::ColorButton theme_color_change;
	Gtk::CheckButton theme_force_dark;
	Gtk::Label theme_color_label;
	Gtk::Frame theme_settings_frame;
	Gtk::Grid theme_settings_grid;

	void _color_selected(int p_index);
	void _choose_color();
	void _font_chosen();
	void _on_dark_theme_chosen();
	Theme *theme;

	//////// shortcut editor /////////
	class ShortcutModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ShortcutModelColumns() {
			add(label);
			add(text);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> label;
		Gtk::TreeModelColumn<Glib::ustring> text;
		Gtk::TreeModelColumn<int> index;
	};

	Gtk::VBox shortcut_vbox;
	Gtk::Frame shortcut_frame;
	Gtk::Grid shortcut_grid;

	ShortcutModelColumns shortcut_model_columns;
	Glib::RefPtr<Gtk::ListStore> shortcut_list_store;
	Glib::RefPtr<Gtk::TreeSelection> shortcut_tree_selection;
	Gtk::ScrolledWindow shortcut_scroll;
	Gtk::TreeView shortcut_tree;
	Gtk::Button shortcut_assign;
	Vector<Gtk::TreeModel::Row> shortcut_rows;
	Gtk::Button shortcut_assign_button;
	Gtk::Button shortcut_clear_button;
	Gtk::Button shortcut_reset_button;

	Gtk::MessageDialog key_remap_dialog;
	int key_remap_key;
	int key_remap_mod;
	bool _signal_remap_key(GdkEventKey *p_key);

	void _shortcut_assign();
	void _shortcut_clear();
	void _shortcut_restore();

	KeyBindings *key_bindings;

	void _save_settings();

	class ScanColumns : public Gtk::TreeModelColumnRecord {
	public:
		ScanColumns() {
			add(name);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	Gtk::ScrolledWindow scan_scroll;
	static void scan_callback(const String &p_text, void *p_userdata);
	ScanColumns scan_model_columns;
	Glib::RefPtr<Gtk::ListStore> scan_list_store;
	Glib::RefPtr<Gtk::TreeSelection> scan_tree_selection;
	Gtk::TreeView scan_tree;

	void _save_plugins();

	bool _scan_plugin_key(GdkEvent *p_key);
	static void _scan_callback(const String &, void *p_ud);

	AudioEffectFactory *fx_factory;

public:
	enum {
		MAX_DEFAULT_COMMANDS = 100
	};

private:
	struct DefaultCommand {
		String name;
		char command;
	};

	static DefaultCommand default_commands[MAX_DEFAULT_COMMANDS];

	class CommandEditorModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		//GTK is beyond bizarre at this point

		class CommandModelColumns : public Gtk::TreeModelColumnRecord {
		public:
			CommandModelColumns() {
				add(name);
				add(index);
			}

			Gtk::TreeModelColumn<Glib::ustring> name;
			Gtk::TreeModelColumn<int> index;
		};

		CommandModelColumns command_model_columns;

		CommandEditorModelColumns() {
			add(name);
			add(command);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> command;
		Gtk::TreeModelColumn<int> index;
	};

	CommandEditorModelColumns command_editor_columns;

	Glib::RefPtr<Gtk::ListStore> command_list_store;
	Glib::RefPtr<Gtk::TreeSelection> command_tree_selection;

	Gtk::CellRendererText cell_render_text;
	Gtk::CellRendererCombo cell_render_command;
	Gtk::TreeViewColumn command_column1;
	Gtk::TreeViewColumn command_column2;
	Gtk::TreeView command_tree;
	Gtk::ScrolledWindow command_tree_scroll;

	Glib::RefPtr<Gtk::ListStore> command_commands_list_store;
	//Glib::RefPtr<Gtk::TreeSelection> tree_selection;
	Gtk::Frame command_frame;

	void _update_command_list();
	void _command_name_changed(const Glib::ustring &path, const Glib::ustring &text);
	void _command_value_changed(const Glib::ustring &path, const Glib::ustring &value);

	static SettingsDialog *singleton;

public:
	sigc::signal0<void> update_colors;
	sigc::signal0<void> update_song_step_buffer;
	sigc::signal0<void> update_mix_rate;

	static void set_default_command(int p_index, const String &p_name, char p_command);
	static String get_default_command_name(int p_index);
	static char get_default_command_command(int p_index);
	static void add_default_command(const String &p_name, char p_command);

	void initialize_bindings();
	SettingsDialog(Theme *p_theme, KeyBindings *p_key_bindings, AudioEffectFactory *p_fx_factory);
	static String get_settings_path();
};

#endif // SETTINGS_DIALOG_H
