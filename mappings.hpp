#ifndef Mapping_h
#define Mapping_h

#include "midi.hpp"
#include "output.hpp"

#include <fstream>
#include <functional>
//#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

class Mappings {
  private:
    struct Channel {
      std::string output;
      std::string path;
    };
    struct Mapping {
       std::unordered_map<std::string, Channel> channels;
       std::unordered_map<std::string, std::string> feedbacks;
    };
    std::vector<std::string> filenames;
    Midi* midi;
    std::unordered_map<std::string, Output*> outputs;
    std::vector<Mapping> mappings;
    int currentMappingIndex = 0;
    Mapping& currentMapping() { return mappings[currentMappingIndex]; }
  public:
    Mappings(std::vector<std::string> filenames, Midi* midi, std::unordered_map<std::string, Output*> outputs);
    Mappings(std::initializer_list<std::string> filenames, Midi* midi, std::unordered_map<std::string, Output*> outputs)
      : Mappings(std::vector(filenames), midi, outputs) {}
    void write();
    std::function<void(Midi::Event)> respond = [this](Midi::Event event) {
      try {
        Channel channel = currentMapping().channels.at(event.control);
        outputs[channel.output]->send(channel.path, event.value);
      } catch (std::out_of_range) {}
    };
    std::function<void(std::string, float)> feedback = [this](std::string path, float v) {
      try {
        std::string control = currentMapping().feedbacks.at(path);
        midi->feedback(control, v);
      } catch (std::out_of_range) {}
    };
};

#endif

