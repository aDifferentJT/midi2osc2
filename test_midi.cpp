#include "midi.hpp"
#include <stdlib.h>

int main() {
  Midi midi("MIDI Mix", [](Midi::Event event){
      std::cout << event.control << ":" << event.value << std::endl;
      }, "akai_midimix.profile");
  for(;;){}
}

