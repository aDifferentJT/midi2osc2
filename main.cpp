#include "mappings.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"

int main(int argc, char* argv[]) {
  std::unordered_map<std::string, Output*> outputs;

  Midi midi("MIDI Mix", "akai_midimix.profile");

  OSC qlc("127.0.0.1", 7700, 9000);
  OSC::init();
  outputs["QLC+"] = &qlc;

  Mappings mappings({(std::string)"test.mapping"}, &midi, outputs);

  std::promise<void>().get_future().wait();
}

