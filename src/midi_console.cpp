//#include <rtmidi/RtMidi.h>  // for RtMidiIn, RtMidi, RtMidi::UNSPECIFIED
#include <RtMidi.h>  // for RtMidiIn, RtMidi, RtMidi::UNSPECIFIED
#include <csignal>          // for signal, SIGINT
#include <future>           // for promise, future
#include <iomanip>          // for operator<<, setfill, setw
#include <iostream>         // for operator<<, cout, ostream, basic_ostream
#include <sstream>          // for operator<<, stringstream
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
      std::stringstream s;
      switch (event.control.type) {
        case MidiControl::Type::Button:
          s << "B";
          break;
        case MidiControl::Type::Fader:
          s << "F";
          break;
      }
      s << std::setfill('0') << std::setw(3) << static_cast<int>(event.control.number);
      std::cout << "\x1B[1m";
      std::cout << "\x1B[2K\r";
      std::cout << "\x1B#3" << s.str();
      std::cout << "\n";
      std::cout << "\x1B#4" << s.str();
      std::cout << "\x1B[1A";
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

    ~MidiConsole() {
      rtMidiIn.cancelCallback();
      std::cout << "\x1B""c";
      std::cout.flush();
    }
};

std::promise<void> p;

extern "C" void stop(int sig) {
  (void)sig;
  p.set_value();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  std::signal(SIGINT, &stop);

  MidiConsole midiConsole;

  p.get_future().wait();
}

