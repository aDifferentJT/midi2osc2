#include "midi.hpp"
#include <algorithm>   // for mismatch, copy
#include <chrono>      // for milliseconds
#include <cmath>       // for fabsf
#include <cstdint>     // for uint8_t
#include <iostream>    // for cerr, cout
#include <stdexcept>   // for out_of_range
#include <thread>      // for sleep_for
#include <variant>     // for get
#include "config.hpp"  // for Config
#include "gui.hpp"     // for GUI
#include <fstream> // IWYU pragma: keep


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Midi::Profile::Profile(const std::string& filename) {
  std::ifstream f(filename);
  while (!f.eof()) {
    MidiControl::Type type;
    uint8_t number;
    switch (f.get()) {
      case 'B':
        type = MidiControl::Type::Button;
        break;
      case 'F':
        type = MidiControl::Type::Fader;
        break;
      case std::char_traits<char>::eof():
        return;
      default:
        f.unget();
        std::cerr << "Unrecognised control: " << f.get() << std::endl;
        throw;
    }
    int n;
    f >> n;
    number = static_cast<uint8_t>(n);
    int cEnc = (static_cast<int>(type) << 8) | number;
    if (f.get() != ':') { throw; }
    std::string str;
    std::getline(f, str);
    std::size_t catchIndicatorInd = str.find(':');
    std::string name = str.substr(0, catchIndicatorInd);
    mapMidiControlToString[cEnc] = name;
    mapStringToMidiControl[name] = cEnc;
    if (catchIndicatorInd != std::string::npos) {
      catchIndicatorMap[name] = str.substr(catchIndicatorInd + 1, std::string::npos);
    }
  }
}

std::string Midi::Profile::stringFromMidiControl(MidiControl c) {
  return mapMidiControlToString.at((static_cast<int>(c.type) << 8) | c.number);
}
MidiControl Midi::Profile::controlFromString(const std::string& str) {
  int cEnc = mapStringToMidiControl.at(str);
  return {static_cast<MidiControl::Type>(cEnc >> 8), static_cast<uint8_t>(cEnc & 0xFF)};
}
std::optional<MidiControl> Midi::Profile::catchIndicator(MidiControl c) {
  try {
    return controlFromString(catchIndicatorMap.at(stringFromMidiControl(c)));
  } catch (std::out_of_range&) {
    return std::nullopt;
  }
}

std::set<uint8_t> Midi::Profile::allLeds() {
  std::set<uint8_t> s;
  for (auto& x : mapMidiControlToString) {
    int cEnc = x.first;
    if (static_cast<MidiControl::Type>(cEnc >> 8) == MidiControl::Type::Button) {
      s.insert(static_cast<uint8_t>(cEnc & 0xFF));
    }
  }
  return s;
}

void Midi::setLed(uint8_t number, bool value) {
  if (isMock) {
    config.gui.send("mockSetLed:" + profile.stringFromMidiControl({MidiControl::Type::Button, number}) + ":" + (value ? "true" : "false"));
  } else {
    std::vector<unsigned char> message({0x90, number, value ? static_cast<uint8_t>(0x7F) : static_cast<uint8_t>(0x00)});
    rtMidiOut.sendMessage(&message);
  }
}

template <typename T>
bool inBounds(T a, T b, T c) {
  return (a <= b && b <= c) || (a >= b && b >= c);
}

void Midi::midiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData) {
  std::cout << "Callback" << std::endl;
  (void)timeStamp;
  Midi* midi = static_cast<Midi*>(userData);
  MidiEvent event(*message);
  std::string control = midi->profile.stringFromMidiControl(event.control);
  float value;
  switch (event.control.type) {
    case MidiControl::Type::Button:
      if (control == midi->config.bankLeft || control == midi->config.bankRight) {
        value = std::get<bool>(event.value) ? 1.0 : 0.0;
      } else {
        if (std::get<bool>(event.value)) {
          try {
            midi->buttonStates[event.control.number] = !midi->buttonStates.at(event.control.number);
          } catch (std::out_of_range&) {
            midi->buttonStates[event.control.number] = true;
          }
        }
        value = midi->buttonStates[event.control.number] ? 1.0 : 0.0;
        midi->setLed(event.control.number, value >= 0.5);
      }
      midi->callback({control, value});
      break;
    case MidiControl::Type::Fader:
      value = static_cast<float>(std::get<uint8_t>(event.value)) / 127.0;
      try {
        FaderState state = midi->faderStates.at(event.control.number);
        if (state.received
            && !((state.moved && inBounds(value, *state.received, *state.moved))
                 || fabsf(value - *state.received) < 0.01)) {
          midi->faderStates[event.control.number] = {value, state.received};
        } else {
          midi->faderStates[event.control.number] = {value, std::nullopt};
        }
      } catch (std::out_of_range&) {
        midi->faderStates[event.control.number] = {value, std::nullopt};
      }
      if (std::optional<MidiControl> indicator = midi->profile.catchIndicator(event.control)) {
        midi->setLed(indicator->number, midi->faderStates.at(event.control.number).received.has_value());
      }
      if (!midi->faderStates.at(event.control.number).received.has_value()) {
        midi->callback({control, value});
      }
      break;
  }
}

Midi::Midi(const std::string& deviceName, const std::string& profileFilename, Config& config)
  : rtMidiIn(RtMidi::UNSPECIFIED, "midi2osc2")
  , rtMidiOut(RtMidi::UNSPECIFIED, "midi2osc2")
  , profile(profileFilename)
    , config(config)
    , isMock(deviceName == "mock")
{
  if (!isMock) {
    portNumber = -1;
    for (unsigned int i = 0; i < rtMidiIn.getPortCount(); i++) {
      std::string portName = rtMidiIn.getPortName(i);
      if (std::mismatch(deviceName.begin(), deviceName.end(), portName.begin(), portName.end()).first == deviceName.end()) {
        portNumber = i;
        break;
      }
    }
    if (portNumber == (unsigned int)-1) {
      std::cerr << "Midi device not found" << std::endl;
      throw;
    }
    rtMidiIn.openPort(portNumber);
    rtMidiOut.openPort(portNumber);
    for (uint8_t led : profile.allLeds()) {
      setLed(led, true);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    for (uint8_t led : profile.allLeds()) {
      setLed(led, false);
    }
    rtMidiIn.setCallback(&Midi::midiCallback, this);
  }
}

Midi::~Midi() {
  for (uint8_t led : profile.allLeds()) {
    setLed(led, false);
  }
}

void Midi::feedback(const std::string& controlS, float v) {
  MidiControl control = profile.controlFromString(controlS);
  switch (control.type) {
    case MidiControl::Type::Button:
      setLed(control.number, v >= 0.5);
      break;
    case MidiControl::Type::Fader:
      try {
        FaderState state = faderStates.at(control.number);
        if (state.moved && std::fabs(v - *state.moved) < 0.01) {
          faderStates[control.number] = {state.moved, std::nullopt};
        } else {
          faderStates[control.number] = {state.moved, v};
        }
      } catch (std::out_of_range&) {
        faderStates[control.number] = {std::nullopt, v};
      }
      if (std::optional<MidiControl> indicator = profile.catchIndicator(control)) {
        setLed(indicator->number, faderStates.at(control.number).received.has_value());
      }
      break;
  }
}

