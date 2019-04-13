#include "mappings.hpp"
#include <iostream>

Mappings::Mappings(std::vector<std::string> filenames, Midi* midi, std::unordered_map<std::string, Output*> outputs, GUI* gui)
  : outputs(outputs)
    , gui(gui)
{
  midi->setCallback([this](Midi::Event event) {
    try {
      Channel channel = currentMapping().channels.at(event.control);
      this->outputs.at(channel.output)->send(channel.path, event.value);
      this->gui->send("moved:" + event.control + ":" + std::to_string(event.value) + ":" + channel.output + ":" + channel.path);
    } catch (std::out_of_range&) {
      this->gui->send("moved:" + event.control + ":" + std::to_string(event.value) + "::");
    }
  });
  for (std::pair<std::string, Output*> output : outputs) {
    output.second->setCallback([this, midi](std::string path, float v) {
      try {
        std::string control = currentMapping().feedbacks.at(path);
        midi->feedback(control, v);
      } catch (std::out_of_range&) {}
    });
  }
  gui->setCallback([this](std::string str) {
    std::cout << str << std::endl;
    size_t commandStart = 0;
    size_t commandEnd = str.find(':', commandStart);
    std::string command = str.substr(commandStart, commandEnd - commandStart);
    if (command == "setControl") {
      size_t controlStart = commandEnd + 1;
      size_t controlEnd = str.find(':', controlStart);
      std::string control = str.substr(controlStart, controlEnd - controlStart);
      size_t outputStart = controlEnd + 1;
      size_t outputEnd = str.find(':', outputStart);
      std::string output = str.substr(outputStart, outputEnd - outputStart);
      size_t pathStart = outputEnd + 1;
      size_t pathEnd = str.find(':', pathStart);
      std::string path = str.substr(pathStart, pathEnd - pathStart);
      this->currentMapping().channels[control] = {output, path};
      this->currentMapping().feedbacks[path] = control;
    }
    this->write();
  });
  std::transform(filenames.begin(), filenames.end(), std::back_inserter(mappings),
                 [](std::string filename) -> Mapping {
                   Mapping mapping;
                   mapping.filename = filename;
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
  for (const Mapping& mapping : mappings) {
    std::ofstream f(mapping.filename);
    for (std::pair<std::string, Channel> channel : mapping.channels) {
      f << channel.first << ":" << channel.second.output << ":" << channel.second.path << "\n";
    }
    f.close();
  }
}

