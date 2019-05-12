#include "add_effect_dialog.h"

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
	text += "Provider: " + info->provider_caption + "\n";
	if (info->category != String())
		text += "Category: " + info->category + "\n";
	if (info->author != String())
		text += "Author: " + info->author + "\n";
	if (info->description != String())
		text += "Description:\n" + info->description;

	description_text->set_text(text.utf8().get_data());
}

void AddEffectDialog::_activated(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *p_column) {

	response(Gtk::RESPONSE_OK);
}

int AddEffectDialog::get_selected_effect_index() {

	Gtk::TreeModel::iterator iter = tree_selection->get_selected();
	if (!iter)
		return -1;
	Gtk::TreeModel::Row row = *iter;
	return row[model_columns.index];
}

void AddEffectDialog::update_effect_list() {

	list_store->clear();
	printf("factory: %p\n", fx_factory);

	printf("effect count: %i\n", fx_factory->get_audio_effect_count());

	for (int i = 0; i < fx_factory->get_audio_effect_count(); i++) {

		Gtk::TreeModel::iterator iter = list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[model_columns.name] = fx_factory->get_audio_effect(i)->caption.utf8().get_data();
		row[model_columns.provider] = fx_factory->get_audio_effect(i)->provider_caption.utf8().get_data();
		row[model_columns.index] = i;
	}
}

AddEffectDialog::AddEffectDialog(AudioEffectFactory *p_fx_factory) :
		Gtk::MessageDialog("", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL) {
	set_title("Choose Effect");
	fx_factory = p_fx_factory;
	Gtk::Box &vbox = *get_vbox();
	//add(vbox);
	description_text = Gtk::TextBuffer::create();
	vbox.pack_start(scroll, Gtk::PACK_EXPAND_WIDGET);
	scroll.add(tree);
	description_label.set_label("Effect Description");
	vbox.pack_start(description_label, Gtk::PACK_SHRINK);
	vbox.pack_start(description, Gtk::PACK_SHRINK);
	description.set_buffer(description_text);

	show_all_children();
	vbox.get_children()[0]->hide();
	vbox.set_spacing(0);

	list_store = Gtk::ListStore::create(model_columns);
	tree_selection = tree.get_selection();
	tree_selection->signal_changed().connect(sigc::mem_fun(this, &AddEffectDialog::_selection_changed));

	tree.set_model(list_store);
	tree.append_column("Effect", model_columns.name);
	tree.get_column(0)->set_expand(true);
	tree.append_column("Provider", model_columns.provider);
	tree.get_column(1)->set_expand(false);

	tree.signal_row_activated().connect(sigc::mem_fun(this, &AddEffectDialog::_activated));

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();

	set_default_size(width / 4, height / 2);
}
