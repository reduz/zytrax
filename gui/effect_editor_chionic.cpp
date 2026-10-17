#include "effect_editor_chionic.h"


#include "gui/interface.h"


static const float MIN_ZOOM=0.0025;
static const float ZOOM_ADJUST_MARGIN_MIN=30.0; //percent of screen
static const float ZOOM_ADJUST_MARGIN_MAX=5.0; //percent of screen


void EditorChionicEnvelope::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
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

int EditorChionicEnvelope::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}


void EditorChionicEnvelope::mouse_button(GdkEventButton *event, bool p_press) {


	int cur_height=size_h-font_h_spacing;
	float env_normalize=-SynthChionic::envelope_type_get_min(envelope_type);
	float env_total=SynthChionic::envelope_type_get_max(envelope_type)-SynthChionic::envelope_type_get_min(envelope_type);

	if (!p_press) { //release


		if (grab.moving_point && event->button == 1) {

			grab.moving_point=false;
			queue_draw();
			envelope_changed.emit();;
		}

	} else {

		float gx=event->x,gy=event->y;

		// check if we grabbed anything.. distance in pixels
		int closest_node=-1;
		float closest_node_dist=1000;
		for (int i=0;i<envelope->get_point_count();i++) {


			float node_x,node_y;

			node_x=(float)envelope->get_point(i).offset;
			node_y=(float)envelope->get_point(i).value;

			node_x/=zoom;
			node_y+=env_normalize;
			node_y=node_y*(float)cur_height/env_total;
			node_y=cur_height-node_y;

			float distance=sqrt(pow(node_x-gx,2)+pow(node_y-gy,2));

			if ( (distance<closest_node_dist) && (distance<(float)MIN_DRAG_DISTANCE)) {

				closest_node_dist=distance;
				closest_node=i;
			}

		}

		switch (event->button) {

			case 1: {

				// check the fish
				if (closest_node!=-1) {

					//convert to local coords

					grab.moving_point=true;
					grab.node=closest_node;
					grab.x=envelope->get_point(grab.node).offset;
					grab.y=envelope->get_point(grab.node).value;

					move_mouse_to_selected_node();
				} else { //couldnt find close node, adding the point!


					gx*=zoom; // to env coordinates
					gy=(cur_height-gy)*env_total/(float)cur_height;
					gy-=env_normalize;



					gy = CLAMP(gy,SynthChionic::envelope_type_get_min(envelope_type),SynthChionic::envelope_type_get_max(envelope_type));
					int which=envelope->add_position(gx,gy,true);

					if (which>=0) {
						grab.moving_point=true;

						grab.node=which;
						grab.x=gx;
						grab.y=gy;
						move_mouse_to_selected_node();
						queue_draw();
						envelope_changed.emit();;

					}
				}

			} break;
			case 3: {

				if (closest_node!=-1) {


					envelope->del_position(closest_node);

					queue_draw();
					envelope_changed.emit();
					adjust_zoom_to_window();
				}
			} break;
			default: {}
		}



	}
}

bool EditorChionicEnvelope::on_motion_notify_event(GdkEventMotion *motion_event) {

	if (!grab.moving_point)
		return false;


	int cur_height=size_h-font_h_spacing;
	float env_normalize=-SynthChionic::envelope_type_get_min(envelope_type);
	float env_total=SynthChionic::envelope_type_get_max(envelope_type)-SynthChionic::envelope_type_get_min(envelope_type);

	float gx=motion_event->x,gy=motion_event->y;
	gx*=zoom;

	gy=float(cur_height-gy)*env_total/float(cur_height);
	gy-=env_normalize;

	if ( (gx!=envelope->get_point(grab.node).offset) || (gy!=envelope->get_point(grab.node).value) ) {

		//avoid pointless overdraw

		gy = CLAMP(gy,SynthChionic::envelope_type_get_min(envelope_type),SynthChionic::envelope_type_get_max(envelope_type));
		envelope->set_position(grab.node,gx,gy);

		if (adjust_zoom_to_window()) {
			move_mouse_to_selected_node();
		}

		queue_draw();
		envelope_changed.emit();

	}
	return false;
}

bool EditorChionicEnvelope::adjust_zoom_to_window() {

	if (envelope->get_point_count()==1) {
		zoom = MIN_ZOOM;
		return false;
	}

	float cur_width=size_w;
	float env_len=envelope->get_point(envelope->get_point_count()-1).offset;

	float adjust_margin_min=ZOOM_ADJUST_MARGIN_MIN*cur_width/100.0;
	float adjust_margin_max=ZOOM_ADJUST_MARGIN_MAX*cur_width/100.0;

	bool changed=false;

	float old_zoom=zoom;

	if ( (env_len/zoom) < (cur_width-adjust_margin_min) ) {

		zoom=env_len/(cur_width-adjust_margin_max);
	}

	if ( (env_len/zoom) > (cur_width-adjust_margin_max) ) {

		zoom=env_len/(cur_width-adjust_margin_min);
	}


	if (zoom<MIN_ZOOM) {
		zoom=MIN_ZOOM;
	}

	if (zoom!=old_zoom)
		changed=true;

	return changed;

}

