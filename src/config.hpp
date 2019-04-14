#ifndef Config_h
#define Config_h

#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Config {
  public:
    struct Group {
      std::string name;
      std::set<std::string> controls;
      Group(std::string name, std::set<std::string> controls) : name(std::move(name)), controls(std::move(controls)) {}
    };
    std::unordered_map<std::string, Output*> outputs;
    //std::optional<Midi> midi;
    Midi* midi;
    std::vector<std::string> banks;
    std::string bankLeft;
    std::string bankRight;
    std::vector<Group> channelGroups;
    std::vector<Group> actionGroups;

    Config(asio::io_context& io_context, std::string filename);

    std::string channelForControl(const std::string& control) const;
    std::string actionForControl(const std::string& control) const;
};

#endif
