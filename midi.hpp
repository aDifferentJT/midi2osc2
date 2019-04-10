#ifndef MIDI_h
#define MIDI_h

#include <rtmidi/RtMidi.h>

#include <cassert>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class Midi {
  public:
    struct Event {
      std::string control;
      float value;
      //Event(std::string control, std::variant<bool, uint8_t> value);
    };
  private:
    struct Control {
      enum class Type { Button, Fader };
      Type type;
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
        std::unordered_map<std::string, std::string> catchIndicatorMap;
      public:
        Profile(std::string filename);
        std::string stringFromControl(Control);
        Control controlFromString(std::string);
        std::optional<Control> catchIndicator(Control);
    };
    struct FaderState {
      std::optional<float> moved;
      std::optional<float> received;
    };
    RtMidiIn rtMidiIn;
    RtMidiOut rtMidiOut;
    unsigned int portNumber;
    std::function<void(Event)> callback = [](Event e){};
    std::unordered_map<uint8_t, bool> buttonStates;
    std::unordered_map<uint8_t, FaderState> faderStates;
    Profile profile;

    void setLed(uint8_t number, bool value);
  public:
    Midi(std::string deviceName, std::string profileFilename);
    void feedback(std::string control, float v);
    void setCallback(std::function<void(Event)> f) { callback = f; }
};

#endif

