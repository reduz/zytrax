#include "filter_array_editor.h"
#include "dsp/db.h"

#define FREQ_2_LOG( m_val ) ( log( (m_val) ) / log (2) )

#define LOG_2_FREQ( m_val ) ( pow(2, (m_val)) )


static float float_2_freq(float p_float,float p_min,float p_max) {

	float logfbeg=FREQ_2_LOG(p_min);
	float logfend=FREQ_2_LOG(p_max);
	float logf=logfbeg+(logfend-logfbeg)*p_float;
	return LOG_2_FREQ(logf);

}


static int freq_2_pixel(float p_freq,float p_min,float p_max,float p_width) {

	float w=p_width;
	float logfbeg=FREQ_2_LOG(p_min);
	float logfend=FREQ_2_LOG(p_max);
	float logf=FREQ_2_LOG(p_freq);

	return (int) (w * ( (logf-logfbeg)/(logfend-logfbeg)));
}


void FilterArrayEditor::_draw_text(const Cairo::RefPtr<Cairo::Context> &cr, int x,
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

int FilterArrayEditor::_get_text_width(const Cairo::RefPtr<Cairo::Context> &cr, const String &p_text) const {
	Cairo::TextExtents te;
	cr->get_text_extents(p_text.utf8().get_data(), te);
	return te.width;
}

void FilterArrayEditor::_draw_fill_rect(const Cairo::RefPtr<Cairo::Context> &cr,
		int x, int y, int w, int h,
		const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->stroke();
}

void FilterArrayEditor::_draw_rect(const Cairo::RefPtr<Cairo::Context> &cr, int x,
		int y, int w, int h, const Gdk::RGBA &p_color) {

	Gdk::Cairo::set_source_rgba(cr, p_color);
	cr->rectangle(x, y, w, h);
	cr->stroke();
}


bool FilterArrayEditor::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();

	int w = allocation.get_width();
	int h = allocation.get_height();

	size_w = w;
	size_h = h;

	_draw_fill_rect(cr,0,0,w,h,theme->colors[Theme::COLOR_BACKGROUND]);

	theme->select_font_face(cr);

	Cairo::FontExtents fe;
	cr->get_font_extents(fe);

	Cairo::TextExtents te;
	cr->get_text_extents("XXX", te);
	int fw = te.width;
	cr->get_text_extents("XX", te);
	fw -= te.width;

	if (w<1)
		return false;

	/* Frequency response graph */
	float *response = new float[w];
	int response_len=w;

	for (int i=0;i<response_len;i++)
		response[i]=0;


	float response_roof=0;

	for (int i=0;i<filters;i++) {

		Filter f;

		f.set_mode( filter[i].mode );

		f.set_cutoff( filter[i].cutoff  );

		f.set_resonance( filter[i].resonance );
		f.set_stages( stages );


		Filter::Coeffs fc;
		f.prepare_coefficients(&fc);


		for (int j=0;j<response_len;j++) {


			float freq= float_2_freq(  (float)j/(float)response_len  ,MIN_FREQ, MAX_FREQ );

			float freqresponse=f.get_response( freq, &fc );
			freqresponse=powf(freqresponse,stages); //apply stages

			response[j]+=freqresponse;
			if (response[j]>response_roof)
				response_roof=response[j];

		}
	}


	float linear_roof=sqrt(response_roof);

	if (linear_roof<AMP_STEPS)
		linear_roof=AMP_STEPS;

	for (int i=0;i<response_len;i++) {

		int gh=(int)( sqrt(response[i])*h/linear_roof );
		_draw_rect(cr,i,h -gh,1,gh,theme->colors[Theme::COLOR_PATTERN_EDITOR_NOTE]);

	}

	Gdk::RGBA text_outline = theme->colors[Theme::COLOR_BACKGROUND];
	Gdk::RGBA text_color = theme->colors[Theme::COLOR_FOCUS];

	Gdk::RGBA filter_color = theme->colors[Theme::COLOR_FOCUS];
	Gdk::RGBA filter_color_bg = filter_color;
	filter_color_bg.set_alpha( filter_color_bg.get_alpha() * 0.7 );


	for (int i=0;i<filters;i++) {

		int x=freq_2_pixel( filter[i].cutoff ,MIN_FREQ,MAX_FREQ,response_len);

		bool selected=(drag.index==i || (drag.over==i && drag.index==-1));

		Gdk::RGBA c = selected ? filter_color : filter_color_bg;

		_draw_rect(cr,x,0,1,h,c);
		float range=(filter[i].resonance/ (RESONANCE_MAX-RESONANCE_MIN))*(float)size_h;
		_draw_rect(cr,x-4,h - range, 9,1,c);

		String s=String::num(i+1);
		int wd = _get_text_width(cr,s);

		x-=wd/2;
		int y=h-fe.descent-2;
		_draw_text(cr,x-1,y-1,s,text_outline,false);
		_draw_text(cr,x+1,y-1,s,text_outline,false);
		_draw_text(cr,x+1,y+1,s,text_outline,false);
		_draw_text(cr,x-1,y+1,s,text_outline,false);
		_draw_text(cr,x,y,s,text_color,false);

	}
	/* show freq and dB */

	if (filters > 0) {
		String max_db=String("Peak: ") + (response_roof==0?String("-oo"):String::num(linear2db(response_roof),1) ) + "dB";
		_draw_text(cr,5,3+fe.ascent,max_db,text_color,false);
	} else {
		_draw_text(cr,5,3+fe.ascent,"Click to add a filter",text_color,false);
	}

	if (drag.over>=0) {

		float cutoff=filter[drag.over].cutoff;

		String text;

		if (cutoff>=1000.0)

			text=String::num( cutoff/1000,1 )  + "khz";
		else
			text=String::num( (int)cutoff ) + "hz";

		_draw_text(cr, w - _get_text_width(cr,text) - 3, 3 + fe.ascent, text, text_color, false);
	}



	delete[] response;

	return false;
}


