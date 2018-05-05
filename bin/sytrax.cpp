#include <gtkmm.h>

#include "gui/interface.h"

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.examples.base");

  Interface window;
  window.set_default_size(800, 600);

  return app->run(window);
}
