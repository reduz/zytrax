#include "effect_editor.h"
#include "settings_dialog.h"
void EffectEditor::edit(AudioEffect *p_effect, Track *p_track, Gtk::Widget *p_editor) {

	effect = p_effect;
	track = p_track;
	editor = p_editor;

	effect_vbox.pack_start(*p_editor, Gtk::PACK_EXPAND_WIDGET);
	p_editor->show();
	update_automations();
}

void EffectEditor::_automation_toggled(const Glib::ustring &path) {

	if (updating_automation) {
		return;
	}
	updating_automation = true;
	Gtk::TreeIter iter = list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	bool visible = (*iter)[model_columns.visible];
	(*iter)[model_columns.visible] = !visible;

	toggle_automation_visibility.emit(track, effect, (*iter)[model_columns.index], !visible);
	updating_automation = false;
}

void EffectEditor::update_automations() {

	if (updating_automation) {
		return;
	}

	updating_automation = true;

	list_store->clear();

	for (int i = 0; i < effect->get_control_port_count(); i++) {

		ControlPort *port = effect->get_control_port(i);
		if (!port->is_visible()) {
			continue;
		}
		Gtk::TreeModel::iterator iter = list_store->append();
		Gtk::TreeModel::Row row = *iter;

		bool visible = false;

		for (int j = 0; j < track->get_automation_count(); j++) {
			if (track->get_automation(j)->get_control_port() == port) {
				visible = true;
				break;
			}
		}

		row[model_columns.name] = port->get_name().utf8().get_data();
		row[model_columns.visible] = visible;
		//row[model_columns.commands] = command_list_store;
		if (port->get_command() == 0) {
			row[model_columns.command] = "<assign>";
		} else {
			char s[2] = { char('A' + (port->get_command() - 'a')), 0 };
			row[model_columns.command] = s;
		}
		row[model_columns.index] = i;
	}

	updating_automation = false;
}

void EffectEditor::_command_edited(const Glib::ustring &path, const Glib::ustring &value) {

	Gtk::TreeIter iter = list_store->get_iter(path);
	ERR_FAIL_COND(!iter);

	Glib::ustring us = value;
	if (value.length() == 0) {
		return;
	}

	updating_automation = true;
	char valc = value[0];
	if (valc == '<') {
		valc = 0; //unselected
		(*iter)[model_columns.command] = "<assign>";
	} else {
		valc = 'a' + (valc - 'A'); //unselected
		(*iter)[model_columns.command] = value;
	}

	select_automation_command.emit(track, effect, (*iter)[model_columns.index], int(valc));
	updating_automation = false;
}

bool EffectEditor::_automation_menu_timeout() {

	Gtk::TreeModel::iterator iter = tree_selection->get_selected();
	if (!iter)
		return false;
	Gtk::TreeModel::Row row = *iter;
	int selected = row[model_columns.index];
	if (effect->get_control_port(selected)->get_command() == 0) {
		automation_popup_item.set_sensitive(false);
	} else {
		automation_popup_item.set_sensitive(true);
	}

	return false;
}
void EffectEditor::_automation_rmb(GdkEventButton *button_event) {

	if ((button_event->type == GDK_BUTTON_PRESS) && (button_event->button == 3)) {

		automation_popup.popup_at_pointer((GdkEvent *)button_event);
		//we can only override this only BEFORE the event, so selection is wrong, adjust sensitivity in a timer :(
		menu_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &EffectEditor::_automation_menu_timeout),
				1, Glib::PRIORITY_DEFAULT);
	}
}

void EffectEditor::_automation_menu_action() {

	Gtk::TreeModel::iterator iter = tree_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	int selected = row[model_columns.index];
	ERR_FAIL_COND(effect->get_control_port(selected)->get_command() == 0);
	String identifier = effect->get_control_port(selected)->get_identifier();

	SettingsDialog::add_default_command(identifier, effect->get_control_port(selected)->get_command());
}

EffectEditor::EffectEditor() {

	track = NULL;
	effect = NULL;

	add(main_vbox);

	main_vbox.pack_start(split, Gtk::PACK_EXPAND_WIDGET);
	split.pack1(automation_scroll, false, false);

	automation_scroll.add(tree);

	list_store = Gtk::ListStore::create(model_columns);
	tree_selection = tree.get_selection();

	tree.set_model(list_store);

	column.set_title("Automations");
	column.pack_start(cell_render_check, false);
	column.pack_start(cell_render_text, true);
	cell_render_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditor::_automation_toggled));
	column.add_attribute(cell_render_check.property_active(), model_columns.visible);
	column.add_attribute(cell_render_text.property_text(), model_columns.name);
	tree.set_model(list_store);
	tree.append_column(column);
	tree.get_column(0)->set_expand(true);

	command_list_store = Gtk::ListStore::create(model_columns.command_model_columns);
	{
		{
			Gtk::TreeModel::iterator iter = command_list_store->append();
			Gtk::TreeModel::Row row = *iter;
			row[model_columns.command_model_columns.name] = "<unassigned>";
			row[model_columns.command_model_columns.index] = 0;
		}
		for (int i = 'a'; i <= 'z'; i++) {

			Gtk::TreeModel::iterator iter = command_list_store->append();
			Gtk::TreeModel::Row row = *iter;

			const char s[2] = { char('A' + (i - 'a')), 0 };
			row[model_columns.command_model_columns.name] = s;
			row[model_columns.command_model_columns.index] = i;
		}
	}

	column2.set_title("Command");
	column2.pack_start(cell_render_command, false);
	column2.add_attribute(cell_render_command.property_text(), model_columns.command);
	cell_render_command.signal_edited().connect(sigc::mem_fun(*this, &EffectEditor::_command_edited));

	cell_render_command.property_model() = command_list_store;
	cell_render_command.property_text_column() = 0;
	cell_render_command.property_editable() = true;
	cell_render_command.property_has_entry() = false;

	cell_render_command.set_visible(true);

	tree.append_column(column2);
	tree.get_column(1)->set_expand(false);
	tree.set_can_focus(false);
	//tree_selection->set_mode(Gtk::SELECTION_NONE);

	tree.signal_button_press_event().connect_notify(sigc::mem_fun(*this, &EffectEditor::_automation_rmb));

	automation_popup_item.set_label("Make Command Default");
	automation_popup_item.signal_activate().connect(sigc::mem_fun(*this, &EffectEditor::_automation_menu_action));
	automation_popup_item.show();
	automation_popup.append(automation_popup_item);

	////////////////
	split.pack2(effect_vbox, true, false);

	show_all_children();

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	//automation_scroll.set_size_request(screen->get_height() / 5, screen->get_height() / 4);
	automation_scroll.set_propagate_natural_width(true);
	automation_scroll.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);

	updating_automation = false;
}
