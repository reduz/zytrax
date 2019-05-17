#include "settings_dialog.h"
#include "engine/audio_effect.h"
#include "globals/json_file.h"

bool ThemeColorList::on_button_press_event(GdkEventButton *event) {
	if (event->button == 1) {
		//select
		selected = event->y / font_height;
		if (selected < 0 || selected >= Theme::COLOR_MAX) {
			selected = -1;
		}
		color_selected.emit(selected);
		queue_draw();
	}

	return false;
}

bool ThemeColorList::on_key_press_event(GdkEventKey *key_event) {

	return true;
}

void ThemeColorList::get_preferred_width_vfunc(int &minimum_width,
		int &natural_width) const {

	minimum_width = 1;
	natural_width = 1;
}

void ThemeColorList::get_preferred_height_for_width_vfunc(
		int /* width */, int &minimum_height, int &natural_height) const {
	minimum_height = font_height * Theme::COLOR_MAX;
	natural_height = font_height * Theme::COLOR_MAX;
}

void ThemeColorList::get_preferred_height_vfunc(int &minimum_height,
		int &natural_height) const {
	minimum_height = font_height * Theme::COLOR_MAX;
	natural_height = font_height * Theme::COLOR_MAX;
}

void ThemeColorList::get_preferred_width_for_height_vfunc(
		int /* height */, int &minimum_width, int &natural_width) const {
	minimum_width = 1;
	natural_width = 1;
}

void ThemeColorList::on_size_allocate(Gtk::Allocation &allocation) {
	// Do something with the space that we have actually been given:
	//(We will not be given heights or widths less than we have requested, though
	// we might get more)

	// Use the offered allocation for this container:
	set_allocation(allocation);

	if (m_refGdkWindow) {
		m_refGdkWindow->move_resize(allocation.get_x(), allocation.get_y(),
				allocation.get_width(),
				allocation.get_height());
	}
}

void ThemeColorList::on_realize() {
	// Do not call base class Gtk::Widget::on_realize().
	// It's intended only for widgets that set_has_window(false).

	set_realized();

	if (!m_refGdkWindow) {
		// Create the GdkWindow:

		GdkWindowAttr attributes;
		memset(&attributes, 0, sizeof(attributes));

		Gtk::Allocation allocation = get_allocation();

		// Set initial position and size of the Gdk::Window:
		attributes.x = allocation.get_x();
		attributes.y = allocation.get_y();
		attributes.width = allocation.get_width();
		attributes.height = allocation.get_height();

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK | Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK |
								Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
								Gdk::BUTTON1_MOTION_MASK | Gdk::KEY_PRESS_MASK |
								Gdk::KEY_RELEASE_MASK;
		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.wclass = GDK_INPUT_OUTPUT;

		m_refGdkWindow = Gdk::Window::create(get_parent_window(), &attributes,
				GDK_WA_X | GDK_WA_Y);
		set_window(m_refGdkWindow);

		// make the widget receive expose events
		m_refGdkWindow->set_user_data(gobj());
	}
}

void ThemeColorList::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void ThemeColorList::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, const String &p_text,
		const Gdk::RGBA &p_color, bool p_down) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->move_to(x, y);
	if (p_down)
		cr->rotate_degrees(90);
	cr->show_text(p_text.utf8().get_data());
	if (p_down)
		cr->rotate_degrees(-90);
	cr->move_to(0, 0);
	cr->stroke();
}

int ThemeColorList::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}
void ThemeColorList::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void ThemeColorList::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

bool ThemeColorList::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	{
		//update min width
		theme->select_font_face(cr);
		Cairo::FontExtents fe;
		cr->get_font_extents(fe);

		if (font_height != fe.height || font_ascent != fe.ascent) {
			font_height = fe.height;
			font_ascent = fe.ascent;
			queue_resize();
		}
	}

	Gdk::RGBA white;
	white.set_red(1.0);
	white.set_green(1.0);
	white.set_blue(1.0);
	white.set_alpha(1.0);

	Gdk::RGBA black;
	black.set_red(0.0);
	black.set_green(0.0);
	black.set_blue(0.0);
	black.set_alpha(1.0);

	Gdk::RGBA hilited;
	hilited.set_red(1.0);
	hilited.set_green(0.4);
	hilited.set_blue(0.4);
	hilited.set_alpha(1.0);

	for (int i = 0; i < Theme::COLOR_MAX; i++) {
		Gdk::Cairo::set_source_rgba(cr, theme->colors[i]);
		cr->rectangle(0, i * font_height, w, font_height);
		cr->fill();
		int tw = _get_text_width(cr, theme->color_names[i]);
		int x_ofs = (w - tw) / 2;
		int y_ofs = i * font_height + font_ascent;
		_draw_text(cr, x_ofs - 1, y_ofs, theme->color_names[i], black, false);
		_draw_text(cr, x_ofs + 1, y_ofs, theme->color_names[i], black, false);
		_draw_text(cr, x_ofs, y_ofs - 1, theme->color_names[i], black, false);
		_draw_text(cr, x_ofs, y_ofs + 1, theme->color_names[i], black, false);

		if (i == selected) {
			_draw_rect(cr, 0, i * font_height, w, font_height - 1, white);
			_draw_text(cr, x_ofs, y_ofs, theme->color_names[i], hilited, false);
		} else {
			_draw_text(cr, x_ofs, y_ofs, theme->color_names[i], white, false);
		}
	}

	return false;
}

