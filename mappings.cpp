#include "mappings.hpp"
#include <iostream>

Mappings::Mappings(std::vector<std::string> filenames, Midi* midi, std::unordered_map<std::string, Output*> outputs)
  : filenames(filenames)
    , midi(midi)
    , outputs(outputs)
{
  midi->setCallback(respond);
  for (std::pair<std::string, Output*> output : outputs) {
    output.second->setCallback(feedback);
  }
  std::transform(filenames.begin(), filenames.end(), std::back_inserter(mappings),
      [](std::string filename) -> Mapping {
      Mapping mapping;
      std::ifstream f(filename);
      while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
      std::string line;
      std::getline(f, line);
      size_t controlStart = 0;
      size_t controlEnd = line.find(":", controlStart);
      std::string control = line.substr(controlStart, controlEnd - controlStart);
      size_t outputStart = controlEnd + 1;
      size_t outputEnd = line.find(":", outputStart);
      std::string output = line.substr(outputStart, outputEnd - outputStart);
      size_t pathStart = outputEnd + 1;
      size_t pathEnd = line.find(":", pathStart);
      std::string path = line.substr(pathStart, pathEnd - pathStart);
      mapping.channels[control] = {output, path};
      mapping.feedbacks[path] = control;
      }
      return mapping;
      });
}

void Mappings::write() {

}