void EditorChionicEnvelope::move_mouse_to_selected_node() {

//	int cur_width=w;
	int cur_height=size_h-font_h_spacing;
//	int real_height=size_h;

	if (cur_height<=0)
		return;

	float env_normalize=-SynthChionic::envelope_type_get_min(envelope_type);
	float env_total=SynthChionic::envelope_type_get_max(envelope_type)-SynthChionic::envelope_type_get_min(envelope_type);
	if (env_total==0)
		return; //canut be draw, broken envelope
	int center=int(env_normalize*(float)cur_height/env_total);
	center=cur_height-center;


	float node_x=(float)envelope->get_point(grab.node).offset;
	float node_y=(float)envelope->get_point(grab.node).value;

	node_x/=zoom;
	node_y+=env_normalize;
	node_y=node_y*(float)cur_height/env_total;
	node_y=cur_height-node_y;


	// Get the widget's window
	auto window = get_window();
	if (!window) return;

	// Translate the point to screen coordinates
	int screen_x, screen_y;
	window->get_origin(screen_x, screen_y);
	screen_x += node_x;
	screen_y += node_y;

	// Get the default display and pointer
	auto display = Gdk::Display::get_default();
	if (!display) return;

	auto seat = display->get_default_seat();
	if (!seat) return;

	auto pointer = seat->get_pointer();
	if (!pointer) return;

	// Warp the pointer to the desired screen coordinates
	pointer->warp(display->get_default_screen(), screen_x, screen_y);

}


void EditorChionicEnvelope::paint_envelope_points(int w, int h, int font_h, int font_ascent, int char_w, const Cairo::RefPtr<Cairo::Context> &cr) {

	int cur_height=h-font_h_spacing;
	int cur_width=w;
	int real_height=h;

	if (cur_height<=0)
		return;

	float env_normalize=-SynthChionic::envelope_type_get_min(envelope_type);
	float env_total=SynthChionic::envelope_type_get_max(envelope_type)-SynthChionic::envelope_type_get_min(envelope_type);

	Gdk::RGBA loop_color = theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT];
	if (envelope->has_loop()) {

		float node_begin,node_end;

		node_begin=envelope->get_point( envelope->get_loop_begin() ).offset/zoom;
		node_end=envelope->get_point( envelope->get_loop_end() ).offset/zoom;

		_draw_fill_rect(cr,(int)node_begin,0,(int)1,cur_height,loop_color);
		_draw_fill_rect(cr,(int)node_end,0,(int)1,cur_height,loop_color);

	}

	if (envelope->has_sustain_loop()) {

		float node_begin,node_end;

		node_begin=envelope->get_point( envelope->get_sustain_loop_begin() ).offset/zoom;
		node_end=envelope->get_point( envelope->get_sustain_loop_end() ).offset/zoom;

		_draw_fill_rect(cr,(int)node_begin,0,(int)1,cur_height,loop_color);
		_draw_fill_rect(cr,(int)node_end,0,(int)1,cur_height,loop_color);

	}


	_draw_fill_rect(cr,0,cur_height,cur_width,real_height-cur_height,theme->colors[Theme::COLOR_BACKGROUND]);

	for (int i=0;i<envelope->get_point_count();i++) {


		float node_x,node_y;

		node_x=(float)envelope->get_point(i).offset/zoom;

		node_y=(float)envelope->get_point(i).value;
		node_y+=env_normalize;
		node_y=node_y*(float)cur_height/env_total;
		node_y=cur_height-node_y;

		_draw_rect(cr,(int)node_x-2,(int)node_y-2,6,6,theme->colors[Theme::COLOR_FOCUS]);

	}


	/* This code calculates the increment for displaying the numbers, so they dont overlap */

	float avg_string_width=char_w*5;

	float increment=1.0;
	int step=0;

	while ((increment/zoom)<avg_string_width) {


		if (step % 2)
			increment*=2;
		else
			increment*=5;
		step++;

	}



	for (float i=0.0f;i<(cur_width*zoom);i+=increment) {

		int real_screen_offset=int((float)i/zoom);

		String num=String::num(i)+"s";
		int num_w=num.length()*char_w;

		if (i>0.0f) {
			_draw_text(cr,real_screen_offset-num_w/2,cur_height+font_ascent,num,loop_color,false);
		}
	}


	String str;

	if (grab.moving_point) {

		str="Moving #" + String::num(grab.node+1) +"/" +String::num(envelope->get_point_count()) +", pos: " + String::num(envelope->get_point(grab.node).offset,3) +"s,"+ SynthChionic::envelope_type_get_legend(envelope_type,envelope->get_point(grab.node).value);
	} else {

		str=String::num(envelope->get_point_count())+" Nodes.";

	}

	_draw_text(cr,2,cur_height+font_ascent+font_h,str,loop_color,false);

}

