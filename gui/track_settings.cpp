#include "track_settings.h"

void AddEffectDialog::_selection_changed() {

	Gtk::TreeModel::iterator iter = tree_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	int selected = row[model_columns.index];

	const AudioEffectInfo *info = fx_factory->get_audio_effect(selected);
	String text;
	text = "Name: " + info->caption + "\n";
	text += String() + "Type: " + (info->synth ? "Synth" : "Effect") + "\n";
	text += "Provider: " + info->provider->get_name() + "\n";
	if (info->category != String())
		text += "Category: " + info->category + "\n";
	if (info->author != String())
		text += "Author: " + info->author + "\n";
	if (info->description != String())
		text += "Description:\n" + info->description;

	description_text->set_text(text.utf8().get_data());
}

void AddEffectDialog::_add_effect_pressed() {
}

void AddEffectDialog::_activated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *p_column) {

	_add_effect_pressed();
}

void AddEffectDialog::update_effect_list() {

	list_store->clear();
	printf("factory: %p\n",fx_factory);

	printf("effect count: %i\n", fx_factory->get_audio_effect_count());


	for (int i = 0; i < fx_factory->get_audio_effect_count(); i++) {

		Gtk::TreeModel::iterator iter = list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[model_columns.name] = fx_factory->get_audio_effect(i)->caption.utf8().get_data();
		row[model_columns.provider] = fx_factory->get_audio_effect(i)->provider->get_name().utf8().get_data();
		row[model_columns.index] = i;
	}
}

AddEffectDialog::AddEffectDialog(AudioEffectFactory *p_fx_factory) {
	fx_factory = p_fx_factory;
	add(vbox);
	description_text = Gtk::TextBuffer::create();
	effect_label.set_label("Effect List:");
	vbox.pack_start(effect_label, Gtk::PACK_SHRINK);
	vbox.pack_start(scroll, Gtk::PACK_EXPAND_WIDGET);
	scroll.add(tree);
	description_label.set_label("Description");
	vbox.pack_start(description_label, Gtk::PACK_SHRINK);
	vbox.pack_start(description, Gtk::PACK_SHRINK);
	description.set_buffer(description_text);
	vbox.pack_start(add_button_hb, Gtk::PACK_SHRINK);
	add_button_hb.pack_start(empty[0], Gtk::PACK_EXPAND_WIDGET);
	add_button.set_label("Add Effect");
	add_button_hb.pack_start(add_button, Gtk::PACK_EXPAND_WIDGET);
	add_button_hb.pack_start(empty[1], Gtk::PACK_EXPAND_WIDGET);

	show_all_children();
	set_title("Effect Selection");

	list_store = Gtk::ListStore::create(model_columns);
	tree_selection = tree.get_selection();
	tree_selection->signal_changed().connect(sigc::mem_fun(this, &AddEffectDialog::_selection_changed));

	tree.set_model(list_store);
	tree.append_column("Effect", model_columns.name);
	tree.append_column("Provider", model_columns.provider);
	tree.signal_row_activated().connect(sigc::mem_fun(this, &AddEffectDialog::_activated));

	set_default_size(400, 600);
}

void TrackSettings::_add_effect() {

	add_effect_dialog.show();
	add_effect_dialog.update_effect_list();
}

TrackSettings::TrackSettings(AudioEffectFactory *p_fx_factory) :
		add_effect_dialog(p_fx_factory) {

	fx_factory = p_fx_factory;

	set_title("Track Settings");

	add(main_hbox);
	main_hbox.pack_start(left_vbox, Gtk::PACK_SHRINK);
	track_list_label.set_text("Track:");
	left_vbox.pack_start(track_list_label, Gtk::PACK_SHRINK);
	left_vbox.pack_start(track_list, Gtk::PACK_SHRINK);
	rack_label.set_text("Synths/Effects:");

	left_vbox.pack_start(rack_label, Gtk::PACK_SHRINK);

	left_vbox.pack_start(rack_menu_hb, Gtk::PACK_SHRINK);

	add_effect.set_label("Add");
	add_effect.signal_pressed().connect(sigc::mem_fun(this, &TrackSettings::_add_effect));
	rack_menu_hb.pack_start(add_effect, Gtk::PACK_EXPAND_WIDGET);
	remove_effect.set_label("Remove");
	rack_menu_hb.pack_start(remove_effect, Gtk::PACK_EXPAND_WIDGET);
	move_effect_up.set_label("Move Up");
	rack_menu_hb.pack_start(move_effect_up, Gtk::PACK_EXPAND_WIDGET);
	move_effect_down.set_label("Move Down");
	rack_menu_hb.pack_start(move_effect_down, Gtk::PACK_EXPAND_WIDGET);

	left_vbox.pack_start(rack, Gtk::PACK_EXPAND_WIDGET);

	show_all_children();

	set_default_size(800, 600);
}
