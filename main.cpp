#include "mappings.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"

int main(int argc, char* argv[]) {
  std::unordered_map<std::string, Output*> outputs;

  OSC::init();
  OSC qlc("127.0.0.1", 7700, 9000, [](OSC::Message message){});
  outputs["QLC+"] = &qlc;

  Mappings mappings(std::vector({(std::string)"test.mapping"}), outputs);

  Midi midi("MIDI Mix", mappings.respond, "akai_midimix.profile");

  std::promise<void>().get_future().wait();
}

