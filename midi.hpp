#ifndef MIDI_h
#define MIDI_h

#include <rtmidi/RtMidi.h>

#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

class Midi {
  public:
    struct Event {
      std::string control;
      float value;
      Event(std::string control, std::variant<bool, uint8_t> value);
    };
  private:
    enum class ControlType { Button, Fader };
    struct Control {
      ControlType type;
      uint8_t number;
    };
    struct MidiEvent {
      Control control;
      std::variant<bool, uint8_t> value;
      MidiEvent(std::vector<unsigned char> message);
    };
    struct Profile {
      private:
        std::unordered_map<int, std::string> mapControlToString;
        std::unordered_map<std::string, int> mapStringToControl;
        std::unordered_map<std::string, std::string> feedbackCatchMap;
      public:
        Profile(std::string filename);
        std::string stringFromControl(Control);
        Control controlFromString(std::string);
        std::string feedbackCatch(std::string);
    };
    RtMidiIn rtMidiIn;
    RtMidiOut rtMidiOut;
    unsigned int portNumber;
    std::function<void(Event)> recvCallback;
    std::unordered_map<uint8_t, bool> buttonStates;
    Profile profile;
  public:
    Midi(std::string deviceName, std::function<void(Event)> recvCallback, std::string profileFilename);
    void setLed(uint8_t number, bool value);
};

#endif

