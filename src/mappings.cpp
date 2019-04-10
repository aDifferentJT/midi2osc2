#include "mappings.hpp"
#include <iostream>

Mappings::Mappings(std::vector<std::string> filenames, Midi* midi, std::unordered_map<std::string, Output*> outputs)
  : filenames(filenames)
    , outputs(outputs)
{
  midi->setCallback([this, outputs](Midi::Event event) {
    try {
      Channel channel = currentMapping().channels.at(event.control);
      outputs.at(channel.output)->send(channel.path, event.value);
    } catch (std::out_of_range&) {}
  });
  for (std::pair<std::string, Output*> output : outputs) {
    output.second->setCallback([this, midi](std::string path, float v) {
      try {
        std::string control = currentMapping().feedbacks.at(path);
        midi->feedback(control, v);
      } catch (std::out_of_range&) {}
    });
  }
  std::transform(filenames.begin(), filenames.end(), std::back_inserter(mappings),
                 [](std::string filename) -> Mapping {
                   Mapping mapping;
                   std::ifstream f(filename);
                   while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
                     std::string line;
                     std::getline(f, line);
                     size_t controlStart = 0;
                     size_t controlEnd = line.find(':', controlStart);
                     std::string control = line.substr(controlStart, controlEnd - controlStart);
                     size_t outputStart = controlEnd + 1;
                     size_t outputEnd = line.find(':', outputStart);
                     std::string output = line.substr(outputStart, outputEnd - outputStart);
                     size_t pathStart = outputEnd + 1;
                     size_t pathEnd = line.find(':', pathStart);
                     std::string path = line.substr(pathStart, pathEnd - pathStart);
                     mapping.channels[control] = {output, path};
                     mapping.feedbacks[path] = control;
                   }
                   return mapping;
                 });
}

void Mappings::write() {
  for (const std::string& filename : filenames) {
    std::cout << filename << std::endl;
  }
}