bool FilterArrayEditor::on_button_press_event(GdkEventButton *event) {

	if (event->button == 3 && drag.index==-1) {

		if (drag.over>=0) {
			for(int i=drag.over;i<filters;i++) {
				filter[i]=filter[i+1];
			}
			filters--;
			drag.over = -1;

			queue_draw();
			filter_changed.emit();
		}
	}

	if (event->button != 1)
		return false;

	if (drag.over>=0) {
		drag.index=drag.over;
	} else if (filters < filter_max) {

		float freq= float_2_freq(  (float)event->x/(float)size_w ,MIN_FREQ,MAX_FREQ );

		filter[filters].cutoff = freq;
		filter[filters].resonance = CLAMP((float)(size_h-event->y)/size_h, RESONANCE_MIN, RESONANCE_MAX );

		new_filter_popup.popup(event->button,event->time);

	}

	return false;
}

bool FilterArrayEditor::on_motion_notify_event(GdkEventMotion *motion_event) {

	if (drag.index==-1) {

		/* no drag, find over */

		int closest=-1;
		int closest_dist=0;

		for (int i=0;i<filters;i++) {
			int x=freq_2_pixel( filter[i].cutoff ,MIN_FREQ,MAX_FREQ,size_w);

			int distance=fabs(motion_event->x-x);
			if (distance>10)
				continue;

			if (closest==-1 || closest_dist>distance)
				closest=i;

		}

		if (closest!=drag.over) {

			drag.over=closest;
			queue_draw();
		}

	} else {

		float freq= float_2_freq(  (float)motion_event->x/(float)size_w ,MIN_FREQ,MAX_FREQ );

		filter[drag.index].cutoff = CLAMP(freq,MIN_FREQ, MAX_FREQ);
		float reso=(float)(size_h-motion_event->y)/size_h;
		if (reso<0)
			reso=0;
		if (reso>1)
			reso=1;

		//reso*=filter[drag.index].resonance;
		filter[drag.index].resonance=reso;
		queue_draw();
		filter_changed.emit();
	}

	return false;

}

bool FilterArrayEditor::on_button_release_event(GdkEventButton *event) {

	drag.index=-1;
	queue_draw();

	return false;
}

void FilterArrayEditor::on_size_allocate(Gtk::Allocation &allocation) {
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

void FilterArrayEditor::on_realize() {
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

		attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK | Gdk::POINTER_MOTION_MASK |
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

void FilterArrayEditor::on_unrealize() {
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}



void FilterArrayEditor::set_filter_count(int p_filters) {
	ERR_FAIL_INDEX(p_filters,filter_max)
	filters = p_filters;

}

int FilterArrayEditor::get_filter_count() const {
	return filters;
}

void FilterArrayEditor::set_filter_stages(int p_stages) {
	ERR_FAIL_COND(p_stages <1 || p_stages>4);
	stages = p_stages;
	queue_draw();
}
int FilterArrayEditor::get_filter_stages() const {
	return stages;
}

void FilterArrayEditor::set_filter_mode(int p_index, Filter::Mode p_mode) {
	ERR_FAIL_INDEX(p_index, filter_max);
	filter[p_index].mode = p_mode;
	queue_draw();
}

Filter::Mode FilterArrayEditor::get_filter_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, filter_max,Filter::BANDPASS);
	return filter[p_index].mode;
}

void FilterArrayEditor::set_filter_cutoff(int p_index, float p_cutoff) {
	ERR_FAIL_INDEX(p_index, filter_max);
	filter[p_index].cutoff= CLAMP(p_cutoff,MIN_FREQ,MAX_FREQ);
	queue_draw();
}

float FilterArrayEditor::get_filter_cutoff(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, filter_max,0);
	return filter[p_index].cutoff;
}

void FilterArrayEditor::set_filter_resonance(int p_index, float p_resonance) {
	ERR_FAIL_INDEX(p_index, filter_max);
	filter[p_index].resonance= CLAMP(p_resonance,RESONANCE_MIN,RESONANCE_MAX);
	queue_draw();

}
float FilterArrayEditor::get_filter_resonance(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, filter_max,0);
	return filter[p_index].resonance;
}

void FilterArrayEditor::_filter_added(int p_idx) {
	filter[filters].mode = Filter::Mode(p_idx);
	filters++;
	queue_draw();
	filter_changed.emit();
}

FilterArrayEditor::FilterArrayEditor(int p_filter_max, Theme *p_theme) {


	theme = p_theme;
	filter_max=p_filter_max;
	filter	= new FilterData[filter_max];
	filters = 0;

	new_filter_menu = Gio::Menu::create();


	std::vector<std::string> filter_names = {
		"Lowpass", "Highpass", "Bandpass", "Notch"};

	// Create an ActionGroup
	auto action_group = Gio::SimpleActionGroup::create();

	for (size_t i = 0; i < filter_names.size(); ++i) {
		const auto& name = filter_names[i];
		new_filter_menu->append(name, "menu.filter_" + std::to_string(i));

		// Add action for this menu item
		action_group->add_action(
					"filter_" + std::to_string(i),
					[this,i]() {
			_filter_added(i);
		});
	}
	// Bind actions to this widget
	insert_action_group("menu", action_group);

	// Bind menu model to popup
	new_filter_popup.bind_model(new_filter_menu, true);
	new_filter_popup.attach_to_widget(*this);

}


FilterArrayEditor::~FilterArrayEditor(){

	delete[] filter;
}
