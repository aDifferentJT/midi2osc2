#ifndef Midi_h
#define Midi_h

#include <bits/stdint-uintn.h>  // for uint8_t
#include <rtmidi/RtMidi.h>      // for RtMidiIn, RtMidiOut
#include <functional>           // for function
#include <optional>             // for optional
#include <string>               // for string
#include <unordered_map>        // for unordered_map
#include <utility>              // for move
#include <variant>              // for variant
#include <vector>               // for vector
class Config;

class Midi {
  public:
    struct Event {
      std::string control;
      float value;
    };
  private:
    struct MidiControl {
      enum class Type { Button, Fader };
      Type type = Type::Button;
      uint8_t number = 0;
    };
    struct MidiEvent {
      MidiControl control;
      std::variant<bool, uint8_t> value;
      explicit MidiEvent(std::vector<unsigned char> message);
    };
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
    };
    struct FaderState {
      std::optional<float> moved;
      std::optional<float> received;
    };

    RtMidiIn rtMidiIn;
    RtMidiOut rtMidiOut;
    unsigned int portNumber;
    std::function<void(Event)> callback = [](Event e){ (void)e; };
    std::unordered_map<uint8_t, bool> buttonStates;
    std::unordered_map<uint8_t, FaderState> faderStates;
    Profile profile;

    Config& config;

    void setLed(uint8_t number, bool value);

    static void midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData);
  public:
    const bool isMock;

    Midi(const std::string& deviceName, const std::string& profileFilename, Config& config);
    void feedback(const std::string& controlS, float v);
    void setCallback(std::function<void(Event)> f) { callback = std::move(f); }

    void setLed(const std::string& control, bool value) { setLed(profile.controlFromString(control).number, value); }

    void recvMockMoved(Event e) { if (!isMock) { throw; } callback(std::move(e)); }
};

#endif

