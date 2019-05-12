#include "effect_editor.h"

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

		//printf("ADDING: %s\n", port->get_name().utf8().get_data());
		row[model_columns.name] = port->get_name().utf8().get_data();
		row[model_columns.visible] = visible;
		row[model_columns.index] = i;
	}

	updating_automation = false;
}

EffectEditor::EffectEditor() {

	track = NULL;
	effect = NULL;

	add(main_vbox);

	main_vbox.pack_start(split, Gtk::PACK_EXPAND_WIDGET);
	split.pack1(automation_scroll, false, false);

	automation_scroll.add(tree);

	list_store = Gtk::ListStore::create(model_columns);
	//	tree_selection = tree.get_selection();

	column.set_title("Automations");
	column.pack_start(cell_render_check, false);
	column.pack_start(cell_render_text, true);
	cell_render_check.signal_toggled().connect(sigc::mem_fun(*this, &EffectEditor::_automation_toggled));
	column.add_attribute(cell_render_check.property_active(), model_columns.visible);
	column.add_attribute(cell_render_text.property_text(), model_columns.name);
	tree.set_model(list_store);
	tree.append_column(column);
	tree.get_column(0)->set_expand(true);
	///
	split.pack2(effect_vbox, true, false);

	show_all_children();

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	automation_scroll.set_size_request(screen->get_height() / 6, screen->get_height() / 4);
	updating_automation = false;
}
