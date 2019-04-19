#include <rtmidi/RtMidi.h>  // for RtMidiIn, RtMidi, RtMidi::UNSPECIFIED
#include <future>           // for promise, future
#include <iomanip>          // for operator<<, setfill, setw
#include <iostream>         // for operator<<, cout, ostream, basic_ostream
#include <string>           // for allocator, operator<<
#include <vector>           // for vector
#include "midi_core.hpp"    // for MidiControl, MidiEvent, MidiControl::Type

class MidiConsole {
  private:
    RtMidiIn rtMidiIn;

    static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
      (void)timeStamp;
      (void)userData;
      MidiEvent event(*message);
      switch (event.control.type) {
        case MidiControl::Type::Button:
          std::cout << "B";
          break;
        case MidiControl::Type::Fader:
          std::cout << "F";
          break;
      }
      std::cout << std::setfill('0') << std::setw(3) << (int)event.control.number;
      std::cout << '\r';
      std::cout.flush();
    }
  public:
    MidiConsole()
      : rtMidiIn(RtMidi::UNSPECIFIED, "midi_console")
      {

      for (unsigned int i = 0; i < rtMidiIn.getPortCount(); i++) {
        std::cout << i << ":" << rtMidiIn.getPortName(i) << std::endl;
      }
      std::cout << "Select a controller: ";
      unsigned int portNumber;
      std::cin >> portNumber;

      rtMidiIn.openPort(portNumber);
      rtMidiIn.setCallback(&midiCallback);
    }
};

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  MidiConsole midiConsole;

  std::promise<void>().get_future().wait();
}

