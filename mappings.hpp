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
    using Mapping = std::unordered_map<std::string, std::function<void(float)>>;
    std::vector<std::string> filenames;
    std::unordered_map<std::string, Output*> outputs;
    std::vector<Mapping> mappings;
    int currentMappingIndex = 0;
    Mapping& currentMapping() { return mappings[currentMappingIndex]; }
  public:
    Mappings(std::vector<std::string> filenames, std::unordered_map<std::string, Output*> outputs);
    void write();
    std::function<void(Midi::Event)> respond = [this](Midi::Event event) {
      try {
        currentMapping().at(event.control)(event.value);
      } catch (std::out_of_range) {}
    };
};

#endif