void EditorChionicEnvelope::draw_envelope_pos(int p_x,const Cairo::RefPtr<Cairo::Context> &cr) {

//	int cur_width=w;
	int cur_height=size_h-font_h_spacing-1;
//	int real_height=size_h;

	if (cur_height<=0)
		return;

	float env_normalize=-SynthChionic::envelope_type_get_min(envelope_type);
	float env_total=SynthChionic::envelope_type_get_max(envelope_type)-SynthChionic::envelope_type_get_min(envelope_type);
	if (env_total==0)
		return; //canut be draw, broken envelope
	int center=int(env_normalize*(float)cur_height/env_total);
	center=cur_height-center;

	_draw_fill_rect(cr,p_x,0,1,cur_height,theme->colors[Theme::COLOR_BACKGROUND]);

	int real_x_offset=int(p_x*zoom);
	int prev_real_x_offset=int((p_x-1)*zoom);

	int line_size=1;


	if (real_x_offset!=prev_real_x_offset) {

		if ((real_x_offset % 5)==0) {

			line_size=3;
		}

		if ((real_x_offset % 10)==0) {

			line_size=5;
		}

		if ((real_x_offset % 50)==0) {

			line_size=7;
		}

		if ((real_x_offset % 100)==0) {

			line_size=9;
		}

		if ((real_x_offset % 500)==0) {

			line_size=11;
		}
	}


	int end=center+(line_size/2);

	if (end>=(cur_height))
		end=cur_height;

	int begin=center-(line_size/2);
	_draw_fill_rect(cr,p_x,begin,1,end-begin+1,theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE_NOFIT]);

	float offset = (float)p_x*zoom;
	float val=envelope->get_height_at_offset(offset);

	if (offset>envelope->get_point(envelope->get_point_count()-1).offset)
		return; //dont draw what it's pointless to draw


	val+=env_normalize;
	val=val*(float)cur_height/env_total;
	val=cur_height-val;

	float prev_offset = (float)(p_x-1)*zoom;
	float prev_val=envelope->get_height_at_offset(prev_offset);
	prev_val+=env_normalize;
	prev_val=prev_val*(float)cur_height/env_total;
	prev_val=cur_height-prev_val;

	Gdk::RGBA line_c=theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE];
	// "El-Cheapo" Antialiasing..
	Gdk::RGBA bg=theme->colors[Theme::COLOR_BACKGROUND];
	line_c.set_red((line_c.get_red() + bg.get_red())/2);
	line_c.set_green((line_c.get_green() + bg.get_green())/2);
	line_c.set_blue((line_c.get_blue() + bg.get_blue())/2);

	int from=(int)prev_val;
	int to=(int)val;
	if (from>to) {

		int aux=from;
		from=to;
		to=aux;
	}
	to+=1;
	_draw_fill_rect(cr,p_x,from-1,1,(to-from)+2,line_c); //must make it better (22 years later still not great).
	_draw_fill_rect(cr,p_x,from,1,to-from,theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE]); //must make it better

}


bool EditorChionicEnvelope::on_button_press_event(GdkEventButton *event) {
	mouse_button(event,true);
	return false;
}

bool EditorChionicEnvelope::on_button_release_event(GdkEventButton *release_event) {
	mouse_button(release_event,false);
	return false;
}


void EditorChionicEnvelope::on_size_allocate(Gtk::Allocation &allocation) {
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

void EditorChionicEnvelope::on_realize() {
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

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK |
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

void EditorChionicEnvelope::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

void EditorChionicEnvelope::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void EditorChionicEnvelope::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}

bool EditorChionicEnvelope::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	size_w = w;
	size_h = h;

	_draw_fill_rect(cr,0,0,w,h,theme->colors[Theme::COLOR_BACKGROUND]);

	theme->select_font_face(cr);

	Cairo::FontExtents fe;
	cr->get_font_extents(fe);

	int fw = Theme::get_char_width(cr);

	int fh = fe.height;
	int fa = fe.ascent;
	int sep = 1;
	fh += sep;

	font_h_spacing = fh * 2 + 6;

	if (recalc_pending) {
		adjust_zoom_to_window();
		recalc_pending=false;
	}

	for (int i=0;i<w;i++) {

		draw_envelope_pos(i,cr);
	}

	paint_envelope_points(w,h,fh,fa,fw,cr);

	int cur_height=size_h-font_h_spacing-1;
#if 0
	Player *pl=editor->get_player();
	for (int i=0;i<pl->get_virtual_channels();i++) {

		int voice_pos=-1;


		if (pl->get_voice_envelope( i, Instrument::VOLUME_ENVELOPE)==envelope ) {

			voice_pos=pl->get_voice_envelope_pos(i,Instrument::VOLUME_ENVELOPE);

		} else if (pl->get_voice_envelope( i, Instrument::PAN_ENVELOPE)==envelope ) {

			voice_pos=pl->get_voice_envelope_pos(i,Instrument::PAN_ENVELOPE);

		} else if (pl->get_voice_envelope( i, Instrument::PITCH_ENVELOPE)==envelope ) {

			voice_pos=pl->get_voice_envelope_pos(i,Instrument::PITCH_ENVELOPE);
		}

		if (voice_pos==-1)
			continue;

		voice_pos=(int)((float)voice_pos/zoom);
		if (voice_pos>=w)
			continue;

		p->draw_fill_rect(Point(voice_pos,0),Point(1,cur_height),color(COLOR_ENVELOPE_LINE));
	}
#endif
	return false;
}

EditorChionicEnvelope::EditorChionicEnvelope(SynthChionic::EnvelopeLFOType p_envelope_type,SynthChionic::Envelope *p_envelope, Theme *p_theme) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("envelope"),
		Gtk::Widget() {

	envelope_type = p_envelope_type;
	envelope = p_envelope;
	theme = p_theme;
}

EditorChionicEnvelope::~EditorChionicEnvelope() {
}
///////////////////////// EnvelopeLFO


void EditorChionicEnvelopeLFO::_envelope_points_changed() {

	if (updating) {
		return;
	}

	updating = true;

	loop_enable.set_active(envelope.has_loop());
	loop_begin.get_adjustment()->set_upper(envelope.get_point_count()-1);
	loop_begin.get_adjustment()->set_value(envelope.get_loop_begin());
	loop_end.get_adjustment()->set_upper(envelope.get_point_count()-1);
	loop_end.get_adjustment()->set_value(envelope.get_loop_end());

	sustain_loop_enable.set_active(envelope.has_sustain_loop());
	sustain_loop_begin.get_adjustment()->set_upper(envelope.get_point_count()-1);
	sustain_loop_begin.get_adjustment()->set_value(envelope.get_sustain_loop_begin());
	sustain_loop_end.get_adjustment()->set_upper(envelope.get_point_count()-1);
	sustain_loop_end.get_adjustment()->set_value(envelope.get_sustain_loop_end());

	if (!first_setup) {
		chionic->set_envelope(generator,envelope_type,envelope);
	} else {
		first_setup = false;
	}

	updating = false;

}

