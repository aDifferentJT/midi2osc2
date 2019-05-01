#ifndef Midi_h
#define Midi_h

// IWYU pragma: no_include <bits/stdint-uintn.h>
#include <cstdint> // IWYU pragma: keep

#include <rtmidi/RtMidi.h>      // for RtMidiIn, RtMidiOut
#include <functional>           // for function
#include <optional>             // for optional
#include <set>                  // for set
#include <string>               // for string
#include <unordered_map>        // for unordered_map
#include <utility>              // for move
#include <vector>               // for vector
#include "midi_core.hpp"        // for MidiControl
class Config;

class Midi {
  public:
    struct Event {
      std::string control;
      float value;
    };
  private:
    struct Profile {
      private:
        std::unordered_map<int, std::string> mapMidiControlToString;
        std::unordered_map<std::string, int> mapStringToMidiControl;
        std::unordered_map<std::string, std::string> catchIndicatorMap;
      public:
        explicit Profile(const std::string& filename);
        std::string stringFromMidiControl(MidiControl c);
        MidiControl controlFromString(const std::string& str);
        std::optional<MidiControl> catchIndicator(MidiControl c);
        std::set<uint8_t> allLeds();
    };
    struct FaderState {
      std::optional<float> moved;
      std::optional<float> received;
    };

    RtMidiIn rtMidiIn;
    RtMidiOut rtMidiOut;
    unsigned int portNumber;
    std::function<void(Event)> callback = [](Event e){ (void)e; };
    std::function<void(std::string, bool)> mockCallback = [](std::string led, bool value){ (void)led; (void)value; };
    std::unordered_map<uint8_t, bool> buttonStates;
    std::unordered_map<uint8_t, FaderState> faderStates;
    Profile profile;

  public:
    Config* config; // TODO this is never initialised

    void setLed(uint8_t number, bool value);

    static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData);
  public:
    const bool isMock;

    Midi(const std::string& deviceName, const std::string& profileFilename);
    Midi(const Midi&) = delete;
    Midi& operator =(const Midi&) = delete;
    Midi(Midi&& other) = default;
    Midi& operator =(Midi&&) = default;
    ~Midi();

    void feedback(const std::string& controlS, float v);
    void setCallback(std::function<void(Event)> f) { callback = std::move(f); }
    void setMockCallback(std::function<void(std::string, bool)> f) { mockCallback = std::move(f); }

    void setLed(const std::string& control, bool value) { setLed(profile.controlFromString(control).number, value); }

    void recvMockMoved(Event e) { if (!isMock) { throw; } callback(std::move(e)); }

    friend class MidiConsole;
};

#endif