ThemeColorList::ThemeColorList(Theme *p_theme) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("theme_colors"),
		Gtk::Widget() {

	font_height = 1;
	font_ascent = 1;
	selected = 0;
	theme = p_theme;

	set_has_window(true);
	set_can_focus(true);
	set_focus_on_click(true);

	set_name("theme_color_list");
}

ThemeColorList::~ThemeColorList() {
}

////////////////////////////////

void SettingsDialog::_driver_changed() {

	Gtk::TreeModel::iterator iter = driver_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[model_columns.index];

			SoundDriverManager::init_driver(id);
			_save_settings();
		}
	}
}
void SettingsDialog::_driver_freq_changed() {

	Gtk::TreeModel::iterator iter = frequency_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[model_columns.index];

			SoundDriverManager::set_mix_frequency(SoundDriverManager::MixFrequency(id));
			SoundDriverManager::init_driver();
			update_mix_rate.emit();
			_save_settings();
		}
	}
}
void SettingsDialog::_driver_buffer_changed() {

	Gtk::TreeModel::iterator iter = buffer_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[model_columns.index];

			SoundDriverManager::set_buffer_size(SoundDriverManager::BufferSize(id));
			SoundDriverManager::init_driver();
			_save_settings();
		}
	}
}
void SettingsDialog::_driver_step_changed() {

	Gtk::TreeModel::iterator iter = step_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[model_columns.index];

			SoundDriverManager::set_step_buffer_size(SoundDriverManager::BufferSize(id));
			//but actually not here..
			update_song_step_buffer.emit();
			_save_settings();
		}
	}
}

bool SettingsDialog::_scan_plugin_key(GdkEvent *p_key) {
	//avoid closing scan with escape key
	if (p_key->type == GDK_KEY_PRESS && ((GdkEventKey *)(p_key))->keyval == GDK_KEY_Escape) {
		return true;
	} else {
		return false;
	}
}

void SettingsDialog::_scan_callback(const String &p_name, void *p_ud) {
	SettingsDialog *sd = (SettingsDialog *)p_ud;
	Gtk::TreeModel::Row row = *(sd->scan_list_store->append());
	row[sd->scan_model_columns.name] = p_name.utf8().get_data();
	while (gtk_events_pending()) {
		gtk_main_iteration_do(false);
	}
}
void SettingsDialog::_scan_plugins() {

	MessageDialog scan("", false /* use_markup */, Gtk::MESSAGE_OTHER, Gtk::BUTTONS_NONE);
	scan.get_vbox()->get_children()[0]->hide();
	scan.get_vbox()->set_spacing(0);

	scan.get_vbox()->pack_start(scan_scroll, Gtk::PACK_EXPAND_WIDGET);
	Gtk::Button *response_button = scan.add_button("Close", Gtk::RESPONSE_OK);
	response_button->set_sensitive(false);

	scan_list_store->clear();

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();

	scan.set_default_size(width / 5, height / 3);
	scan.show_all_children();
	scan.get_vbox()->get_children()[0]->hide();
	scan.set_deletable(false);
	scan.set_transient_for(*this);
	scan.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	scan.set_title("Scanning.. (Please Wait)");
	scan.signal_event().connect(sigc::mem_fun(*this, &SettingsDialog::_scan_plugin_key));

	scan.show();

	fx_factory->rescan_effects(_scan_callback, this);

	response_button->set_sensitive(true);
	scan.set_deletable(true);
	scan.set_title("Scanning.. Done");

	Gtk::TreeModel::Row row = *(scan_list_store->append());
	row[scan_model_columns.name] = "Done.";
	row = *(scan_list_store->append());
	row[scan_model_columns.name] = ("Found " + String::num(fx_factory->get_audio_effect_count()) + " effect(s).").utf8().get_data();

	scan.run();
	scan.hide();
	_save_plugins();
}
void SettingsDialog::_save_plugins() {
	String save_to = get_settings_path() + "/plugins.json";
	JSON::Node node = JSON::object();

	{ //plugins
		JSON::Node plugin_array = JSON::array();

		for (int i = 0; i < fx_factory->get_audio_effect_count(); i++) {

			JSON::Node plugin_node = JSON::object();
			const AudioEffectInfo *info = fx_factory->get_audio_effect(i);

			plugin_node.add("caption", info->caption.utf8().get_data());
			plugin_node.add("short_caption", info->short_caption.utf8().get_data());
			plugin_node.add("description", info->description.utf8().get_data());
			plugin_node.add("author", info->author.utf8().get_data());
			plugin_node.add("category", info->category.utf8().get_data());
			plugin_node.add("unique_id", info->unique_ID.utf8().get_data());
			plugin_node.add("icon_string", info->icon_string.utf8().get_data());
			plugin_node.add("version", info->version.utf8().get_data());
			plugin_node.add("synth", info->synth);
			plugin_node.add("has_ui", info->has_ui);
			plugin_node.add("provider_caption", info->provider_caption.utf8().get_data());
			plugin_node.add("provider_id", info->provider_id.utf8().get_data());
			plugin_node.add("path", info->path.utf8().get_data());

			plugin_array.add(plugin_node);
		}

		node.add("plugins", plugin_array);
	}

	save_json(save_to, node);
}

