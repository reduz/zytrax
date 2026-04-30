#ifndef EFFECT_EDITOR_CHIONIC_H
#define EFFECT_EDITOR_CHIONIC_H

#include "effects/chionic/synth_chionic.h"
#include "gui/effect_editor_midi.h"
#include "gui/oscillator_editor.h"
#include "gui/filter_array_editor.h"

#include <gtkmm.h>


class EditorChionicEnvelope : public Gtk::Widget {
protected:
	SynthChionic::EnvelopeLFOType envelope_type = SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE;
	SynthChionic::Envelope * envelope = nullptr;

	enum {
		MIN_DRAG_DISTANCE=8 // min drag distance in pixels
	};

	float zoom = 0.01;

	int font_h_spacing = 1;

	int size_w = 0;
	int size_h = 0;


	struct Grab {
		bool moving_point = false;
		int node = -1;
		float x = 0,y = 0;
	} grab;


	bool recalc_pending = true;

	void draw_envelope_pos(int p_x,const Cairo::RefPtr<Cairo::Context> &cr);
	void paint_envelope_points(int w, int h, int font_h, int font_ascent, int char_w, const Cairo::RefPtr<Cairo::Context> &cr);

	bool adjust_zoom_to_window();
	void move_mouse_to_selected_node();

	void on_size_allocate(Gtk::Allocation &allocation) override;
	void on_realize() override;
	void on_unrealize() override;
	bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

	void mouse_button(GdkEventButton *event, bool p_press);

	bool on_button_press_event(GdkEventButton *event);
	bool on_button_release_event(GdkEventButton *event);
	bool on_motion_notify_event(GdkEventMotion *motion_event);


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
	sigc::signal0<void> envelope_changed;

	EditorChionicEnvelope(SynthChionic::EnvelopeLFOType p_envelope_type,SynthChionic::Envelope *p_envelope, Theme *p_theme);
	~EditorChionicEnvelope();
};

class EditorChionicEnvelopeLFO : public Gtk::Grid {

	SynthChionic::EnvelopeLFOType envelope_type = SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE;
	int generator = 0;
	SynthChionic * chionic = nullptr;

	class ComboModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ComboModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
	};
	ComboModelColumns combo_model_columns;

	Gtk::Label envelope_label;

	SynthChionic::Envelope envelope;
	SynthChionic::LFO lfo;

	EditorChionicEnvelope envelope_editor;
	int column = 0;
	Gtk::CheckButton sustain_loop_enable;
	Gtk::Label sustain_loop_begin_label;
	Gtk::SpinButton sustain_loop_begin;
	Gtk::Label sustain_loop_end_label;
	Gtk::SpinButton sustain_loop_end;

	Gtk::CheckButton loop_enable;
	Gtk::Label loop_begin_label;
	Gtk::SpinButton loop_begin;
	Gtk::Label loop_end_label;
	Gtk::SpinButton loop_end;

	Gtk::Label lfo_label;

	Gtk::Label lfo_osc_label;
	Gtk::ComboBox lfo_osc_type;
	Glib::RefPtr<Gtk::ListStore> lfo_osc_list_store;

	Gtk::Label lfo_delay_label;
	Gtk::SpinButton lfo_delay;
	Gtk::Label lfo_depth_label;
	Gtk::HScale lfo_depth;
	Gtk::Label lfo_rate_label;
	Gtk::SpinButton lfo_rate;

	bool updating = false;
	void _envelope_points_changed();
	void _envelope_changed();
	void _lfo_changed();

	bool first_setup = true;
public:
	int get_column() const { return column; }
	EditorChionicEnvelopeLFO(SynthChionic * p_chionic,int p_generator,SynthChionic::EnvelopeLFOType p_envelope_type);
};

class EditorChionicGenerator : public Gtk::Grid {

	SynthChionic * chionic = nullptr;
	int generator = 0;

	class ComboModelColumns : public Gtk::TreeModelColumnRecord {
	public:
		ComboModelColumns() {
			add(name);
			add(index);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
	};
	ComboModelColumns combo_model_columns;

	Gtk::Grid oscillator_grid;

	Gtk::Label type_label;
	Gtk::ComboBox type_combo;
	Glib::RefPtr<Gtk::ListStore> type_combo_list_store;

	Gtk::Label modulation_label;
	Gtk::ComboBox modulation_source;
	Gtk::HBox modulation_hbox;
	Glib::RefPtr<Gtk::ListStore> modulation_source_list_store;
	Gtk::ComboBox modulation_combo;
	Glib::RefPtr<Gtk::ListStore> modulation_combo_list_store;

	Gtk::Notebook notebook;

	Gtk::Label oscillator_type_label;
	Gtk::ComboBox oscillator_type_combo;
	Glib::RefPtr<Gtk::ListStore> oscillator_type_combo_list_store;

	OscillatorEditor oscillator_view;

	Gtk::Label oscillator_shape_label;
	Gtk::HScale oscillator_shape;


	// Volume
	Gtk::Label volume_label;
	Gtk::Label volume_db_label;
	Gtk::HScale volume_db;

	Gtk::Label volume_randomness_label;
	Gtk::HScale volume_randomness;

	Gtk::Label volume_punch_label;

	Gtk::Label volume_punch_amount_db_label;
	Gtk::HScale volume_punch_amount_db;

	Gtk::Label volume_punch_time_label;
	Gtk::SpinButton volume_punch_time;


	// Pan
	Gtk::Label pan_main_label;
	Gtk::Label pan_label;
	Gtk::HScale pan;

	Gtk::Label pan_randomness_label;
	Gtk::HScale pan_randomness;
	// Shape

	Gtk::Label shape_label;
	Gtk::Label shape_randomness_label;
	Gtk::HScale shape_randomness;
	Gtk::Label shape_velocity_ratio_label;
	Gtk::HScale shape_velocity_ratio;

	// Filter
	Gtk::Label filter_label;
	Gtk::Label filter_randomness_label;
	Gtk::HScale filter_randomness;
	Gtk::Label filter_velocity_ratio_label;
	Gtk::HScale filter_velocity_ratio;
	Gtk::Label filter_stages_label;
	Gtk::SpinButton filter_stages;

	// Pitch
	Gtk::Label pitch_label;
	Gtk::Label fine_tune_label;
	Gtk::HScale fine_tune;

	Gtk::Label coarse_tune_label;
	Gtk::SpinButton coarse_tune;

	EditorChionicEnvelopeLFO *env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_MAX];

	FilterArrayEditor filter_array_editor;

	void _generator_main_changed();
	void _oscillator_changed();
	void _parameters_changed();
	void _filter_changed();

	bool first_setup = true;

	bool updating = false;
public:
	EditorChionicGenerator(SynthChionic *p_chionic, int p_generator);
	~EditorChionicGenerator();
};

class EffectEditorChionic : public Gtk::VBox {


	EffectEditorMIDI effect_editor_midi;

	EditorChionicGenerator *generator_editor[SynthChionic::MAX_GENERATORS] = {} ;
	SynthChionic *synth_chionic = nullptr;

public:
	EffectEditorChionic(SynthChionic *p_chionic, EffectEditor *p_editor);
	~EffectEditorChionic();
};

Gtk::Widget *create_chionic_editor(AudioEffect *p_vst, EffectEditor *p_editor);

#endif // EFFECT_EDITOR_CHIONIC_H
