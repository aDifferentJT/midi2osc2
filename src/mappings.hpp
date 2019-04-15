#ifndef Mapping_h
#define Mapping_h

#include "config.hpp"
#include "gui.hpp"
#include "midi.hpp"
#include "output.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

class Mappings {
  private:
    struct Control {
      std::string output;
      std::string path;
      bool inverted;
      Control(std::string str, size_t start = 0);
      Control() = default;
      std::string encode() const { return output + ":" + path + ":" + (inverted ? "true" : "false"); }
    };
    struct Channel {
      std::string output;
      std::string channel;
      Channel(std::string str, size_t start = 0);
      Channel() = default;
      std::string encode() const { return output + ":" + channel; }
    };
    struct Action {
      std::string action;
      Action(std::string str, size_t start = 0);
      Action() = default;
      std::string encode() const { return action; }
    };
    struct Mapping {
      std::string filename;
      std::unordered_map<std::string, Control> controls;
      std::unordered_map<std::string, Channel> channels;
      std::unordered_map<std::string, Action> actions;
      std::unordered_map<std::string, std::string> feedbacks;
    };
    const Config& config;
    GUI* gui;
    std::vector<Mapping> mappings;
    size_t currentMappingIndex = 0;
    Mapping& currentMapping() { return mappings[currentMappingIndex]; }

    void refreshBankLeds();
  public:
    Mappings(const Config& config, GUI* gui);
//    Mappings(std::initializer_list<std::string> filenames, Midi* midi, const std::unordered_map<std::string, Output*>& outputs, GUI* gui)
//      : Mappings(std::vector(filenames), midi, outputs, gui) {}
    void write();
};

#endif

