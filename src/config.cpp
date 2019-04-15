#include "config.hpp"
#include <fstream>     // for size_t, ifstream, operator<<, basic_ostream, endl
#include <iostream>    // for cerr
#include "gui.hpp"     // for GUI
#include "midi.hpp"    // for Midi
#include "osc.hpp"     // for OSC
#include "output.hpp"  // for Output
namespace asio { class io_context; }
#include <fstream> // IWYU pragma: keep

Config::Config(asio::io_context& io_context, const std::string& filename) : gui(std::make_shared<GUI>(io_context)) {
  std::ifstream f(filename);
  while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
    std::string str;
    std::getline(f, str);
    if (str.empty() || str.substr(0, 2) == "//") {
      continue;
    }
    std::size_t typeEnd = str.find(':');
    std::string type = str.substr(0, typeEnd);
    if (type == "osc") {
      std::size_t nameStart = typeEnd + 1;
      std::size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::size_t addressStart = nameEnd + 1;
      std::size_t addressEnd = str.find(':', addressStart);
      std::string address = str.substr(addressStart, addressEnd - addressStart);
      std::size_t outPortStart = addressEnd + 1;
      std::size_t outPortEnd = str.find(':', outPortStart);
      unsigned short outPort = std::stoi(str.substr(outPortStart, outPortEnd - outPortStart));
      std::size_t inPortStart = outPortEnd + 1;
      std::size_t inPortEnd = str.find(':', inPortStart);
      unsigned short inPort = std::stoi(str.substr(inPortStart, inPortEnd - inPortStart));
      outputs[name] = std::make_unique<OSC>(io_context, address, outPort, inPort);
      if (inPortEnd != std::string::npos) {
        outputs.at(name)->send(str.substr(inPortEnd + 1));
      }
    } else if (type == "midi") {
      std::size_t nameStart = typeEnd + 1;
      std::size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::size_t profileStart = nameEnd + 1;
      std::size_t profileEnd = str.find(':', profileStart);
      std::string profile = str.substr(profileStart, profileEnd - profileStart);
      midi = std::make_shared<Midi>(name, profile, *this);
    } else if (type == "bank") {
      banks.push_back(str.substr(typeEnd + 1));
    } else if (type == "bankLeft") {
      bankLeft = str.substr(typeEnd + 1);
    } else if (type == "bankRight") {
      bankRight = str.substr(typeEnd + 1);
    } else if (type == "channelGroup") {
      std::size_t nameStart = typeEnd + 1;
      std::size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::set<std::string> controls;
      std::size_t controlEnd = nameEnd;
      std::size_t controlStart;
      do {
        controlStart = controlEnd + 1;
        controlEnd = str.find(',', controlStart);
        controls.insert(str.substr(controlStart, controlEnd - controlStart));
      } while (controlEnd != std::string::npos);
      channelGroups.emplace_back(name, controls);
    } else if (type == "actionGroup") {
      std::size_t nameStart = typeEnd + 1;
      std::size_t nameEnd = str.find(':', nameStart);
      std::string name = str.substr(nameStart, nameEnd - nameStart);
      std::set<std::string> controls;
      std::size_t controlEnd = nameEnd;
      std::size_t controlStart;
      do {
        controlStart = controlEnd + 1;
        controlEnd = str.find(',', controlStart);
        controls.insert(str.substr(controlStart, controlEnd - controlStart));
      } while (controlEnd != std::string::npos);
      actionGroups.emplace_back(name, controls);
    } else {
      std::cerr << "Cannot parse line: " << str << std::endl;
    }
  }
}