void EditorChionicEnvelopeLFO::_envelope_changed() {

	if (updating) {
		return;
	}

	updating = true;

	if (loop_end.get_adjustment()->get_value() < loop_begin.get_adjustment()->get_value()) {
		// check crossing
		if (loop_begin.get_adjustment()->get_value() != envelope.get_loop_begin()) { // moved begin, adjust end
			loop_end.get_adjustment()->set_value(loop_begin.get_adjustment()->get_value());
		}

		if (loop_end.get_adjustment()->get_value() != envelope.get_loop_end()) { // moved end, adjust begin
			loop_begin.get_adjustment()->set_value(loop_end.get_adjustment()->get_value());
		}
	}

	if (sustain_loop_end.get_adjustment()->get_value() < sustain_loop_begin.get_adjustment()->get_value()) {
		// check crossing
		if (sustain_loop_begin.get_adjustment()->get_value() != envelope.get_sustain_loop_begin()) { // moved begin, adjust end
			sustain_loop_end.get_adjustment()->set_value(sustain_loop_begin.get_adjustment()->get_value());
		}

		if (sustain_loop_end.get_adjustment()->get_value() != envelope.get_sustain_loop_end()) { // moved end, adjust begin
			sustain_loop_begin.get_adjustment()->set_value(sustain_loop_end.get_adjustment()->get_value());
		}
	}

	envelope.set_use_loop(loop_enable.get_active());
	envelope.set_loop_begin(loop_begin.get_adjustment()->get_value());
	envelope.set_loop_end(loop_end.get_adjustment()->get_value());

	envelope.set_use_sustain_loop(sustain_loop_enable.get_active());
	envelope.set_sustain_loop_begin(sustain_loop_begin.get_adjustment()->get_value());
	envelope.set_sustain_loop_end(sustain_loop_end.get_adjustment()->get_value());

	envelope_editor.queue_draw();

	chionic->set_envelope(generator,envelope_type,envelope);

	updating = false;
}

void EditorChionicEnvelopeLFO::_lfo_changed() {

	lfo.delay = lfo_delay.get_adjustment()->get_value();
	lfo.depth = lfo_depth.get_adjustment()->get_value();
	lfo.rate = lfo_rate.get_adjustment()->get_value();
	lfo.type = OscillatorType(lfo_osc_type.get_active_row_number());
	chionic->set_lfo(generator,envelope_type,lfo);
}

