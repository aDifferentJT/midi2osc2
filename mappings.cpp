#include "mappings.hpp"
#include <iostream>

Mappings::Mappings(std::vector<std::string> filenames, std::unordered_map<std::string, Output*> outputs)
  : filenames(filenames)
    , outputs(outputs)
{
  std::transform(filenames.begin(), filenames.end(), std::back_inserter(mappings),
      [this](std::string filename) -> Mapping {
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
      mapping[control] = [this, output, path](float v) { this->outputs[output]->send(path, v); };
      }
      return mapping;
      });
}

void Mappings::write() {

}

