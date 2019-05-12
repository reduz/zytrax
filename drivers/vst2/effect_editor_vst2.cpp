#include "effect_editor_vst2.h"
//keep this for order
#include "audio_effect_provider_vst2.h"

void EffectPlaceholderVST2::_vst_resize(void *self, int w, int h) {
	EffectPlaceholderVST2 *ph = (EffectPlaceholderVST2 *)self;
	return;
}

void EffectPlaceholderVST2::resize_editor(int left, int top, int right, int bottom) {
	if (vst_window) {
		RECT rc;
		rc.left = left;
		rc.right = right;
		rc.bottom = bottom;
		rc.top = top;

		const auto style = GetWindowLongPtr(vst_window, GWL_STYLE);
		const auto exStyle = GetWindowLongPtr(vst_window, GWL_EXSTYLE);
		const BOOL fMenu = GetMenu(vst_window) != nullptr;
		AdjustWindowRectEx(&rc, style, fMenu, exStyle);
		MoveWindow(vst_window, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	}
}
void EffectPlaceholderVST2::on_size_allocate(Gtk::Allocation &allocation) {
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

bool EffectPlaceholderVST2::_update_window_position() {

	bool visible = is_visible();

	if (visible) {
		//make sure it really is..
		GtkWidget *p = gobj();
		GtkWidget *w = gtk_widget_get_parent(p);
		while (w) {
			if (GTK_IS_NOTEBOOK(w)) {
				GtkNotebook *notebook = GTK_NOTEBOOK(w);
				int cpage = gtk_notebook_get_current_page(notebook);
				if (p != gtk_notebook_get_nth_page(notebook, cpage)) {
					visible = false;
					break;
				}
			}

			p = w;
			w = gtk_widget_get_parent(p);
		}
	}

	GtkWidget *toplevel = gtk_widget_get_toplevel(gobj());
	ERR_FAIL_COND_V(!GTK_IS_WINDOW(toplevel), false);
	int root_x, root_y;
	//gtk_window_get_position(GTK_WINDOW(toplevel), &root_x, &root_y);
	gdk_window_get_origin(gtk_widget_get_window(gobj()), &root_x, &root_y);
	int tlx = 0, tly = 0;
	/*gtk_widget_translate_coordinates(gobj(), gtk_widget_get_toplevel(gobj()), 0, 0, &tlx, &tly);
	root_x += tlx;
	root_y += tlx;*/

	if (root_x != prev_x || root_y != prev_y || prev_w != vst_w || prev_h != vst_h) {
		resize_editor(root_x, root_y, root_x + vst_w, root_y + vst_h);
		prev_x = root_x;
		prev_y = root_y;
		prev_w = vst_w;
		prev_h = vst_h;
	}

	if (prev_visible != visible) {
		ShowWindow(vst_window, visible ? SW_SHOW : SW_HIDE);
		prev_visible = visible;
	}

	return true;
}

void EffectPlaceholderVST2::on_realize() {
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

		attributes.event_mask = get_events() | /* Gdk::EXPOSURE_MASK |*/
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

		/* Set the Window Parentship and update timer */
	}

	if (m_refGdkWindow) {
		printf("CREATE\n");
		//hwnd for gtk window
		HWND hwnd = gdk_win32_window_get_impl_hwnd(m_refGdkWindow->gobj());
		//set as parent. This works, while SetParent DOES NOT.
		SetWindowLongPtr(vst_window, GWLP_HWNDPARENT, (LONG_PTR)hwnd);
		/*SetParent((HWND)vst_window, (HWND)hwnd);*/
		//turn on update timer to reposition the Window
		//sorry, this is the only way I found..
		update_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &EffectPlaceholderVST2::_update_window_position),
				50, Glib::PRIORITY_DEFAULT);
		//Show the Window
		ShowWindow(vst_window, SW_SHOW);
		prev_visible = true;
	}

#if 0

		//size allocate
		/*vst_effect->open_user_interface(hwnd);
		vst_effect->get_user_interface_size(vst_w, vst_h);
		set_size_request(vst_w, vst_h);
		printf("rect: %i,%i\n", vst_w, vst_h);*/

		//SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & (~WS_CLIPCHILDREN));
		//const auto style = WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPEDWINDOW;
		const auto style = WS_POPUP;
		vst_window = CreateWindowExW(0, L"VST_HOST", vst_effect->get_path().c_str(), style, 0, 0, 0, 0, NULL, 0, 0, 0);
		SetWindowLongPtr(vst_window, GWLP_HWNDPARENT, (LONG_PTR)hwnd);
		//SetParent((HWND)vst_window, (HWND)hwnd);
		//Whoe the Wind
		ShowWindow(vst_window, SW_SHOW);


		/*
		HWND window_parent = NULL;
		HMODULE hInst = GetModuleHandleA(NULL);
		vst_window = CreateWindowExA(0, "FST", "memario",
				window_parent ? WS_CHILD : (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				(HWND)window_parent, NULL,
				hInst,
				NULL);

		ERR_FAIL_COND(!vst_window);

		if (!SetPropA(vst_window, "fst_ptr", vst_effect)) {
			ERR_PRINT("cant set window local ptr");
		}
		if (window_parent) {
			SetParent((HWND)vst_window, (HWND)window_parent);
		}
*/

		vst_effect->get_user_interface_size(vst_w, vst_h);
		vst_effect->open_user_interface(vst_window);
		vst_effect->get_user_interface_size(vst_w, vst_h);

		printf("xy: %i,%i\n", int(attributes.x), int(attributes.y));
		resize_editor(attributes.x + 0, attributes.y + 0, attributes.x + vst_w, attributes.y + vst_h);
		ShowWindow(vst_window, SW_SHOW);
		set_size_request(vst_w, vst_h);
		/*
		printf("rect: %i,%i\n", vst_w, vst_h);
		SetWindowPos((HWND)vst_window,
				HWND_TOP ,
				0, 0,
				vst_w, vst_h,
				SWP_NOACTIVATE | SWP_NOOWNERZORDER);
		ShowWindow(vst_window, SW_SHOWNA);
		UpdateWindow(vst_window);

		SetTimer(NULL, idle_timer_id, 50, (TIMERPROC)idle_hands);
*/
		effects.push_back(vst_effect);