EditorChionicEnvelopeLFO::EditorChionicEnvelopeLFO(SynthChionic * p_chionic,int p_generator,SynthChionic::EnvelopeLFOType p_envelope_type) : envelope_editor(p_envelope_type, &envelope, ::Interface::get_singleton()->get_theme()){
	chionic = p_chionic;
	generator = p_generator;
	envelope_type = p_envelope_type;

	envelope = p_chionic->get_envelope(generator,envelope_type);
	lfo = p_chionic->get_lfo(generator,envelope_type);


	envelope_label.set_markup("<b>Envelope:</b>");
	envelope_label.set_halign(Gtk::Align::ALIGN_START);
	envelope_label.set_margin_top(5);
	envelope_label.set_margin_bottom(5);
	attach(envelope_label,0,column,4,1);
	column++;
	attach(envelope_editor,0,column,4,1);
	envelope_editor.set_size_request(128,200);

	column++;

	loop_enable.set_label("Loop");
	attach(loop_enable,0,column,2,1);

	sustain_loop_enable.set_label("Sustain Loop");
	attach(sustain_loop_enable,2,column,2,1);

	column++;

	loop_begin_label.set_text("Begin");
	attach(loop_begin_label,0,column,1,1);


	attach(loop_begin,1,column,1,1);

	loop_begin.set_adjustment(Gtk::Adjustment::create(0, 0, 1));
	loop_end.set_adjustment(Gtk::Adjustment::create(0, 0, 1));

	sustain_loop_begin_label.set_text("Begin");
	attach(sustain_loop_begin_label,2,column,1,1);

	attach(sustain_loop_begin,3,column,1,1);

	sustain_loop_begin.set_adjustment(Gtk::Adjustment::create(0, 0, 1));
	sustain_loop_end.set_adjustment(Gtk::Adjustment::create(0, 0, 1));

	column++;

	loop_end_label.set_text("End");
	attach(loop_end_label,0,column,1,1);
	loop_end_label.set_hexpand(true);

	attach(loop_end,1,column,1,1);

	loop_end.set_hexpand(true);

	sustain_loop_end_label.set_text("End");
	attach(sustain_loop_end_label,2,column,1,1);

	sustain_loop_end_label.set_hexpand(true);

	attach(sustain_loop_end,3,column,1,1);

	sustain_loop_end.set_hexpand(true);

	column++;

	lfo_label.set_markup("<b>LFO:</b>");
	lfo_label.set_halign(Gtk::Align::ALIGN_START);
	lfo_label.set_margin_top(5);
	lfo_label.set_margin_bottom(5);
	attach(lfo_label,0,column,4,1);

	column++;

	lfo_osc_label.set_text("OSC Type");
	attach(lfo_osc_label,0,column,1,1);

	attach(lfo_osc_type,1,column,1,1);

	{

		lfo_osc_list_store = Gtk::ListStore::create(combo_model_columns);
		lfo_osc_type.set_model(lfo_osc_list_store);

		const char *text[OSC_TYPE_MAX] = {
			"Sine",
			"Triangle",
			"Square",
			"Saw",
		};

		for (int i = 0; i < OSC_TYPE_MAX; i++) {
			Gtk::TreeModel::Row row = *(lfo_osc_list_store->append());
			row[combo_model_columns.name] = text[i];
			row[combo_model_columns.index] = i;

			if (lfo.type==i) {
				lfo_osc_type.set_active(row);
			}
		}

		lfo_osc_type.pack_start(combo_model_columns.name);
	}

	lfo_delay_label.set_text("Delay");
	attach(lfo_delay_label,2,column,1,1);

	attach(lfo_delay,3,column,2,1);

	column++;

	lfo_depth_label.set_text("Depth");
	attach(lfo_depth_label,0,column,1,1);

	attach(lfo_depth,1,column,1,1);

	lfo_rate_label.set_text("Rate");
	attach(lfo_rate_label,2,column,1,1);

	attach(lfo_rate,3,column,1,1);


	lfo_rate.set_adjustment(Gtk::Adjustment::create(lfo.rate,0.01,10,0.01));
	lfo_rate.set_digits(2);
	lfo_rate.signal_output().connect([&]() -> bool {
	    Glib::ustring text = Glib::ustring::format(lfo_rate.get_value()) + " hz";
	    lfo_rate.set_text(text);
	    return true; // Indicate that the custom text is set
	});

	lfo_delay.set_adjustment(Gtk::Adjustment::create(lfo.delay,0,5,0.1));
	lfo_delay.set_digits(1);
	lfo_delay.signal_output().connect([&]() -> bool {
	    Glib::ustring text = Glib::ustring::format(lfo_delay.get_value()) + " sec(s)";
	    lfo_delay.set_text(text);
	    return true; // Indicate that the custom text is set
	});

	lfo_depth.set_adjustment(Gtk::Adjustment::create(lfo.depth,0,1,0.01));
	lfo_depth.set_digits(2);
	/*lfo_depth.signal_output().connect([&]() -> bool {
	    Glib::ustring text = Glib::ustring::format(lfo_depth.get_value()) + " %";
	    lfo_depth.set_text(text);
	    return true; // Indicate that the custom text is set
	});*/


	_envelope_points_changed();

	envelope_editor.envelope_changed.connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_points_changed));

	sustain_loop_enable.signal_toggled().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));
	sustain_loop_begin.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));
	sustain_loop_end.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));

	loop_enable.signal_toggled().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));
	loop_begin.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));
	loop_end.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_envelope_changed));

	lfo_osc_type.signal_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_lfo_changed));

	lfo_delay.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_lfo_changed));
	lfo_depth.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_lfo_changed));
	lfo_rate.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicEnvelopeLFO::_lfo_changed));

}

///////////////////////// Generator

void EditorChionicGenerator::_generator_main_changed() {

	Gtk::TreeModel::iterator iter = type_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[combo_model_columns.index];

			if (id == SynthChionic::GEN_TYPE_DISABLED) {
				notebook.hide();
			} else {
				notebook.show();
			}

			if (id == SynthChionic::GEN_TYPE_OSCILLATOR) {
				oscillator_grid.show();
			} else {
				oscillator_grid.hide();
			}

			if (!first_setup) {
				chionic->set_generator_type(generator,SynthChionic::GeneratorType(id));
			}
		}
	}

	iter = modulation_source.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[combo_model_columns.index];

			if (!first_setup) {
				chionic->set_modulation_source(generator,id);
			}
		}
	}

	iter = modulation_combo.get_active();
	if (iter) {
		Gtk::TreeModel::Row row = *iter;
		if (row) {
			//Get the data for the selected row, using our knowledge of the tree
			//model:
			int id = row[combo_model_columns.index];

			if (!first_setup) {
				chionic->set_modulation_mode(generator,SynthChionic::ModulationMode(id));
			}
		}
	}

	bool disable_modulation = generator == 0 || chionic->get_generator_type(generator) != SynthChionic::GEN_TYPE_OSCILLATOR;
	modulation_source.set_sensitive(!disable_modulation);
	modulation_combo.set_sensitive(!disable_modulation);


}

void EditorChionicGenerator::_oscillator_changed() {
	if (updating) {
		return;
	}

	{
		Gtk::TreeModel::iterator iter = oscillator_type_combo.get_active();
		if (iter) {
			Gtk::TreeModel::Row row = *iter;
			if (row) {
				//Get the data for the selected row, using our knowledge of the tree
				//model:
				int id = row[combo_model_columns.index];

				chionic->set_oscillator_type( generator, OscillatorType(id));
			}
		}
	}

	chionic->set_oscillator_shape(generator, oscillator_shape.get_adjustment()->get_value());


	oscillator_view.set_osc_type( chionic->get_oscillator_type(generator) );
	oscillator_view.set_shape(chionic->get_oscillator_shape(generator) );

}

