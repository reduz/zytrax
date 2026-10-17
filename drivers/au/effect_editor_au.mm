#ifdef OSX_ENABLED

#include "effect_editor_au.h"
#include "audio_effect_provider_au.h"

#import <Cocoa/Cocoa.h>
#import <AudioUnit/AUCocoaUIView.h>

// Forward-declare the private GDK Quartz accessor (exported but not in public header).
extern "C" {
NSWindow *gdk_quartz_window_get_nswindow(GdkWindow *window);
}

// Borderless windows refuse key status by default, so keystrokes would never
// reach the AU view (e.g. text fields in the plugin UI could not be typed into).
@interface ZytraxAUHostPanel : NSPanel
@end

@implementation ZytraxAUHostPanel
- (BOOL)canBecomeKeyWindow {
	return YES;
}
- (BOOL)canBecomeMainWindow {
	return NO;
}
@end

static NSWindow *_get_toplevel_nswindow(GtkWidget *p_widget) {
	GtkWidget *toplevel = gtk_widget_get_toplevel(p_widget);
	if (!GTK_IS_WINDOW(toplevel)) return nil;
	GdkWindow *gdk_top = gtk_widget_get_window(toplevel);
	if (!gdk_top) return nil;
	return gdk_quartz_window_get_nswindow(gdk_top);
}

// ── EffectPlaceholderAU ───────────────────────────────────────────────────────

EffectPlaceholderAU::EffectPlaceholderAU(AudioEffectAU *p_effect) :
		au_effect(p_effect), au_view(nullptr), au_panel(nullptr),
		au_w(400), au_h(300) {
}

EffectPlaceholderAU::~EffectPlaceholderAU() {
	update_timer.disconnect();
	_close_panel();
}

void EffectPlaceholderAU::_close_panel() {
	if (au_panel) {
		NSPanel *panel = (__bridge_transfer NSPanel *)(void *)au_panel;
		[[panel parentWindow] removeChildWindow:panel];
		[panel orderOut:nil];
		au_panel = nullptr;
		(void)panel; // ARC releases here
	}
	if (au_view) {
		// view is owned by the panel's content view; just release our retain
		NSView *v = (__bridge_transfer NSView *)(void *)au_view;
		(void)v;
		au_view = nullptr;
	}
}

void EffectPlaceholderAU::on_realize() {
	Gtk::DrawingArea::on_realize();

	// Obtain the AU Cocoa view
	void *raw = au_effect->get_user_interface_view();
	if (!raw) return;

	NSView *auView = (__bridge NSView *)(void *)raw;
	au_view = raw; // we hold a +1 retain from get_user_interface_view

	NSRect bounds = [auView bounds];
	au_w = (int)bounds.size.width;
	au_h = (int)bounds.size.height;
	if (au_w < 10) au_w = 400;
	if (au_h < 10) au_h = 300;
	set_size_request(au_w, au_h);

	// Build a borderless NSPanel to host the AU view
	NSRect frame = NSMakeRect(0, 0, au_w, au_h);
	NSPanel *panel = [[ZytraxAUHostPanel alloc]
			initWithContentRect:frame
			          styleMask:NSWindowStyleMaskBorderless
			            backing:NSBackingStoreBuffered
			              defer:NO];
	[panel setOpaque:YES];
	[panel setHasShadow:NO];
	// It is a child of the GTK window, so it should hide/show along with it rather
	// than on app (de)activation.
	[panel setHidesOnDeactivate:NO];
	[[panel contentView] addSubview:auView];
	[auView setFrame:[[panel contentView] bounds]];
	[auView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

	au_panel = (void *)(__bridge_retained void *)panel;

	// Start a timer to keep the panel positioned over this widget (and attached
	// as a child of the GTK window, so it moves with it).
	update_timer = Glib::signal_timeout().connect(
			sigc::mem_fun(*this, &EffectPlaceholderAU::_update_position), 50);

	_update_position();
}

void EffectPlaceholderAU::on_unrealize() {
	update_timer.disconnect();
	_close_panel();
	Gtk::DrawingArea::on_unrealize();
}

bool EffectPlaceholderAU::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	// Placeholder background — the real UI is in the floating panel above.
	cr->set_source_rgb(0.08, 0.08, 0.08);
	cr->paint();
	return true;
}

