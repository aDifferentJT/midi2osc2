#include "osc.hpp"
#include <stdlib.h>

int main() {
  OSC osc("127.0.0.1", 7700, 9000, [](OSC::Message message){});
  OSC::init();
  for(float i = 0.0;; i += 0.01) {
    osc.send("/1",i);
    std::cout << i << std::endl;
    usleep(10000);
    if (i >= 1.0) {
      i = 0.0;
    }
  }
}