void EditorChionicGenerator::_parameters_changed() {

	chionic->set_fine_tune(generator, fine_tune.get_adjustment()->get_value());
	chionic->set_coarse_tune(generator, coarse_tune.get_adjustment()->get_value());
	chionic->set_volume_db(generator,volume_db.get_adjustment()->get_value());
	chionic->set_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE,volume_randomness.get_adjustment()->get_value());

	chionic->set_punch_amount(generator,volume_punch_amount_db.get_adjustment()->get_value());
	chionic->set_punch_time(generator,volume_punch_time.get_adjustment()->get_value());


	chionic->set_pan(generator,pan.get_adjustment()->get_value());
	chionic->set_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_PAN,pan_randomness.get_adjustment()->get_value());

	chionic->set_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_SHAPE,shape_randomness.get_adjustment()->get_value());

	chionic->set_shape_velocity_ratio(generator,shape_velocity_ratio.get_adjustment()->get_value());

	chionic->set_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_FILTER,filter_randomness.get_adjustment()->get_value());
	chionic->set_filter_velocity_ratio(generator,filter_velocity_ratio.get_adjustment()->get_value());

	chionic->set_filter_stages(generator,filter_stages.get_adjustment()->get_value());
	filter_array_editor.set_filter_stages(filter_stages.get_adjustment()->get_value());
}

void EditorChionicGenerator::_filter_changed() {

	for(int i=0;i<SynthChionic::MAX_FILTERS;i++) {
		SynthChionic::Filter f;
		if (i<filter_array_editor.get_filter_count()) {
			f.enabled=true;
			f.cutoff_hz=filter_array_editor.get_filter_cutoff(i);
			f.resonance=filter_array_editor.get_filter_resonance(i);
			f.mode=filter_array_editor.get_filter_mode(i);
		}
		chionic->set_filter(generator,i,f);
	}

}



