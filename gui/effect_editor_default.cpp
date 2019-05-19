#include "effect_editor_default.h"

void EffectEditorDefault::_combo_changed(int p_idx) {
	ERR_FAIL_COND(!combos.has(p_idx));
	Gtk::TreeModel::iterator iter = combos[p_idx].combo->get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[model_columns.index];

			effect->get_control_port(p_idx)->set(float(id));
		}
	}
}
void EffectEditorDefault::_scale_changed(int p_idx) {
	ERR_FAIL_COND(!scales.has(p_idx));

	effect->get_control_port(p_idx)->set(scales[p_idx]->get_adjustment()->get_value());
}
void EffectEditorDefault::_toggle_clicked(int p_idx) {

	ERR_FAIL_COND(!buttons.has(p_idx));

	effect->get_control_port(p_idx)->set(buttons[p_idx]->get_active() ? 1.0 : 0.0);
}

EffectEditorDefault::EffectEditorDefault(AudioEffect *p_effect) {

	effect = p_effect;

	bool natural_height = p_effect->get_control_port_count() <= 10;

	set_propagate_natural_width(true);
	set_propagate_natural_height(natural_height);
	set_policy(Gtk::POLICY_NEVER, natural_height ? Gtk::POLICY_NEVER : Gtk::POLICY_ALWAYS);

	effect_grid.set_margin_left(8);
	effect_grid.set_margin_right(8);
	effect_grid.set_margin_top(8);
	effect_grid.set_margin_bottom(8);
	effect_grid.set_row_spacing(2);
	effect_grid.set_column_spacing(8);

	effect_grid.set_hexpand(true);
	effect_grid.set_vexpand(true);

	Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
	int width = screen->get_width();
	int height = screen->get_height();

	set_size_request(width / 4, natural_height ? 1 : height / 3);

	add(effect_grid);

	for (int i = 0; i < p_effect->get_control_port_count(); i++) {
		ControlPort *cp = p_effect->get_control_port(i);
		if (!cp->is_visible()) {
			continue;
		}
		switch (cp->get_hint()) {
			case ControlPort::HINT_RANGE:
			case ControlPort::HINT_RANGE_NORMALIZED: {
				Gtk::Label *name_label = new Gtk::Label;
				name_label->set_text(cp->get_name().utf8().get_data());
				effect_grid.attach(*name_label, 0, i, 1, 1);

				Gtk::HScale *scale = new Gtk::HScale;
				scale->set_adjustment(Gtk::Adjustment::create(cp->get(), cp->get_min(), cp->get_max(), cp->get_step()));

				//geez GTK, you could guess this from the step..
				if (cp->get_step() >= 0.99) {
					scale->set_digits(0);
				} else if (cp->get_step() >= 0.099) {
					scale->set_digits(1);
				} else if (cp->get_step() >= 0.0099) {
					scale->set_digits(2);
				} else {
					scale->set_digits(3);
				}

				scale->set_hexpand(true);

				effect_grid.attach(*scale, 1, i, 1, 1);

				if (cp->get_hint() == ControlPort::HINT_RANGE_NORMALIZED) {
					Gtk::Label *value_label = new Gtk::Label;
					value_label->set_text(cp->get_value_as_text().utf8().get_data());
					effect_grid.attach(*value_label, 2, i, 1, 1);
					widgets.push_back(value_label);
					labels[i] = value_label;
					scale->set_draw_value(false);
				}

				scales[i] = scale;

				widgets.push_back(name_label);

				widgets.push_back(scale);

				scale->get_adjustment()->signal_value_changed().connect(sigc::bind<int>(sigc::mem_fun(*this, &EffectEditorDefault::_scale_changed), i));

			} break;
			case ControlPort::HINT_ENUM: {

				Gtk::Label *name_label = new Gtk::Label;
				name_label->set_text(cp->get_name().utf8().get_data());
				effect_grid.attach(*name_label, 0, i, 1, 1);

				combos[i] = Combo();
				Combo &combo = combos[i];

				combo.list_store = Gtk::ListStore::create(model_columns);

				combo.list_store->clear();
				float eff_current = cp->get();

				for (int i = 0; i <= cp->get_max(); i++) {

					Gtk::TreeModel::iterator iter = combo.list_store->append();
					Gtk::TreeModel::Row row = *iter;

					cp->set(i);
					row[model_columns.name] = cp->get_value_as_text().utf8().get_data();
					row[model_columns.index] = i;
				}

				combo.combo = new Gtk::ComboBox;
				combo.combo->set_model(combo.list_store);
				combo.combo->pack_start(model_columns.name);
				combo.combo->set_active((int)eff_current);
				combo.combo->signal_changed().connect(sigc::bind<int>(sigc::mem_fun(*this, &EffectEditorDefault::_combo_changed), i));
				cp->set(eff_current);

				effect_grid.attach(*combo.combo, 1, i, 1, 1);
				combo.combo->set_hexpand(true);

				widgets.push_back(name_label);
				widgets.push_back(combo.combo);

			} break;
			case ControlPort::HINT_TOGGLE: {

				Gtk::CheckButton *check = new Gtk::CheckButton;
				check->set_label(cp->get_name().utf8().get_data());
				check->set_active(cp->get() > 0.5);
				effect_grid.attach(*check, 0, i, 2, 1);
				buttons[i] = check;
				widgets.push_back(check);
				check->signal_clicked().connect(sigc::bind<int>(sigc::mem_fun(*this, &EffectEditorDefault::_toggle_clicked), i));
			} break;
		}
	}

	show_all_children();
}

EffectEditorDefault::~EffectEditorDefault() {
	for (int i = 0; i < widgets.size(); i++) {
		delete widgets[i];
	}
}

Gtk::Widget *create_default_editor_func(AudioEffect *p_effect, EffectEditor *p_editor) {

	return new EffectEditorDefault(p_effect);
}
