#ifndef MidiCore_h
#define MidiCore_h

//#include <bits/stdint-uintn.h>  // for uint8_t
#include <cstdint>  // PRAGMA IWYU: kepp
#include <variant>              // for variant
#include <vector>               // for vector

struct MidiControl {
  enum class Type { Button, Fader };
  Type type = Type::Button;
  uint8_t number = 0;
};
struct MidiEvent {
  MidiControl control;
  std::variant<bool, uint8_t> value;
  explicit MidiEvent(std::vector<unsigned char> message) {
    switch (message[0]) {
      case 0x90:
        control.type = MidiControl::Type::Button;
        control.number = message[1];
        value = true;
        break;
      case 0x80:
        control.type = MidiControl::Type::Button;
        control.number = message[1];
        value = false;
        break;
      case 0xB0:
        control.type = MidiControl::Type::Fader;
        control.number = message[1];
        if (message.size() == 2) {
          value = static_cast<uint8_t>(0);
        } else {
          value = message[2];
        }
        break;
    }
  }
};

#endif

