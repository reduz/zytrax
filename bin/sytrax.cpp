#include <gtkmm.h>

#include "drivers/audio_effect_factory_lv2.h"
#include "drivers/factory_wrapper_vst2.h"
#include "gui/interface.h"

int main(int argc, char *argv[]) {

	AudioEffectFactory effect_factory;

	AudioEffectProvider *provider_vst2 = create_vst2_provider();
	provider_vst2->scan_effects(&effect_factory);
	effect_factory.add_provider(provider_vst2);

#ifdef UNIX_ENABLED

	AudioEffectProviderLV2 provider_lv2(&argc, &argv); //lv2 madness
	provider_lv2.scan_effects(&effect_factory);
	effect_factory.add_provider(&provider_lv2);
#endif

	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.examples.base");

	/* make it dark */
	/*g_object_set(gtk_settings_get_default(),
			"gtk-application-prefer-dark-theme", TRUE,
			NULL);*/

	Interface window(app.operator->(), &effect_factory);
	window.set_default_size(800, 600);

	int ret = app->run(window);

	delete provider_vst2;

	return ret;
}