void SettingsDialog::_browse_plugin_path() {
	Gtk::FileChooserDialog dialog("Select a folder containing plugins",
			Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
	dialog.set_transient_for(*this);

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

	Gtk::TreeModel::iterator iter = plugin_tree_selection->get_selected();
	if (!iter)
		return;

	Gtk::TreeModel::Row row = *iter;
	int index = row[plugin_model_columns.index];

	String existing = AudioEffectProvider::get_scan_path(index);
	if (existing != String()) {
		dialog.set_filename(existing.utf8().get_data());
	}

	int result = dialog.run();
	if (result == Gtk::RESPONSE_OK) {
		String path;
		path.parse_utf8(dialog.get_filename().c_str());
		AudioEffectProvider::set_scan_path(index, path);
		row[plugin_model_columns.text] = dialog.get_filename();
		_save_settings();
	}
}
void SettingsDialog::_plugin_path_edited(const Glib::ustring &path, const Glib::ustring &text) {
	Gtk::TreeIter iter = plugin_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	String s;
	s.parse_utf8(text.c_str());
	AudioEffectProvider::set_scan_path((*iter)[plugin_model_columns.index], s);

	(*iter)[plugin_model_columns.text] = text;
	_save_settings();
}

void SettingsDialog::_color_selected(int p_index) {

	if (theme_color_list.get_selected() >= 0) {
		Gdk::RGBA color = theme->colors[theme_color_list.get_selected()];
		theme_color_change.set_rgba(color);
	}
}
void SettingsDialog::_choose_color() {
	if (theme_color_list.get_selected() >= 0) {
		theme->colors[theme_color_list.get_selected()] = theme_color_change.get_rgba();
		theme_color_list.queue_draw();
		update_colors.emit();
		_save_settings();
	}
}

void SettingsDialog::_font_chosen() {
	theme->font.parse_utf8(theme_font_button.get_font_name().c_str());
	update_colors.emit();
	_save_settings();
}

void SettingsDialog::_on_dark_theme_chosen() {
	theme->color_scheme = theme_force_dark.get_active() ? Theme::COLOR_SCHEME_DARK : Theme::COLOR_SCHEME_DEFAULT;
	_save_settings();
}

bool SettingsDialog::_signal_remap_key(GdkEventKey *p_key) {

	if (p_key->keyval >= GDK_KEY_Shift_L && p_key->keyval <= GDK_KEY_Hyper_R) {
		return false; //no modifiers welcome
	}
	Gtk::AccelKey accel(p_key->keyval, Gdk::ModifierType(p_key->state));
	key_remap_dialog.set_message(accel.get_abbrev());
	key_remap_key = p_key->keyval;
	key_remap_mod = p_key->state;

	return true;
}

void SettingsDialog::_shortcut_assign() {

	Gtk::TreeModel::iterator iter = shortcut_tree_selection->get_selected();
	if (!iter)
		return;

	Gtk::TreeModel::Row row = *iter;
	int index = row[shortcut_model_columns.index];

	key_remap_dialog.set_message(key_bindings->get_keybind_text(KeyBindings::KeyBind(index)).utf8().get_data());

	if (key_remap_dialog.run() == Gtk::RESPONSE_OK) {

		key_bindings->set_keybind(KeyBindings::KeyBind(index), key_remap_key, key_remap_mod);

		row[shortcut_model_columns.text] = key_bindings->get_keybind_text(KeyBindings::KeyBind(index)).utf8().get_data();
	}

	key_remap_dialog.hide();
	_save_settings();
}
void SettingsDialog::_shortcut_clear() {

	Gtk::TreeModel::iterator iter = shortcut_tree_selection->get_selected();
	if (!iter)
		return;

	Gtk::TreeModel::Row row = *iter;
	int index = row[shortcut_model_columns.index];

	key_bindings->clear_keybind(KeyBindings::KeyBind(index));

	row[shortcut_model_columns.text] = key_bindings->get_keybind_text(KeyBindings::KeyBind(index)).utf8().get_data();
	_save_settings();
}
void SettingsDialog::_shortcut_restore() {

	Gtk::TreeModel::iterator iter = shortcut_tree_selection->get_selected();
	if (!iter)
		return;

	Gtk::TreeModel::Row row = *iter;
	int index = row[shortcut_model_columns.index];

	key_bindings->reset_keybind(KeyBindings::KeyBind(index));

	row[shortcut_model_columns.text] = key_bindings->get_keybind_text(KeyBindings::KeyBind(index)).utf8().get_data();

	_save_settings();
}

void SettingsDialog::initialize_bindings() {

	for (int i = 0; i < KeyBindings::BIND_MAX; i++) {

		String label = key_bindings->get_keybind_name(KeyBindings::KeyBind(i));
		String text = key_bindings->get_keybind_text(KeyBindings::KeyBind(i));
		Gtk::TreeModel::Row row = *(shortcut_list_store->append());
		row[shortcut_model_columns.label] = label.utf8().get_data();
		row[shortcut_model_columns.text] = text.utf8().get_data();
		row[shortcut_model_columns.index] = i;
		shortcut_rows.push_back(row);

		if (i == 0) {
			shortcut_tree_selection->select(row);
		}
	}
}

void SettingsDialog::_save_settings() {
	String save_to = get_settings_path() + "/settings.json";
	JSON::Node node = JSON::object();

	{ //audio
		JSON::Node audio_node = JSON::object();
		std::string driver_id;
		if (SoundDriverManager::get_current_driver_index() >= 0) {
			SoundDriver *driver = SoundDriverManager::get_driver(SoundDriverManager::get_current_driver_index());
			if (driver) {
				driver_id = driver->get_id().utf8().get_data();
			}
		}

		audio_node.add("id", driver_id);
		audio_node.add("mixing_hz", SoundDriverManager::get_mix_frequency());
		audio_node.add("buffer_size", SoundDriverManager::get_buffer_size());
		audio_node.add("block_size", SoundDriverManager::get_step_buffer_size());

		node.add("audio", audio_node);
	}

	{ //plugins
		JSON::Node plugin_node = JSON::object();
		for (int i = 0; i < AudioEffectProvider::MAX_SCAN_PATHS; i++) {
			String path = AudioEffectProvider::get_scan_path(i).strip_edges();
			if (path != String()) {
				plugin_node.add(String::num(i).ascii().get_data(), path.utf8().get_data());
			}
		}
		node.add("plugins", plugin_node);
	}

	{ //theme
		JSON::Node theme_node = JSON::object();
		theme_node.add("font", theme->font.utf8().get_data());
		JSON::Node colors_node = JSON::object();
		for (int i = 0; i < Theme::COLOR_MAX; i++) {
			JSON::Node array = JSON::array();
			array.add(theme->colors[i].get_red());
			array.add(theme->colors[i].get_green());
			array.add(theme->colors[i].get_blue());
			colors_node.add(theme->color_names[i], array);
		}
		theme_node.add("colors", colors_node);
		theme_node.add("use_dark_theme", theme->color_scheme == Theme::COLOR_SCHEME_DARK);
		node.add("theme", theme_node);
	}

	{ //key bindings

		JSON::Node bindings = JSON::object();
		JSON::Node array = JSON::array();
		for (int i = 0; i < KeyBindings::BIND_MAX; i++) {
			JSON::Node bind = JSON::object();
			bind.add("name", key_bindings->get_keybind_name(KeyBindings::KeyBind(i)));
			bind.add("key", key_bindings->get_keybind_key(KeyBindings::KeyBind(i)));
			bind.add("mods", key_bindings->get_keybind_mod(KeyBindings::KeyBind(i)));
			array.add(bind);
		}

		bindings.add("keys", array);
		node.add("key_bindings", bindings);
	}

	{ //default commands
		JSON::Node def_commands = JSON::array();
		for (int i = 0; i < MAX_DEFAULT_COMMANDS; i++) {
			if (default_commands[i].name == "" && default_commands[i].command == 0) {
				continue;
			}
			JSON::Node command = JSON::object();
			command.add("index", i);
			command.add("identifier", default_commands[i].name.utf8().get_data());
			command.add("command", default_commands[i].command);
			def_commands.add(command);
		}

		node.add("default_commands", def_commands);
	}

	save_json(save_to, node);
}

SettingsDialog::DefaultCommand SettingsDialog::default_commands[MAX_DEFAULT_COMMANDS];

void SettingsDialog::_update_command_list() {
	command_list_store->clear();

	for (int i = 0; i < MAX_DEFAULT_COMMANDS; i++) {

		Gtk::TreeModel::iterator iter = command_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[model_columns.name] = default_commands[i].name.utf8().get_data();

		if (default_commands[i].command == 0) {
			row[command_editor_columns.command] = "<assign>";
		} else {
			char s[2] = { char('A' + (default_commands[i].command - 'a')), 0 };
			row[command_editor_columns.command] = s;
		}
		row[command_editor_columns.index] = i;
	}
}

void SettingsDialog::_command_name_changed(const Glib::ustring &path, const Glib::ustring &text) {

	Gtk::TreeIter iter = command_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	int index = (*iter)[command_editor_columns.index];
	String s;
	s.parse_utf8(text.c_str());
	(*iter)[command_editor_columns.name] = text;

	default_commands[index].name = s;
	_save_settings();
}

void SettingsDialog::_command_value_changed(const Glib::ustring &path, const Glib::ustring &value) {

	Gtk::TreeIter iter = command_list_store->get_iter(path);
	ERR_FAIL_COND(!iter);
	int index = (*iter)[command_editor_columns.index];
	char valc = value[0];
	if (valc == '<') {
		valc = 0; //unselected
		(*iter)[command_editor_columns.command] = "<assign>";
	} else {
		valc = 'a' + (valc - 'A'); //unselected
		(*iter)[command_editor_columns.command] = value;
	}

	default_commands[index].command = valc;
	_save_settings();
}

void SettingsDialog::set_default_command(int p_index, const String &p_name, char p_command) {
	ERR_FAIL_INDEX(p_index, MAX_DEFAULT_COMMANDS);
	default_commands[p_index].name = p_name;
	default_commands[p_index].command = p_command;
}

String SettingsDialog::get_default_command_name(int p_index) {
	ERR_FAIL_INDEX_V(p_index, MAX_DEFAULT_COMMANDS, String());
	return default_commands[p_index].name;
}
char SettingsDialog::get_default_command_command(int p_index) {
	ERR_FAIL_INDEX_V(p_index, MAX_DEFAULT_COMMANDS, 0);
	return default_commands[p_index].command;
}

void SettingsDialog::add_default_command(const String &p_name, char p_command) {

	bool exists = false;
	for (int i = 0; i < MAX_DEFAULT_COMMANDS; i++) {
		if (default_commands[i].name == p_name) {
			default_commands[i].command = p_command;
			exists = true;
			break;
		}
	}

	if (!exists) {
		for (int i = 0; i < MAX_DEFAULT_COMMANDS; i++) {
			if (default_commands[i].name == String()) {
				default_commands[i].name = p_name;
				default_commands[i].command = p_command;
				break;
			}
		}
	}

	if (singleton) {
		singleton->_update_command_list();
		singleton->_save_settings();
	}
}
SettingsDialog *SettingsDialog::singleton = NULL;
SettingsDialog::SettingsDialog(Theme *p_theme, KeyBindings *p_key_bindings, AudioEffectFactory *p_fx_factory) :
		MessageDialog("", false /* use_markup */, Gtk::MESSAGE_OTHER, Gtk::BUTTONS_CLOSE),
		key_remap_dialog("", false, Gtk::MESSAGE_OTHER, Gtk::BUTTONS_OK_CANCEL),
		theme_color_list(p_theme) {

	singleton = this;
	fx_factory = p_fx_factory;
	key_bindings = p_key_bindings;
	theme = p_theme;
	{

		driver_list_store = Gtk::ListStore::create(model_columns);
		driver_combo.set_model(driver_list_store);

		for (int i = 0; i < SoundDriverManager::get_driver_count(); i++) {
			SoundDriver *driver = SoundDriverManager::get_driver(i);

			Gtk::TreeModel::Row row = *(driver_list_store->append());
			row[model_columns.name] = driver->get_name().utf8().get_data();
			row[model_columns.index] = i;
			driver_rows.push_back(row);
		}

		driver_combo.pack_start(model_columns.name);

		int active_index = SoundDriverManager::get_current_driver_index();
		if (active_index >= 0) {
			driver_combo.set_active(driver_rows[active_index]);
		}
	}
	driver_combo.signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::_driver_changed));

	{

		frequency_list_store = Gtk::ListStore::create(model_columns);
		frequency_combo.set_model(frequency_list_store);

		for (int i = 0; i < SoundDriverManager::MIX_FREQ_MAX; i++) {
			String label = String::num(SoundDriverManager::get_mix_frequency_hz(SoundDriverManager::MixFrequency(i))) + " hz";
			Gtk::TreeModel::Row row = *(frequency_list_store->append());
			row[model_columns.name] = label.utf8().get_data();
			row[model_columns.index] = i;
			frequency_rows.push_back(row);
		}

		frequency_combo.pack_start(model_columns.name);

		int active_index = SoundDriverManager::get_mix_frequency();
		if (active_index >= 0) {
			frequency_combo.set_active(frequency_rows[active_index]);
		}
	}
	frequency_combo.signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::_driver_freq_changed));

	{

		buffer_list_store = Gtk::ListStore::create(model_columns);
		buffer_combo.set_model(buffer_list_store);

		for (int i = 0; i < SoundDriverManager::BUFFER_SIZE_MAX; i++) {
			String label = String::num(SoundDriverManager::get_buffer_size_frames(SoundDriverManager::BufferSize(i))) + " frames";
			Gtk::TreeModel::Row row = *(buffer_list_store->append());
			row[model_columns.name] = label.utf8().get_data();
			row[model_columns.index] = i;
			buffer_rows.push_back(row);
		}

		buffer_combo.pack_start(model_columns.name);

		int active_index = SoundDriverManager::get_buffer_size();
		if (active_index >= 0) {
			buffer_combo.set_active(buffer_rows[active_index]);
		}
	}
	buffer_combo.signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::_driver_buffer_changed));

	{

		step_list_store = Gtk::ListStore::create(model_columns);
		step_combo.set_model(step_list_store);

		for (int i = 0; i < SoundDriverManager::BUFFER_SIZE_MAX; i++) {
			String label = String::num(SoundDriverManager::get_buffer_size_frames(SoundDriverManager::BufferSize(i))) + " frames";
			Gtk::TreeModel::Row row = *(step_list_store->append());
			row[model_columns.name] = label.utf8().get_data();
			row[model_columns.index] = i;
			step_rows.push_back(row);
		}

		step_combo.pack_start(model_columns.name);

		int active_index = SoundDriverManager::get_step_buffer_size();
		if (active_index >= 0) {
			step_combo.set_active(step_rows[active_index]);
		}
	}

	step_combo.signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::_driver_step_changed));

	////// zoom

	get_vbox()->set_spacing(0);

	get_vbox()->pack_start(notebook, Gtk::PACK_EXPAND_WIDGET);
	notebook.append_page(main_vbox, "Audio Settings");

	main_vbox.set_spacing(4);
	main_vbox.set_margin_left(8);
	main_vbox.set_margin_right(8);
	main_vbox.set_margin_top(8);
	main_vbox.set_margin_bottom(8);

	main_vbox.pack_start(sound_settings_frame, Gtk::PACK_SHRINK);
	sound_settings_frame.set_label("Sound Device");
	sound_settings_frame.add(sound_settings_grid);

	driver_label.set_text("Sound Device: ");
	driver_label.set_hexpand(true);
	sound_settings_grid.attach(driver_label, 0, 0, 1, 1);
	driver_combo.set_hexpand(true);
	sound_settings_grid.attach(driver_combo, 1, 0, 1, 1);

	frequency_label.set_text("Mix Frequency: ");
	frequency_label.set_hexpand(true);
	sound_settings_grid.attach(frequency_label, 0, 1, 1, 1);
	frequency_combo.set_hexpand(true);
	sound_settings_grid.attach(frequency_combo, 1, 1, 1, 1);

	buffer_label.set_text("Buffer Size: ");
	buffer_label.set_hexpand(true);
	sound_settings_grid.attach(buffer_label, 0, 2, 1, 1);
	buffer_combo.set_hexpand(true);
	sound_settings_grid.attach(buffer_combo, 1, 2, 1, 1);

	step_label.set_text("Block Size: ");
	step_label.set_hexpand(true);
	sound_settings_grid.attach(step_label, 0, 3, 1, 1);
	step_combo.set_hexpand(true);
	sound_settings_grid.attach(step_combo, 1, 3, 1, 1);

	sound_settings_grid.set_margin_left(8);
	sound_settings_grid.set_margin_right(8);
	sound_settings_grid.set_margin_bottom(8);
	sound_settings_grid.set_margin_top(8);

	main_vbox.pack_start(plugin_path_frame, Gtk::PACK_EXPAND_WIDGET);
	plugin_path_frame.set_label("Plugin Paths");

	plugin_path_frame.add(plugin_path_vbox);
	plugin_path_vbox.pack_start(plugin_scroll, Gtk::PACK_EXPAND_WIDGET);
	plugin_scroll.add(plugin_tree);

	plugin_list_store = Gtk::ListStore::create(plugin_model_columns);
	plugin_tree_selection = plugin_tree.get_selection();

	plugin_tree.append_column("Index", plugin_model_columns.label);
	plugin_column.set_title("Path");
	plugin_column.pack_start(plugin_column_text, true);
	plugin_column_text.property_editable() = true;
	plugin_column_text.signal_edited().connect(sigc::mem_fun(*this, &SettingsDialog::_plugin_path_edited));
	plugin_column.add_attribute(plugin_column_text.property_text(), plugin_model_columns.text);
	plugin_tree.append_column(plugin_column);

	plugin_tree.set_model(plugin_list_store);
	plugin_tree.get_column(0)->set_expand(false);
	plugin_tree.get_column(1)->set_expand(true);

	for (int i = 0; i < AudioEffectProvider::MAX_SCAN_PATHS; i++) {

		Gtk::TreeModel::iterator iter = plugin_list_store->append();
		Gtk::TreeModel::Row row = *iter;

		row[plugin_model_columns.label] = String::num(i).utf8().get_data();
		row[plugin_model_columns.text] = AudioEffectProvider::get_scan_path(i).utf8().get_data();
		row[plugin_model_columns.index] = i;

		if (i == 0) {
			plugin_tree_selection->select(row);
		}
	}

	plugin_path_vbox.pack_start(plugin_hbox, Gtk::PACK_SHRINK);
	plugin_hbox.pack_start(plugin_browse_path, Gtk::PACK_EXPAND_WIDGET);
	plugin_browse_path.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_browse_plugin_path));
	plugin_browse_path.set_label("Browse..");
	plugin_hbox.pack_start(scan_plugins, Gtk::PACK_EXPAND_WIDGET);
	scan_plugins.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_scan_plugins));
	scan_plugins.set_label("Scan");

	//////////////////////

	notebook.append_page(theme_vbox, "Theme Settings");

	theme_vbox.set_spacing(4);
	theme_vbox.set_margin_left(8);
	theme_vbox.set_margin_right(8);
	theme_vbox.set_margin_top(8);
	theme_vbox.set_margin_bottom(8);

	theme_vbox.pack_start(theme_font_frame, Gtk::PACK_SHRINK);
	theme_font_frame.set_label("Font:");
	theme_font_frame.add(theme_font_grid);
	theme_font_label.set_text("Editor Font:");
	theme_font_label.set_hexpand(true);
	theme_font_grid.attach(theme_font_label, 0, 0, 1, 1);
	theme_font_grid.attach(theme_font_button, 1, 0, 1, 1);
	theme_font_button.set_hexpand(true);
	theme_font_grid.set_margin_left(8);
	theme_font_grid.set_margin_right(8);
	theme_font_grid.set_margin_top(8);
	theme_font_grid.set_margin_bottom(8);
	theme_font_button.set_font_name(theme->font.utf8().get_data());
	theme_font_button.signal_font_set().connect(sigc::mem_fun(*this, &SettingsDialog::_font_chosen));

	theme_vbox.pack_start(theme_colors_frame, Gtk::PACK_EXPAND_WIDGET);
	theme_colors_frame.set_label("Colors:");
	theme_colors_frame.add(theme_colors_grid);
	theme_colors_grid.attach(theme_color_list_scroll, 0, 0, 2, 1);
	theme_color_list_scroll.set_hexpand(true);
	theme_color_list_scroll.set_vexpand(true);
	theme_color_list_scroll.add(theme_color_list);
	theme_color_list.color_selected.connect(sigc::mem_fun(*this, &SettingsDialog::_color_selected));
	theme_color_list.set_hexpand(true);
	theme_color_list.set_vexpand(true);
	theme_colors_grid.set_margin_left(8);
	theme_colors_grid.set_margin_right(8);
	theme_colors_grid.set_margin_top(8);
	theme_colors_grid.set_margin_bottom(8);
	theme_color_label.set_text("Change Color: ");
	theme_colors_grid.attach(theme_color_label, 0, 1, 1, 1);
	theme_colors_grid.attach(theme_color_change, 1, 1, 1, 1);
	theme_colors_grid.set_row_spacing(8);
	//theme_color_change.set_label("Change Color");
	theme_color_change.set_use_alpha(false);
	theme_color_change.property_show_editor() = true;
	theme_color_change.set_rgba(theme->colors[0]);

	theme_color_change.signal_color_set().connect(sigc::mem_fun(*this, &SettingsDialog::_choose_color));

	theme_vbox.pack_start(theme_settings_frame, Gtk::PACK_SHRINK);
	theme_settings_frame.set_label("Settings:");
	theme_settings_frame.add(theme_settings_grid);
	theme_force_dark.set_label("Force Dark Theme (needs restart)");
	theme_force_dark.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_on_dark_theme_chosen));

	theme_settings_grid.attach(theme_force_dark, 0, 0, 1, 1);
	theme_force_dark.set_active(theme->color_scheme == Theme::COLOR_SCHEME_DARK);
	theme_settings_grid.set_margin_left(8);
	theme_settings_grid.set_margin_right(8);
	theme_settings_grid.set_margin_top(8);
	theme_settings_grid.set_margin_bottom(8);

	/////////////////////

	notebook.append_page(shortcut_vbox, "Key Bindings");

	shortcut_vbox.set_spacing(4);
	shortcut_vbox.set_margin_left(8);
	shortcut_vbox.set_margin_right(8);
	shortcut_vbox.set_margin_top(8);
	shortcut_vbox.set_margin_bottom(8);

	shortcut_vbox.pack_start(shortcut_frame, Gtk::PACK_EXPAND_WIDGET);
	shortcut_frame.add(shortcut_grid);
	shortcut_grid.set_margin_left(8);
	shortcut_grid.set_margin_right(8);
	shortcut_grid.set_margin_top(8);
	shortcut_grid.set_margin_bottom(8);

	shortcut_list_store = Gtk::ListStore::create(shortcut_model_columns);
	shortcut_tree_selection = shortcut_tree.get_selection();

	shortcut_tree.append_column("Name", shortcut_model_columns.label);
	shortcut_tree.append_column("Shortcut", shortcut_model_columns.text);
	shortcut_tree.set_model(shortcut_list_store);
	shortcut_tree.get_column(0)->set_expand(true);
	shortcut_tree.get_column(1)->set_expand(true);

	shortcut_grid.attach(shortcut_scroll, 0, 0, 3, 1);
	shortcut_scroll.add(shortcut_tree);
	shortcut_scroll.set_hexpand(true);
	shortcut_scroll.set_vexpand(true);

	shortcut_assign_button.set_label("Assign");
	shortcut_grid.attach(shortcut_assign_button, 0, 1, 1, 1);
	shortcut_assign_button.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_shortcut_assign));
	shortcut_clear_button.set_label("Clear");
	shortcut_grid.attach(shortcut_clear_button, 1, 1, 1, 1);
	shortcut_clear_button.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_shortcut_clear));
	shortcut_reset_button.set_label("Reset");
	shortcut_grid.attach(shortcut_reset_button, 2, 1, 1, 1);
	shortcut_reset_button.signal_clicked().connect(sigc::mem_fun(*this, &SettingsDialog::_shortcut_restore));

	key_remap_dialog.signal_key_press_event().connect(sigc::mem_fun(*this, &SettingsDialog::_signal_remap_key));
	key_remap_dialog.set_transient_for(*this);
	key_remap_dialog.set_title("Press a Key:");
	key_remap_dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

	//////////////////

	scan_list_store = Gtk::ListStore::create(scan_model_columns);
	scan_tree_selection = scan_tree.get_selection();
	scan_tree.set_model(scan_list_store);
	scan_tree.append_column("Plugins Found:", scan_model_columns.name);
	scan_tree.get_column(0)->set_expand(false);
	scan_scroll.add(scan_tree);

	//////////////////

	notebook.append_page(command_frame, "Default Commands");
	command_frame.set_label("Commands:");
	command_frame.add(command_tree_scroll);
	command_frame.set_hexpand(true);
	command_frame.set_vexpand(true);
	command_tree_scroll.set_hexpand(true);
	command_tree_scroll.set_vexpand(true);
	command_tree_scroll.add(command_tree);

	command_list_store = Gtk::ListStore::create(command_editor_columns);
	command_tree_selection = command_tree.get_selection();
	command_tree.set_model(command_list_store);

	command_tree.set_model(command_list_store);
	command_tree.set_hexpand(true);
	command_tree.set_vexpand(true);

	command_column1.set_title("Name");
	command_column1.pack_start(cell_render_text, true);
	cell_render_text.signal_edited().connect(sigc::mem_fun(*this, &SettingsDialog::_command_name_changed));
	command_column1.add_attribute(cell_render_text.property_text(), command_editor_columns.name);
	cell_render_text.property_editable() = true;

	command_tree.append_column(command_column1);
	command_tree.get_column(0)->set_expand(true);

	command_commands_list_store = Gtk::ListStore::create(command_editor_columns.command_model_columns);
	{
		{
			Gtk::TreeModel::iterator iter = command_commands_list_store->append();
			Gtk::TreeModel::Row row = *iter;
			row[command_editor_columns.command_model_columns.name] = "<unassigned>";
			row[command_editor_columns.command_model_columns.index] = 0;
		}
		for (int i = 'a'; i <= 'z'; i++) {

			Gtk::TreeModel::iterator iter = command_commands_list_store->append();
			Gtk::TreeModel::Row row = *iter;

			const char s[2] = { char('A' + (i - 'a')), 0 };
			row[command_editor_columns.command_model_columns.name] = s;
			row[command_editor_columns.command_model_columns.index] = i;
		}
	}

	command_column2.set_title("Command");
	command_column2.pack_start(cell_render_command, false);
	command_column2.add_attribute(cell_render_command.property_text(), command_editor_columns.command);
	cell_render_command.signal_edited().connect(sigc::mem_fun(*this, &SettingsDialog::_command_value_changed));

	cell_render_command.property_model() = command_commands_list_store;
	cell_render_command.property_text_column() = 0;
	cell_render_command.property_editable() = true;
	cell_render_command.property_has_entry() = false;
	cell_render_command.set_visible(true);
	command_tree.append_column(command_column2);
	command_tree.get_column(1)->set_expand(false);
	//tree.set_can_focus(false);
	//tree_selection->set_mode(Gtk::SELECTION_NONE);

	bool has_default_commands = false;
	for (int i = 0; i < MAX_DEFAULT_COMMANDS; i++) {
		if (default_commands[i].name != "" || default_commands[i].command) {
			has_default_commands = true;
		}
	}

	if (!has_default_commands) {
		default_commands[0].name = "bend_portamento";
		default_commands[0].command = 'g';
		default_commands[1].name = "bend_vibrato";
		default_commands[1].command = 'h';
		default_commands[2].name = "bend_slide_up";
		default_commands[2].command = 'f';
		default_commands[3].name = "bend_slide_down";
		default_commands[3].command = 'e';
		default_commands[4].name = "cc_Pan";
		default_commands[4].command = 'w';
		default_commands[5].name = "cc_Expression";
		default_commands[5].command = 'm';
		default_commands[6].name = "cc_Breath";
		default_commands[6].command = 'b';
		default_commands[7].name = "cc_Modulation";
		default_commands[7].command = 'u';
		default_commands[8].name = "cc_FilterCutoff";
		default_commands[8].command = 'z';
	}

	_update_command_list();
	///////////////////////
	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();

	set_default_size(width / 4, height / 2);

	show_all_children();
	get_vbox()->get_children()[0]->hide();

	set_title("Settings");
}

String SettingsDialog::get_settings_path() {

	bool created_path = false;

	String path;
	String dir = "ZyTrax";
#ifdef WINDOWS_ENABLED
	path = _wgetenv(L"APPDATA");
#endif

#ifdef FREEDESKTOP_ENABLED
	if (getenv("XDG_CONFIG_HOME")) {
		path.parse_utf8(getenv("XDG_CONFIG_HOME"));
	} else {
		path.parse_utf8(getenv("HOME"));
		dir = "." + dir;
	}
#endif

#ifdef OSX_ENABLED
	path.parse_utf8(getenv("HOME"));
	if (path[path.length() - 1] != '/') {
		path += "/";
	}
	path += "Library/Application Support";
#endif

	if (path[path.length() - 1] != '/') {
		path += "/";
	}

	path += dir;

	if (!created_path) {
#ifdef WINDOWS_ENABLED
		_wmkdir(path.c_str());
#else
		mkdir(path.utf8().get_data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
		created_path = true;
	}
	return path;
}
