#include "midi.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Midi::Event::Event(std::string control, std::variant<bool, uint8_t> value)
  : control(control)
    , value(std::visit(overloaded
          { [](bool v) { return v ? 1.0 : 0.0; }
          , [](uint8_t v) { return (float)v / 127.0; }
          }, value))
{}

Midi::MidiEvent::MidiEvent(std::vector<unsigned char> message) {
  switch (message[0]) {
    case 0x90:
      control.type = ControlType::Button;
      control.number = message[1];
      value = true;
      break;
    case 0x80:
      control.type = ControlType::Button;
      control.number = message[1];
      value = false;
      break;
    case 0xB0:
      control.type = ControlType::Fader;
      control.number = message[1];
      if (message.size() == 2) {
        value = (uint8_t)0;
      } else {
        value = message[2];
      }
      break;
  }
}

Midi::Profile::Profile(std::string filename) {
  std::ifstream f(filename);
  while (!f.eof()) {
    Control c;
    switch (f.get()) {
      case 'B':
        c.type = ControlType::Button;
        break;
      case 'F':
        c.type = ControlType::Fader;
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
    c.number = (uint8_t)n;
    int cEnc = ((int)c.type << 8) | c.number;
    assert(f.get() == ':');
    std::string str;
    std::getline(f, str);
    size_t feedbackCatchInd = str.find(":");
    std::string name = str.substr(0, feedbackCatchInd);
    mapControlToString[cEnc] = name;
    mapStringToControl[name] = cEnc;
    if (feedbackCatchInd != std::string::npos) {
      feedbackCatchMap[name] = str.substr(feedbackCatchInd + 1, std::string::npos);
    }
  }
}

std::string Midi::Profile::stringFromControl(Control c) {
  return mapControlToString[((int)c.type << 8) | c.number];
}
Midi::Control Midi::Profile::controlFromString(std::string str) {
  int cEnc = mapStringToControl[str];
  return {(Midi::ControlType)(cEnc >> 8), (uint8_t)(cEnc & 0xFF)};
}
std::string Midi::Profile::feedbackCatch(std::string str) {
  return feedbackCatchMap[str];
}

Midi::Midi(std::string deviceName, std::function<void(Event)> recvCallback, std::string profileFilename)
  : rtMidiIn(RtMidi::UNSPECIFIED, "midi2osc2")
  , rtMidiOut(RtMidi::UNSPECIFIED, "midi2osc2")
  , recvCallback(recvCallback)
    , profile(profileFilename)
{
  portNumber = -1;
  for (int i = 0; i < rtMidiIn.getPortCount(); i++) {
    std::string portName = rtMidiIn.getPortName(i);
    if (std::mismatch(deviceName.begin(), deviceName.end(), portName.begin(), portName.end()).first == deviceName.end()) {
      portNumber = i;
      break;
    }
  }
  if (portNumber < 0) {
    std::cerr << "Midi device not found" << std::endl;
    throw;
  }
  rtMidiIn.openPort(portNumber);
  rtMidiOut.openPort(portNumber);
  rtMidiIn.setCallback([](double timeStamp, std::vector<unsigned char>* message, void* userData){
      Midi* midi = (Midi*)userData;
      MidiEvent event(*message);
      if (event.control.type == ControlType::Button) {
      if (std::get<bool>(event.value) == true) {
      try {
      midi->buttonStates[event.control.number] = !midi->buttonStates.at(event.control.number);
      } catch (std::out_of_range) {
      midi->buttonStates[event.control.number] = true;
      }
      }
      event.value = midi->buttonStates[event.control.number];
      midi->setLed(event.control.number, std::get<bool>(event.value));
      }
      std::string control = midi->profile.stringFromControl(event.control);
      midi->recvCallback(Event(control, event.value));
      }, this);
}

void Midi::setLed(uint8_t number, bool value) {
  std::vector<unsigned char> message({0x90, number, value ? (uint8_t)0x7F : (uint8_t)0x00});
  rtMidiOut.sendMessage(&message);
}

