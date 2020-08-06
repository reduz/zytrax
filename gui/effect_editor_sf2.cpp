#include "effect_editor_sf2.h"

void EffectEditorSF2::_open_soundfont() {

	Gtk::FileChooserDialog dialog("Select a soundfont file to open",
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	dialog.set_transient_for(*editor);
	dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	//Add response buttons the the dialog:
	gboolean swap_buttons;
	g_object_get(gtk_settings_get_default(), "gtk-alternative-button-order", &swap_buttons, NULL);
	if (swap_buttons) {
		dialog.add_button("Select", Gtk::RESPONSE_OK);
		dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
	} else {
		dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		dialog.add_button("Select", Gtk::RESPONSE_OK);
	}

	auto filter_zt = Gtk::FileFilter::create();
	filter_zt->set_name("SoundFont 2");
	filter_zt->add_pattern("*.sf2");
	dialog.add_filter(filter_zt);

	int result = dialog.run();
	dialog.hide();

	if (result == Gtk::RESPONSE_OK) {
		String path;
		path.parse_utf8(dialog.get_filename().c_str());
		Error err = synth_sf2->load_soundfount(path);

		if (err != OK) {
			String err_str;
			err_str = "Error: Can't open file (invalid format?).";

			Gtk::MessageDialog error_box(err_str.utf8().get_data(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
			error_box.set_transient_for(*editor);
			error_box.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			error_box.run();
			error_box.hide();
		} else {
			_update_patches();
		}
	}
}

void EffectEditorSF2::_selection_changed() {
	if (updating) {
		return;
	}

	Gtk::TreeModel::iterator iter = tree_selection->get_selected();
	if (!iter)
		return;
	Gtk::TreeModel::Row row = *iter;
	int selected = row[model_columns.index];

	synth_sf2->set_preset(selected);
}

void EffectEditorSF2::_update_patches() {

	updating = true;
	list_store->clear();

	for (int i = 0; i < synth_sf2->get_preset_count(); i++) {

		Gtk::TreeModel::iterator iter = list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[model_columns.name] = synth_sf2->get_preset_name(i).utf8().get_data();
		row[model_columns.index] = i;

		if (i == synth_sf2->get_preset()) {
			tree_selection->select(row);
		}
	}
	updating = false;
}

EffectEditorSF2::EffectEditorSF2(AudioSynthSF2 *p_sf2, EffectEditor *p_editor) :
		effect_editor_midi(p_sf2, p_editor) {

	synth_sf2 = p_sf2;
	editor = p_editor;

	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);
	effect_editor_midi.prepend_page(vbox, "Patch List");

	open_soundfont.set_label("Open SoundFont");
	open_soundfont.signal_clicked().connect(sigc::mem_fun(*this, &EffectEditorSF2::_open_soundfont));

	vbox.pack_start(open_soundfont, Gtk::PACK_SHRINK);
	vbox.pack_start(scroll, Gtk::PACK_EXPAND_WIDGET);
	scroll.add(tree);

	list_store = Gtk::ListStore::create(model_columns);
	tree_selection = tree.get_selection();
	tree_selection->signal_changed().connect(sigc::mem_fun(this, &EffectEditorSF2::_selection_changed));

	tree.set_model(list_store);
	tree.append_column("#", model_columns.index);
	tree.get_column(0)->set_expand(false);
	tree.append_column("Patch Name", model_columns.name);
	tree.get_column(1)->set_expand(true);

	//tree.signal_row_activated().connect(sigc::mem_fun(this, &AddEffectDialog::_activated));

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();
	//	set_default_size(width / 2, height / 2);
	vbox.set_size_request(width / 4, height / 2);

	show_all_children();

	updating = false;
	_update_patches();
}

EffectEditorSF2::~EffectEditorSF2() {
}

Gtk::Widget *create_sf2_editor(AudioEffect *p_vst, EffectEditor *p_editor) {
	if (p_vst->get_provider_id() != "internal" || p_vst->get_unique_id() != "sf2") {
		return NULL;
	}
	return new EffectEditorSF2(static_cast<AudioSynthSF2 *>(p_vst), p_editor);
}
