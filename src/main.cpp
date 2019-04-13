#include "mappings.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  asio::io_context io_context;

  std::unordered_map<std::string, Output*> outputs;

  Midi midi("MIDI Mix", "akai_midimix.profile");

  OSC qlc(io_context, "127.0.0.1", 7700, 9000);
  outputs["QLC+"] = &qlc;

  Mappings mappings({(std::string)"test.mapping"}, &midi, outputs);

  asio::executor_work_guard<asio::io_context::executor_type> work
    = asio::make_work_guard(io_context);
  io_context.run();
}

