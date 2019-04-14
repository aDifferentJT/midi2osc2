#include "mappings.hpp"
#include <iostream>

Mappings::Mappings(const Config& config, GUI* gui)
  : config(config)
    , gui(gui)
{
  config.midi->setCallback([this](Midi::Event event) {
    if (event.control == this->config.bankLeft) {
      if (currentMappingIndex > 0) {
        currentMappingIndex -= 1;
        this->gui->send("bank:" + std::to_string(currentMappingIndex));
      }
    } else if (event.control == this->config.bankRight) {
      if (currentMappingIndex < mappings.size() - 1) {
        currentMappingIndex += 1;
        this->gui->send("bank:" + std::to_string(currentMappingIndex));
      }
    } else {
      try {
        Control control = currentMapping().controls.at(event.control);
        this->config.outputs.at(control.output)->send(control.path, control.inverted ? 1.0 - event.value : event.value);
        this->gui->send("moved:" + event.control + ":" + std::to_string(event.value) + ":" + control.output + ":" + control.path + ":" + (control.inverted ? "true" : "false"));
      } catch (std::out_of_range&) {
        this->gui->send("moved:" + event.control + ":" + std::to_string(event.value) + "::");
      }
    }
  });
  for (std::pair<std::string, Output*> output : config.outputs) {
    output.second->setCallback([this](std::string path, float v) {
      try {
        std::string control = this->currentMapping().feedbacks.at(path);
        this->config.midi->feedback(control, v);
      } catch (std::out_of_range&) {}
    });
  }
  gui->setOpenCallback([this]() {
    this->gui->send("bank:" + std::to_string(currentMappingIndex));
  });
  gui->setRecvCallback([this](std::string str) {
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
      size_t invertedStart = pathEnd + 1;
      size_t invertedEnd = str.find(':', invertedStart);
      bool inverted = str.substr(invertedStart, invertedEnd - invertedStart) == "true";
      this->currentMapping().controls[control] = {output, path, inverted};
      this->currentMapping().feedbacks[path] = control;
    } else if (command == "echo") {
      this->gui->send(str);
    }
    this->write();
  });
  std::transform(config.banks.begin(), config.banks.end(), std::back_inserter(mappings),
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
                     size_t invertedStart = pathEnd + 1;
                     size_t invertedEnd = line.find(':', invertedStart);
                     bool inverted = line.substr(invertedStart, invertedEnd - invertedStart) == "true";
                     mapping.controls[control] = {output, path, inverted};
                     mapping.feedbacks[path] = control;
                   }
                   return mapping;
                 });
}

void Mappings::write() {
  for (const Mapping& mapping : mappings) {
    std::ofstream f(mapping.filename);
    for (std::pair<std::string, Control> control : mapping.controls) {
      f << control.first << ":" << control.second.output << ":" << control.second.path << "\n";
    }
    f.close();
  }
}

