#include "config.hpp"

Config::Config(asio::io_context& io_context, std::string filename) {
  std::ifstream f(filename);
  while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
    std::string str;
    std::getline(f, str);
    size_t typeEnd = str.find(':');
    std::string type = str.substr(0, typeEnd);
    if (type == "osc") {
      size_t nameStart = typeEnd + 1;
      size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      size_t addressStart = nameEnd + 1;
      size_t addressEnd = str.find(':', addressStart);
      std::string address = str.substr(addressStart, addressEnd - addressStart);
      size_t outPortStart = addressEnd + 1;
      size_t outPortEnd = str.find(':', outPortStart);
      unsigned short outPort = std::stoi(str.substr(outPortStart, outPortEnd - outPortStart));
      size_t inPortStart = outPortEnd + 1;
      size_t inPortEnd = str.find(':', inPortStart);
      unsigned short inPort = std::stoi(str.substr(inPortStart, inPortEnd - inPortStart));
      outputs.emplace(name, new OSC(io_context, address, outPort, inPort));
      if (inPortEnd != std::string::npos) {
        outputs.at(name)->send(str.substr(inPortEnd + 1));
      }
    } else if (type == "midi") {
      size_t nameStart = typeEnd + 1;
      size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      size_t profileStart = nameEnd + 1;
      size_t profileEnd = str.find(':', profileStart);
      std::string profile = str.substr(profileStart, profileEnd - profileStart);
      midi = new Midi(name, profile, *this);
    } else if (type == "bank") {
      banks.push_back(str.substr(typeEnd + 1));
    } else if (type == "bankLeft") {
      bankLeft = str.substr(typeEnd + 1);
    } else if (type == "bankRight") {
      bankRight = str.substr(typeEnd + 1);
    } else if (type == "channelGroup") {
      size_t nameStart = typeEnd + 1;
      size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::set<std::string> controls;
      size_t controlEnd = nameEnd;
      size_t controlStart;
      do {
        controlStart = controlEnd + 1;
        controlEnd = str.find(',', controlStart);
        controls.insert(str.substr(controlStart, controlEnd - controlStart));
      } while (controlEnd != std::string::npos);
      channelGroups.emplace_back(name, controls);
    } else if (type == "actionGroup") {
      size_t nameStart = typeEnd + 1;
      size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::set<std::string> controls;
      size_t controlEnd = nameEnd;
      size_t controlStart;
      do {
        controlStart = controlEnd + 1;
        controlEnd = str.find(',', controlStart);
        controls.insert(str.substr(controlStart, controlEnd - controlStart));
      } while (controlEnd != std::string::npos);
      actionGroups.emplace_back(name, controls);
    }
  }
}

std::string Config::channelForControl(const std::string& control) const {
  return (*std::find_if(channelGroups.begin(), channelGroups.end(), [control](Group g) { return g.controls.count(control) > 0; })).name;
}

std::string Config::actionForControl(const std::string& control) const {
  return (*std::find_if(actionGroups.begin(), actionGroups.end(), [control](Group g) { return g.controls.count(control) > 0; })).name;
}

