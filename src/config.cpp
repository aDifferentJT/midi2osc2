#include "config.hpp"
#include <fstream>     // for size_t, ifstream, operator<<, basic_ostream, endl
#include <iostream>    // for cerr
#include <cctype>      // for isspace
#include "midi.hpp"    // for Midi
#include "osc.hpp"     // for OSC
#include "output.hpp"  // for Output
namespace asio { class io_context; }
#include <fstream> // IWYU pragma: keep

Config Config::parse(asio::io_context& io_context, const std::string& filename) {
  auto gui = std::make_unique<GUI>(io_context);
  std::unordered_map<std::string, std::unique_ptr<Output>> outputs;
  std::unique_ptr<Midi> midi;
  std::vector<std::string> banks;
  std::string bankLeft;
  std::string bankRight;
  std::vector<Group> channelGroups;
  std::vector<Group> actionGroups;

  std::ifstream f(filename);
  while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
    while (std::isspace(f.peek()) != 0) { f.ignore(); }
    std::string str;
    std::getline(f, str);
    if (str.empty() || str.substr(0, 2) == "//") {
      continue;
    }
    std::size_t typeEnd = str.find(':');
    std::string type = str.substr(0, typeEnd);
    if (type == "osc") {
      auto osc = parseOSC(io_context, str, typeEnd + 1);
      outputs[osc.first] = std::move(osc.second);
    } else if (type == "midi") {
      midi = parseMidi(str, typeEnd + 1);
    } else if (type == "bank") {
      banks.push_back(str.substr(typeEnd + 1));
    } else if (type == "bankLeft") {
      bankLeft = str.substr(typeEnd + 1);
    } else if (type == "bankRight") {
      bankRight = str.substr(typeEnd + 1);
    } else if (type == "channelGroup") {
      channelGroups.push_back(parseGroup(str, typeEnd + 1));
    } else if (type == "actionGroup") {
      actionGroups.push_back(parseGroup(str, typeEnd + 1));
    } else {
      std::cerr << "Cannot parse line: " << str << std::endl;
    }
  }
  if (!midi) {
    std::cerr << "No midi device given in config file" << std::endl;
    throw;
  }
  return Config(
    std::move(gui),
    std::move(outputs),
    std::move(midi),
    std::move(banks),
    std::move(bankLeft),
    std::move(bankRight),
    std::move(channelGroups),
    std::move(actionGroups)
    );
}

#define parseUnit(unit, str, start) std::size_t unit##Start = start; std::size_t unit##End = str.find(':', unit##Start); std::string unit = str.substr(unit##Start, unit##End - unit##Start)

std::pair<std::string, std::unique_ptr<Output>> Config::parseOSC(asio::io_context& io_context, std::string str, std::size_t start) {
  parseUnit(name, str, start);
  parseUnit(address, str, nameEnd + 1);
  parseUnit(outPort, str, addressEnd + 1);
  parseUnit(inPort, str, outPortEnd + 1);
  std::unique_ptr<OSC> osc = std::make_unique<OSC>(io_context, address, std::stoi(outPort), std::stoi(inPort));
  if (inPortEnd != std::string::npos) {
    osc->sendPeriodically(str.substr(inPortEnd + 1));
  }
  return std::make_pair(name, std::move(osc));
}

std::unique_ptr<Midi> Config::parseMidi(std::string str, std::size_t start) {
  parseUnit(name, str, start);
  parseUnit(profile, str, nameEnd + 1);
  return std::make_unique<Midi>(name, profile);
}

Config::Group Config::parseGroup(std::string str, std::size_t start) {
  parseUnit(name, str, start);
  std::set<std::string> controls;
  std::size_t controlEnd = nameEnd;
  std::size_t controlStart;
  do {
    controlStart = controlEnd + 1;
    controlEnd = str.find(',', controlStart);
    controls.insert(str.substr(controlStart, controlEnd - controlStart));
  } while (controlEnd != std::string::npos);
  return Group(name, controls);
}

