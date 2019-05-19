#ifndef EFFECT_EDITOR_H
#define EFFECT_EDITOR_H

#include "engine/song.h"
#include <gtkmm.h>

class EffectEditor : public Gtk::Window {

	class ModelColumns : public Gtk::TreeModelColumnRecord {
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

		ModelColumns() {
			add(name);
			add(visible);
			add(command);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<bool> visible;
		Gtk::TreeModelColumn<Glib::ustring> command;
		Gtk::TreeModelColumn<int> index;
	};

	Gtk::VBox main_vbox;

	Gtk::HPaned split;

	ModelColumns model_columns;

	AudioEffectFactory *fx_factory;
	Glib::RefPtr<Gtk::ListStore> list_store;
	Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	Gtk::ScrolledWindow automation_scroll;

	Gtk::CellRendererToggle cell_render_check;
	Gtk::CellRendererCombo cell_render_command;
	Gtk::CellRendererText cell_render_text;
	Gtk::TreeViewColumn column;
	Gtk::TreeViewColumn column2;
	Gtk::TreeView tree;
	Gtk::VBox effect_vbox;
	Gtk::Widget *editor;

	Song *song;
	int track_index;
	Track *track;
	AudioEffect *effect;

	Glib::RefPtr<Gtk::ListStore> command_list_store;
	//Glib::RefPtr<Gtk::TreeSelection> tree_selection;

	void _automation_toggled(const Glib::ustring &path);
	void _command_edited(const Glib::ustring &path, const Glib::ustring &value);
	bool updating_automation;

	void _automation_rmb(GdkEventButton *button);
	bool _automation_menu_timeout();
	void _automation_menu_action();

	Gtk::Menu automation_popup;
	Gtk::MenuItem automation_popup_item;

	//because GTK is horrible
	sigc::connection menu_timer;

	//hide on escape
	virtual bool on_key_press_event(GdkEventKey *key_event) {

		if (key_event->keyval == GDK_KEY_Escape) {
			hide();
		}

		return false;
	}

public:
	sigc::signal4<void, Track *, AudioEffect *, int, bool> toggle_automation_visibility;
	sigc::signal4<void, Track *, AudioEffect *, int, int> select_automation_command;

	void update_automations();
	void edit(AudioEffect *p_effect, Track *p_track, Gtk::Widget *p_editor);
	EffectEditor();
};

typedef Gtk::Widget *(*EffectEditorPluginFunc)(AudioEffect *, EffectEditor *);

#endif // EFFECT_EDITOR_H
