#include "midi.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/*
   Midi::Event::Event(std::string control, std::variant<bool, uint8_t> value)
   : control(control)
   , value(std::visit(overloaded
   { [](bool v)    -> float { return v ? 1.0 : 0.0; }
   , [](uint8_t v) -> float { return (float)v / 127.0; }
   }, value))
   {}
   */

Midi::MidiEvent::MidiEvent(std::vector<unsigned char> message) {
  switch (message[0]) {
    case 0x90:
      control.type = Control::Type::Button;
      control.number = message[1];
      value = true;
      break;
    case 0x80:
      control.type = Control::Type::Button;
      control.number = message[1];
      value = false;
      break;
    case 0xB0:
      control.type = Control::Type::Fader;
      control.number = message[1];
      if (message.size() == 2) {
        value = (uint8_t)0;
      } else {
        value = message[2];
      }
      break;
  }
}

Midi::Profile::Profile(const std::string& filename) {
  std::ifstream f(filename);
  while (!f.eof()) {
    Control::Type type;
    uint8_t number;
    switch (f.get()) {
      case 'B':
        type = Control::Type::Button;
        break;
      case 'F':
        type = Control::Type::Fader;
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
    number = (uint8_t)n;
    int cEnc = (static_cast<int>(type) << 8) | number;
    if (f.get() != ':') { throw; }
    std::string str;
    std::getline(f, str);
    size_t catchIndicatorInd = str.find(':');
    std::string name = str.substr(0, catchIndicatorInd);
    mapControlToString[cEnc] = name;
    mapStringToControl[name] = cEnc;
    if (catchIndicatorInd != std::string::npos) {
      catchIndicatorMap[name] = str.substr(catchIndicatorInd + 1, std::string::npos);
    }
  }
}

std::string Midi::Profile::stringFromControl(Control c) {
  return mapControlToString.at(((int)c.type << 8) | c.number);
}
Midi::Control Midi::Profile::controlFromString(const std::string& str) {
  int cEnc = mapStringToControl.at(str);
  return {(Midi::Control::Type)(cEnc >> 8), (uint8_t)(cEnc & 0xFF)};
}
std::optional<Midi::Control> Midi::Profile::catchIndicator(Control c) {
  try {
    return controlFromString(catchIndicatorMap.at(stringFromControl(c)));
  } catch (std::out_of_range&) {
    return std::nullopt;
  }
}

void Midi::setLed(uint8_t number, bool value) {
  std::vector<unsigned char> message({0x90, number, value ? (uint8_t)0x7F : (uint8_t)0x00});
  rtMidiOut.sendMessage(&message);
}

template <typename T>
bool inBounds(T a, T b, T c) {
  return (a <= b && b <= c) || (a >= b && b >= c);
}

Midi::Midi(const std::string& deviceName, const std::string& profileFilename)
  : rtMidiIn(RtMidi::UNSPECIFIED, "midi2osc2")
  , rtMidiOut(RtMidi::UNSPECIFIED, "midi2osc2")
    , profile(profileFilename)
{
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
  rtMidiIn.setCallback([](double timeStamp, std::vector<unsigned char>* message, void* userData){
    (void)timeStamp;
    Midi* midi = static_cast<Midi*>(userData);
    MidiEvent event(*message);
    std::string control = midi->profile.stringFromControl(event.control);
    float value;
    switch (event.control.type) {
      case Control::Type::Button:
        if (std::get<bool>(event.value)) {
          try {
            midi->buttonStates[event.control.number] = !midi->buttonStates.at(event.control.number);
          } catch (std::out_of_range&) {
            midi->buttonStates[event.control.number] = true;
          }
        }
        value = midi->buttonStates[event.control.number] ? 1.0 : 0.0;
        midi->setLed(event.control.number, midi->buttonStates[event.control.number]);
        midi->callback({control, value});
        break;
      case Control::Type::Fader:
        value = (float)std::get<uint8_t>(event.value) / 127.0;
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
        if (std::optional<Control> indicator = midi->profile.catchIndicator(event.control)) {
          midi->setLed(indicator->number, midi->faderStates.at(event.control.number).received.has_value());
        }
        if (!midi->faderStates.at(event.control.number).received.has_value()) {
          midi->callback({control, value});
        }
        break;
    }
  }, this);
}

void Midi::feedback(const std::string& controlS, float v) {
  Control control = profile.controlFromString(controlS);
  switch (control.type) {
    case Control::Type::Button:
      setLed(control.number, v >= 0.5);
      break;
    case Control::Type::Fader:
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
      if (std::optional<Control> indicator = profile.catchIndicator(control)) {
        setLed(indicator->number, faderStates.at(control.number).received.has_value());
      }
      break;
  }
}
