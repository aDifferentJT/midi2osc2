#include "mappings.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"
#include "config.hpp"

#include <csignal>

asio::io_context io_context;

extern "C" void stop(int sig) {
  (void)sig;
  io_context.stop();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  //Midi midi("MIDI Mix", "akai_midimix.profile");

  /*
  OSC qlc(io_context, "127.0.0.1", 7700, 9000);
  outputs["QLC+"] = &qlc;
*/

  Config config(io_context, "midi2osc2.conf");

  GUI gui(io_context);

  Mappings mappings(config, &gui);

  std::signal(SIGINT, &stop);

  asio::executor_work_guard<asio::io_context::executor_type> work
    = asio::make_work_guard(io_context);
  io_context.run();
}