#endif
#if 0
		* /
/*
		WNDCLASSEX wcex{ sizeof(wcex) };
		wcex.lpfnWndProc = DefWindowProc;
		wcex.hInstance = GetModuleHandle(0);
		wcex.lpszClassName = "Minimal VST host - Guest VST Window Frame";
		RegisterClassEx(&wcex);

		const auto style = WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPEDWINDOW;
		HWND editorHwnd = CreateWindow(
				wcex.lpszClassName, "soso", style, 0, 0, vst_w, vst_h, hwnd, 0, 0, 0);
		printf("hwnd is %p, new %p\n", hwnd, editorHwnd);

		UpdateWindow(editorHwnd);
		RECT rc;
		rc.left = 0;
		rc.right = vst_w;
		rc.top = 0;
		rc.bottom = vst_h;

		//const auto style = GetWindowLongPtr(editorHwnd, GWL_STYLE);
		const auto exStyle = GetWindowLongPtr(editorHwnd, GWL_EXSTYLE);
		const BOOL fMenu = GetMenu(editorHwnd) != nullptr;
		AdjustWindowRectEx(&rc, style, fMenu, exStyle);
		MoveWindow(editorHwnd, 0, 0, vst_w, vst_h, TRUE);

		vst_effect->open_user_interface(&editorHwnd);
		ShowWindow(editorHwnd, SW_SHOW);
		*/
#endif
}

void EffectPlaceholderVST2::on_unrealize() {
	//clear the window
	if (m_refGdkWindow) {
		printf("DESTROY\n");
		//clear parenthood
		SetWindowLongPtr(vst_window, GWLP_HWNDPARENT, (LONG_PTR)NULL);
		//disconnect timer
		update_timer.disconnect();
		//Hide the Window
		ShowWindow(vst_window, SW_HIDE);
		prev_visible = false;
		printf("unrealized dude\n");
	}
	m_refGdkWindow.reset();

	// Call base class:
	Gtk::Widget::on_unrealize();
}

bool EffectPlaceholderVST2::on_visibility_notify_event(GdkEventVisibility *visibility_event) {
	/*
	if (m_refGdkWindow) {
		if (visibility_event->state == GDK_VISIBILITY_FULLY_OBSCURED) {
			ShowWindow(vst_window, SW_HIDE);
		} else {
			ShowWindow(vst_window, SW_SHOW);
		}
	}
*/
	return false;
}

bool EffectPlaceholderVST2::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	const Gtk::Allocation allocation = get_allocation();
	return false;

	Gdk::RGBA rgba;
	rgba.set_red(0);
	rgba.set_green(0);
	rgba.set_blue(0);
	rgba.set_alpha(1);
	Gdk::Cairo::set_source_rgba(cr, rgba);

	cr->rectangle(0, 0, allocation.get_width(), allocation.get_height());
	cr->fill();
}

EffectPlaceholderVST2::EffectPlaceholderVST2(AudioEffectVST2 *p_vst_effect) :
		// The GType name will actually be gtkmm__CustomObject_mywidget
		Glib::ObjectBase("filler"),
		Gtk::Widget() {

	vst_effect = p_vst_effect;
	vst_w = 1;
	vst_h = 1;
	vst_window = NULL;

	//create the window, but don't use it.
	const auto style = WS_POPUP;
	vst_window = CreateWindowExW(0, L"VST_HOST", vst_effect->get_path().c_str(), style, 0, 0, 0, 0, NULL, 0, 0, 0);
	//open the user interface (it won't be visible though.
	vst_effect->open_user_interface(vst_window);
	//allocate size for this VST here
	vst_effect->get_user_interface_size(vst_w, vst_h);
	set_size_request(vst_w, vst_h);
	prev_x = prev_y = prev_w = prev_h = -1;
	prev_visible = false;
}

EffectPlaceholderVST2::~EffectPlaceholderVST2() {
	vst_effect->close_user_interface();
	DestroyWindow(vst_window);
}

void initialize_vst2_editor() {

	HMODULE hInst = GetModuleHandleA(NULL);
	ERR_FAIL_COND(!hInst);

	WNDCLASSEXW wcex{ sizeof(wcex) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpfnWndProc = DefWindowProc;
	wcex.hInstance = GetModuleHandle(0);
	wcex.lpszClassName = L"VST_HOST";

	if (!RegisterClassExW(&wcex)) {
		ERR_PRINT("Error in initialize_vst2_editor(): (class registration failed");
		return;
	}
}

EffectEditorVST2::EffectEditorVST2(AudioEffectVST2 *p_vst, EffectEditor *p_editor) :
		effect_editor_midi(p_vst, p_editor),
		vst_placeholder(p_vst) {

	vst_effect = p_vst;
	effect_editor_midi.prepend_page(vst_placeholder, "VST2 Plugin");
	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);
	show_all_children();
}
