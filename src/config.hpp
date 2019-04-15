#ifndef Config_h
#define Config_h

#include <algorithm>      // for find_if
#include <memory>         // for shared_ptr, unique_ptr
#include <set>            // for set
#include <string>         // for string, operator==
#include <unordered_map>  // for unordered_map
#include <utility>        // for move
#include <vector>         // for vector<>::const_iterator, vector
#include "output.hpp" // IWYU pragma: keep
class Midi;
namespace asio { class io_context; }

class Config {
  public:
    struct Group {
      std::string name;
      std::set<std::string> controls;
      Group(std::string name, std::set<std::string> controls) : name(std::move(name)), controls(std::move(controls)) {}
    };
    std::unordered_map<std::string, std::unique_ptr<Output>> outputs;
    std::shared_ptr<Midi> midi;
    std::vector<std::string> banks;
    std::string bankLeft;
    std::string bankRight;
    std::vector<Group> channelGroups;
    std::vector<Group> actionGroups;

    Config(asio::io_context& io_context, const std::string& filename);

    std::string channelForControl(const std::string& control) const {
      return std::find_if(channelGroups.begin(), channelGroups.end(), [control](Group g) { return g.controls.count(control) > 0; })->name;
    }
    std::string actionForControl(const std::string& control) const {
      return std::find_if(actionGroups.begin(), actionGroups.end(), [control](Group g) { return g.controls.count(control) > 0; })->name;
    }

    std::set<std::string> controlsForChannel(const std::string& channel) const {
      return std::find_if(channelGroups.begin(), channelGroups.end(), [channel](Group g) { return g.name == channel; })->controls;
    }
    std::set<std::string> controlsForAction(const std::string& action) const {
      return std::find_if(actionGroups.begin(), actionGroups.end(), [action](Group g) { return g.name == action; })->controls;
    }
};

#endif
