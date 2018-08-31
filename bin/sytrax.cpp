#include <gtkmm.h>

#include "drivers/audio_effect_factory_lv2.h"
#include "gui/interface.h"

int main(int argc, char *argv[]) {

	AudioEffectFactory effect_factory;

	AudioEffectProviderLV2 provider_lv2(&argc, &argv); //lv2 madness
	provider_lv2.scan_effects(&effect_factory);

	effect_factory.add_provider(&provider_lv2);

	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.examples.base");

	Interface window(&effect_factory);
	window.set_default_size(800, 600);

	return app->run(window);
}