void EffectPlaceholderAU::on_size_allocate(Gtk::Allocation &alloc) {
	Gtk::DrawingArea::on_size_allocate(alloc);
	_update_position();
}

bool EffectPlaceholderAU::_update_position() {
	if (!au_panel) return false;

	NSPanel *panel = (__bridge NSPanel *)(void *)au_panel;

	// Walk the parent chain to determine if this widget is actually visible
	// (e.g. it might be on a non-current notebook tab).
	bool visible = get_mapped();
	if (visible) {
		GtkWidget *w = gtk_widget_get_parent(GTK_WIDGET(gobj()));
		while (w && visible) {
			if (GTK_IS_NOTEBOOK(w)) {
				GtkNotebook *nb = GTK_NOTEBOOK(w);
				GtkWidget *cur = gtk_notebook_get_nth_page(nb,
						gtk_notebook_get_current_page(nb));
				// Walk up from our widget to see if we're on the current page
				GtkWidget *check = GTK_WIDGET(gobj());
				bool found = false;
				while (check) {
					if (check == cur) { found = true; break; }
					check = gtk_widget_get_parent(check);
					if (check == w) break;
				}
				if (!found) visible = false;
			}
			w = gtk_widget_get_parent(w);
		}
	}

	if (!visible) {
		// Detach too, otherwise AppKit re-shows it whenever the parent is ordered front.
		[[panel parentWindow] removeChildWindow:panel];
		if ([panel isVisible])
			[panel orderOut:nil];
		return true;
	}

	GtkWidget *widget = GTK_WIDGET(gobj());
	GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
	NSWindow *parent = _get_toplevel_nswindow(widget);
	if (!parent) return true;

	// Widget's top-left relative to the toplevel GdkWindow, which is the parent
	// NSWindow's content view. Letting AppKit convert that to screen coordinates
	// avoids mixing GDK root coordinates with NSScreen ones (they differ with
	// multiple monitors).
	int wx = 0, wy = 0;
	if (!gtk_widget_translate_coordinates(widget, toplevel, 0, 0, &wx, &wy))
		return true;

	NSView *content = [parent contentView];
	NSRect nsFrame = NSMakeRect(wx, wy, au_w, au_h);
	if (![content isFlipped])
		nsFrame.origin.y = [content bounds].size.height - wy - au_h;
	nsFrame = [parent convertRectToScreen:[content convertRect:nsFrame toView:nil]];

	if (!NSEqualRects([panel frame], nsFrame))
		[panel setFrame:nsFrame display:YES];

	if ([panel parentWindow] != parent) {
		[[panel parentWindow] removeChildWindow:panel];
		[parent addChildWindow:panel ordered:NSWindowAbove];
	}

	// Only bring to front on first show; after that respect the window stack.
	if (![panel isVisible])
		[panel orderFront:nil];

	return true; // keep timer alive
}

// ── EffectEditorAU ────────────────────────────────────────────────────────────

EffectEditorAU::EffectEditorAU(AudioEffectAU *p_au, EffectEditor *p_editor) :
		effect_editor_midi(p_au, p_editor), au_effect(p_au), placeholder(nullptr) {

	if (p_au->has_user_interface()) {
		placeholder = new EffectPlaceholderAU(p_au);
		placeholder->set_size_request(400, 300); // initial minimum
		effect_editor_midi.prepend_page(*placeholder, "AU Plugin");
	}

	pack_start(effect_editor_midi, Gtk::PACK_EXPAND_WIDGET);
	show_all_children();
}

EffectEditorAU::~EffectEditorAU() {
	delete placeholder;
}

// ── factory helpers ───────────────────────────────────────────────────────────

static Gtk::Widget *create_au_editor(AudioEffect *p_effect, EffectEditor *p_editor) {
	if (!AudioEffectProviderAU::singleton) return nullptr;
	if (p_effect->get_provider_id() != AudioEffectProviderAU::singleton->get_id())
		return nullptr;
	return new EffectEditorAU(static_cast<AudioEffectAU *>(p_effect), p_editor);
}

EffectEditorPluginFunc get_au_editor_function() {
	return &create_au_editor;
}

#endif // OSX_ENABLED