EditorChionicGenerator::EditorChionicGenerator(SynthChionic *p_chionic, int p_generator) : oscillator_view(::Interface::get_singleton()->get_theme()), filter_array_editor(SynthChionic::MAX_FILTERS,::Interface::get_singleton()->get_theme()) {
	chionic = p_chionic;
	generator = p_generator;

	type_label.set_hexpand(true);
	type_combo.set_hexpand(true);
	type_label.set_text("Generator Type: ");
	attach(type_label, 0, 0, 1, 1);
	attach(type_combo, 1, 0, 1, 1);
	set_column_homogeneous(true);

	{

		type_combo_list_store = Gtk::ListStore::create(combo_model_columns);
		type_combo.set_model(type_combo_list_store);

		const char *text[4] = {
			"Disabled",
			"Oscillator",
			"Sampler",
			"Noise",
		};

		for (int i = 0; i < 4; i++) {
			Gtk::TreeModel::Row row = *(type_combo_list_store->append());
			row[combo_model_columns.name] = text[i];
			row[combo_model_columns.index] = i;

			if (chionic->get_generator_type(generator)==i) {
				type_combo.set_active(row);
			}
		}

		type_combo.pack_start(combo_model_columns.name);

		type_combo.signal_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_generator_main_changed));
	}


	modulation_label.set_text("Modulate From: ");
	modulation_hbox.add(modulation_source);
	modulation_hbox.add(modulation_combo);
	modulation_source.set_hexpand(true);

	attach(modulation_label, 0, 1, 1, 1);
	attach(modulation_hbox, 1, 1, 1, 1);

	{

		modulation_source_list_store = Gtk::ListStore::create(combo_model_columns);
		modulation_source.set_model(modulation_source_list_store);

		const char *text[5] = {
			"Disabled",
			"Wave0",
			"Wave1",
			"Wave2",
			"Wave3",
		};

		for (int i = -1; i < p_generator; i++) {
			Gtk::TreeModel::Row row = *(modulation_source_list_store->append());
			row[combo_model_columns.name] = text[i+1];
			row[combo_model_columns.index] = i;

			if (chionic->get_modulation_source(generator)==i) {
				modulation_source.set_active(row);
			}
		}

		modulation_source.pack_start(combo_model_columns.name);

		modulation_source.signal_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_generator_main_changed));
	}

	if (p_generator == 0) {
		modulation_source.set_sensitive(false);
		modulation_combo.set_sensitive(false);
	}

	{

		modulation_combo_list_store = Gtk::ListStore::create(combo_model_columns);
		modulation_combo.set_model(modulation_combo_list_store);

		const char *text[3] = {
			"Add",
			"Ring",
			"FM",
		};

		for (int i = 0; i < 3; i++) {
			Gtk::TreeModel::Row row = *(modulation_combo_list_store->append());
			row[combo_model_columns.name] = text[i];
			row[combo_model_columns.index] = i;

			if (chionic->get_modulation_mode(generator)==i) {
				modulation_combo.set_active(row);
			}
		}

		modulation_combo.pack_start(combo_model_columns.name);

		modulation_combo.signal_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_generator_main_changed));
	}

	attach(notebook, 0, 2, 2, 1);

	notebook.append_page(oscillator_grid,"Oscillator");
	notebook.set_hexpand(true);
	notebook.set_vexpand(true);

	int column = 0;

	oscillator_grid.attach(oscillator_view, 0, column, 2, 1);

	oscillator_view.set_size_request(128,128);
	oscillator_view.set_osc_type( chionic->get_oscillator_type(generator) );
	oscillator_view.set_shape(chionic->get_oscillator_shape(generator) );

	column++;
	oscillator_type_label.set_hexpand(true);
	oscillator_type_combo.set_hexpand(true);
	oscillator_type_label.set_text("Oscilator Type: ");
	oscillator_grid.attach(oscillator_type_label, 0, column, 1, 1);
	oscillator_grid.attach(oscillator_type_combo, 1, column, 1, 1);

	{

		oscillator_type_combo_list_store = Gtk::ListStore::create(combo_model_columns);
		oscillator_type_combo.set_model(oscillator_type_combo_list_store);

		const char *text[OSC_TYPE_MAX] = {
			"Sine",
			"Triangle",
			"Square",
			"Saw",
		};

		for (int i = 0; i < OSC_TYPE_MAX; i++) {
			Gtk::TreeModel::Row row = *(oscillator_type_combo_list_store->append());
			row[combo_model_columns.name] = text[i];
			row[combo_model_columns.index] = i;

			if (chionic->get_generator_type(generator)==i) {
				oscillator_type_combo.set_active(row);
			}
		}

		oscillator_type_combo.pack_start(combo_model_columns.name);

		oscillator_type_combo.signal_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_oscillator_changed));
	}

	column++;

	oscillator_shape_label.set_text("Oscilator Shape: ");
	oscillator_grid.attach(oscillator_shape_label, 0, column, 1, 1);
	oscillator_grid.attach(oscillator_shape, 1, column, 1, 1);
	oscillator_shape.set_adjustment(Gtk::Adjustment::create(chionic->get_oscillator_shape(generator), 0, 1, 0.01));
	oscillator_shape.set_digits(2);
	oscillator_shape.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_oscillator_changed));


	for(int i=0;i<SynthChionic::ENVELOPE_LFO_TYPE_MAX;i++) {
		env_lfo[i] = new EditorChionicEnvelopeLFO(p_chionic,p_generator,SynthChionic::EnvelopeLFOType(i));
		static const char * env_names[SynthChionic::ENVELOPE_LFO_TYPE_MAX] = {
			"Volume",
			"Pan",
			"Filter",
			"Pitch",
			"Shape"
		};
		notebook.append_page(*env_lfo[i],env_names[i]);
	}


	/// Addition to existing pages.

	{
		column = env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->get_column();
		column++;

		pitch_label.set_markup("<b>Tuning:</b>");
		pitch_label.set_halign(Gtk::Align::ALIGN_START);
		pitch_label.set_margin_top(5);
		pitch_label.set_margin_bottom(5);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->attach(pitch_label,0,column,4,1);
		column++;


		fine_tune_label.set_text("Fine Tune: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->attach(fine_tune_label, 0, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->attach(fine_tune, 1, column, 1, 1);
		fine_tune.set_adjustment(Gtk::Adjustment::create(chionic->get_fine_tune(generator), -1, 1, 0.01));
		fine_tune.set_digits(2);
		fine_tune.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		coarse_tune_label.set_text("Coarse Tune: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->attach(coarse_tune_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PITCH]->attach(coarse_tune, 3, column, 1, 1);
		coarse_tune.set_adjustment(Gtk::Adjustment::create(chionic->get_coarse_tune(generator), SynthChionic::MIN_COARSE_TUNE, SynthChionic::MAX_COARSE_TUNE, 1));
		coarse_tune.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

	}

	{
		column = env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->get_column();
		column++;

		volume_label.set_markup("<b>Volume:</b>");
		volume_label.set_halign(Gtk::Align::ALIGN_START);
		volume_label.set_margin_top(5);
		volume_label.set_margin_bottom(5);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_label,0,column,4,1);
		column++;


		volume_db_label.set_text("Volume (dB): ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_db_label, 0, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_db, 1, column, 1, 1);
		volume_db.set_adjustment(Gtk::Adjustment::create(chionic->get_volume_db(generator), -60, 24, 0.1));
		volume_db.set_digits(1);
		volume_db.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		volume_randomness_label.set_text("Randomness: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_randomness_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_randomness, 3, column, 1, 1);
		volume_randomness.set_adjustment(Gtk::Adjustment::create(chionic->get_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE), 0, 1, 0.01));
		volume_randomness.set_digits(2);
		volume_randomness.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		column++;

		volume_punch_amount_db_label.set_text("Punch (dB): ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_punch_amount_db_label, 0, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_punch_amount_db, 1, column, 1, 1);
		volume_punch_amount_db.set_adjustment(Gtk::Adjustment::create(chionic->get_punch_amount(generator), 0, 24, 0.1));
		volume_punch_amount_db.set_digits(1);
		volume_punch_amount_db.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		volume_punch_time_label.set_text("Punch Time: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_punch_time_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_AMPLITUDE]->attach(volume_punch_time, 3, column, 1, 1);
		volume_punch_time.set_adjustment(Gtk::Adjustment::create(chionic->get_punch_time(generator), 0, 1, 0.01));
		volume_punch_time.set_digits(2);
		volume_punch_time.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		volume_punch_time.signal_output().connect([&]() -> bool {
		    Glib::ustring text = Glib::ustring::format(volume_punch_time.get_value()) + " sec";
		    volume_punch_time.set_text(text);
		    return true; // Indicate that the custom text is set
		});

	}

	{
		column = env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->get_column();
		column++;

		pan_main_label.set_markup("<b>Panning:</b>");
		pan_main_label.set_halign(Gtk::Align::ALIGN_START);
		pan_main_label.set_margin_top(5);
		pan_main_label.set_margin_bottom(5);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->attach(pan_main_label,0,column,4,1);
		column++;


		pan_label.set_text("Position: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->attach(pan_label, 0, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->attach(pan, 1, column, 1, 1);
		pan.set_adjustment(Gtk::Adjustment::create(chionic->get_pan(generator), -1, 1, 0.01));
		pan.set_digits(2);
		pan.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		pan_randomness_label.set_text("Randomness: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->attach(pan_randomness_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_PAN]->attach(pan_randomness, 3, column, 1, 1);
		pan_randomness.set_adjustment(Gtk::Adjustment::create(chionic->get_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_PAN), 0, 1, 0.01));
		pan_randomness.set_digits(2);
		pan_randomness.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

	}

	{
		column = env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->get_column();
		column++;

		shape_label.set_markup("<b>Shape:</b>");
		shape_label.set_halign(Gtk::Align::ALIGN_START);
		shape_label.set_margin_top(5);
		shape_label.set_margin_bottom(5);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->attach(shape_label,0,column,4,1);
		column++;


		shape_velocity_ratio_label.set_text("Velocity Ratio: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->attach(shape_velocity_ratio_label, 0, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->attach(shape_velocity_ratio, 1, column, 1, 1);
		shape_velocity_ratio.set_adjustment(Gtk::Adjustment::create(chionic->get_shape_velocity_ratio(generator), 0, 1, 0.01));
		shape_velocity_ratio.set_digits(2);
		shape_velocity_ratio.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		shape_randomness_label.set_text("Randomness: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->attach(shape_randomness_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_SHAPE]->attach(shape_randomness, 3, column, 1, 1);
		shape_randomness.set_adjustment(Gtk::Adjustment::create(chionic->get_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_SHAPE), 0, 1, 0.01));
		shape_randomness.set_digits(2);
		shape_randomness.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

	}

	{
		column = env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->get_column();
		column++;

		filter_label.set_markup("<b>Filter:</b>");
		filter_label.set_halign(Gtk::Align::ALIGN_START);
		filter_label.set_margin_top(5);
		filter_label.set_margin_bottom(5);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_label,0,column,4,1);
		column++;

		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_array_editor, 0, column, 2, 3);
		filter_array_editor.set_size_request(50,100);
		filter_array_editor.filter_changed.connect(sigc::mem_fun(*this, &EditorChionicGenerator::_filter_changed));

		int filter_index =0;
		for(int i=0;i<SynthChionic::MAX_FILTERS;i++) {
			SynthChionic::Filter f = chionic->get_filter(generator,i);
			if (f.enabled) {
				filter_array_editor.set_filter_mode(filter_index,f.mode);
				filter_array_editor.set_filter_cutoff(filter_index,f.cutoff_hz);
				filter_array_editor.set_filter_resonance(filter_index,f.resonance);
				filter_index++;
			}
		}

		filter_array_editor.set_filter_count(filter_index);
		filter_array_editor.set_filter_stages(chionic->get_filter_stages(generator));

		filter_velocity_ratio_label.set_text("Velocity Ratio: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_velocity_ratio_label, 2, column, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_velocity_ratio, 3, column, 1, 1);
		filter_velocity_ratio.set_adjustment(Gtk::Adjustment::create(chionic->get_filter_velocity_ratio(generator), 0, 1, 0.01));
		filter_velocity_ratio.set_digits(2);
		filter_velocity_ratio.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		filter_randomness_label.set_text("Randomness: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_randomness_label, 2, column+1, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_randomness, 3, column+1, 1, 1);
		filter_randomness.set_adjustment(Gtk::Adjustment::create(chionic->get_randomness(generator,SynthChionic::ENVELOPE_LFO_TYPE_FILTER), 0, 1, 0.01));
		filter_randomness.set_digits(2);
		filter_randomness.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));

		filter_stages_label.set_text("Stages: ");
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_stages_label, 2, column+2, 1, 1);
		env_lfo[SynthChionic::ENVELOPE_LFO_TYPE_FILTER]->attach(filter_stages, 3, column+2, 1, 1);
		filter_stages.set_adjustment(Gtk::Adjustment::create(chionic->get_filter_stages(generator), 1, 4, 1));
		filter_stages.signal_value_changed().connect(sigc::mem_fun(*this, &EditorChionicGenerator::_parameters_changed));


	}

	// because of show_all needs to be done afterwards
	Glib::signal_idle().connect_once([this]() {
		_generator_main_changed();
		first_setup = false;
	});
}

EditorChionicGenerator::~EditorChionicGenerator() {
	for(int i=0;i<SynthChionic::ENVELOPE_LFO_TYPE_MAX;i++) {
		delete env_lfo[i];
	}
}

EffectEditorChionic::EffectEditorChionic(SynthChionic *p_chionic, EffectEditor *p_editor) :
		effect_editor_midi(p_chionic, p_editor) {

	synth_chionic = p_chionic;

	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);

	for(int i=SynthChionic::MAX_GENERATORS-1; i>=0 ;i--) {
		generator_editor[i] = new EditorChionicGenerator(p_chionic,i);
		effect_editor_midi.prepend_page(*generator_editor[i], "Wave"+std::to_string(i+1));
	}

	show_all_children();


}

EffectEditorChionic::~EffectEditorChionic() {
	for(int i=0;i<SynthChionic::MAX_GENERATORS;i++) {
		delete generator_editor[i];
	}
}

Gtk::Widget *create_chionic_editor(AudioEffect *p_fx, EffectEditor *p_editor) {
	if (p_fx->get_provider_id() != "internal" || p_fx->get_unique_id() != "chionic") {
		return NULL;
	}
	return new EffectEditorChionic(static_cast<SynthChionic *>(p_fx), p_editor);
}
