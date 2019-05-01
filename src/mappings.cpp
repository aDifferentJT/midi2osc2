#include "mappings.hpp"
#include <algorithm>   // for transform
#include <iostream>    // for cerr, cout
#include <iterator>    // for back_insert_iterator, back_inserter
#include <memory>      // for unique_ptr
#include "gui.hpp"     // for GUI
#include "output.hpp"  // for Output
#include "utils.hpp"   // for bindOptional, getOpt

Mappings::ControlOutput::ControlOutput(const std::string& str, std::size_t start)
  : output([str, start]() {
    std::size_t outputStart = start;
    std::size_t outputEnd = str.find(':', outputStart);
    return str.substr(outputStart, outputEnd - outputStart);
  }())
, path([str, start]() {
  std::size_t outputStart = start;
  std::size_t outputEnd = str.find(':', outputStart);
  std::size_t pathStart = outputEnd + 1;
  std::size_t pathEnd = str.find(':', pathStart);
  return str.substr(pathStart, pathEnd - pathStart);
}())
, inverted([str, start]() {
  std::size_t outputStart = start;
  std::size_t outputEnd = str.find(':', outputStart);
  std::size_t pathStart = outputEnd + 1;
  std::size_t pathEnd = str.find(':', pathStart);
  std::size_t invertedStart = pathEnd + 1;
  std::size_t invertedEnd = str.find(':', invertedStart);
  return str.substr(invertedStart, invertedEnd - invertedStart) == "true";
}()) {}

Mappings::ChannelOutput::ChannelOutput(const std::string& str, std::size_t start)
  : output([str, start]() {
    std::size_t outputStart = start;
    std::size_t outputEnd = str.find(':', outputStart);
    return str.substr(outputStart, outputEnd - outputStart);
  }())
, channel([str, start]() {
  std::size_t outputStart = start;
  std::size_t outputEnd = str.find(':', outputStart);
  std::size_t channelStart = outputEnd + 1;
  std::size_t channelEnd = str.find(':', channelStart);
  return str.substr(channelStart, channelEnd - channelStart);
}()) {}

Mappings::ActionOutput::ActionOutput(const std::string& str, std::size_t start) 
  : action([str, start]() {
    std::size_t actionStart = start;
    std::size_t actionEnd = str.find(':', actionStart);
    return str.substr(actionStart, actionEnd - actionStart);
  }()) {}

Mappings::Mapping::Mapping(const Config& config, const std::string& filename) : config(config), filename(filename) {
  std::ifstream f(filename);
  while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
    std::string line;
    std::getline(f, line);
    std::size_t typeStart = 0;
    std::size_t typeEnd = line.find(':', typeStart);
    std::string type = line.substr(typeStart, typeEnd - typeStart);
    if (type == "control") {
      std::size_t controlStart = typeEnd + 1;
      std::size_t controlEnd = line.find(':', controlStart);
      std::string control = line.substr(controlStart, controlEnd - controlStart);
      addControl(control, ControlOutput(line, controlEnd + 1));
      addFeedback(control);
    } else if (type == "channel") {
      std::size_t channelStart = typeEnd + 1;
      std::size_t channelEnd = line.find(':', channelStart);
      std::string channel = line.substr(channelStart, channelEnd - channelStart);
      addChannel(channel, ChannelOutput(line, channelEnd + 1));
      addFeedbackForChannel(channel);
    } else if (type == "action") {
      std::size_t actionStart = typeEnd + 1;
      std::size_t actionEnd = line.find(':', actionStart);
      std::string action = line.substr(actionStart, actionEnd - actionStart);
      addAction(action, ActionOutput(line, actionEnd + 1));
      addFeedbackForAction(action);
    }
  }
}

void Mappings::Mapping::save() const {
  std::ofstream f(filename);
  for (auto& control : controls) {
    f << "control:" << control.first << ":" << control.second.output << ":" << control.second.path << "\n";
  }
  for (auto& channel : channels) {
    f << "channel:" << channel.first << ":" << channel.second.output << ":" << channel.second.channel << "\n";
  }
  for (auto& action : actions) {
    f << "action:" << action.first << ":" << action.second.action << "\n";
  }
  f.close();
}

std::optional<Mappings::ControlOutput> Mappings::Mapping::outputFromString(const std::string& str) {
  try {
    return controls.at(str);
  } catch (std::out_of_range&) {
    try {
      ChannelOutput channel = channels.at(config.channelForControl(str));
      ActionOutput action = actions.at(config.actionForControl(str));
      std::pair<std::string, bool> merged = config.outputs.at(channel.output)->merge(channel.channel, action.action);
      return ControlOutput(channel.output, merged.first, merged.second);
    } catch (std::out_of_range&) {
      std::cerr << "Control not mapped: " << str << std::endl;
      return std::nullopt;
    }
  }
}

std::string Mappings::Mapping::encodedMappingOf(const std::string& controlName) {
  std::optional<ControlOutput> control = getOpt(controls, controlName);
  std::optional<ChannelOutput> channel = getOpt(channels, config.channelForControl(controlName));
  std::optional<ActionOutput> action = getOpt(actions, config.actionForControl(controlName));
  return
    ( bindOptional<std::string, ControlOutput>(control, &ControlOutput::encode).value_or("::")
     + ":" + config.channelForControl(controlName)
     + ":" + bindOptional<std::string, ChannelOutput>(channel, &ChannelOutput::encode).value_or(":")
     + ":" + config.actionForControl(controlName)
     + ":" + bindOptional<std::string, ActionOutput>(action, &ActionOutput::encode).value_or("")
    );
}

void Mappings::refreshBank() {
  config.midi->setLed(config.bankLeft, currentMappingIndex > 0);
  config.midi->setLed(config.bankRight, currentMappingIndex < mappings.size() - 1);
  config.gui->send("bank:" + std::to_string(currentMappingIndex));
  if (currentMappingIndex > 0) {
    config.gui->send("enableBank:left");
  } else {
    config.gui->send("disableBank:left");
  }
  if (currentMappingIndex < mappings.size() - 1) {
    config.gui->send("enableBank:right");
  } else {
    config.gui->send("disableBank:right");
  }
}

void Mappings::midiCallback(Midi::Event event) {
  if (event.control == config.bankLeft) {
    if (currentMappingIndex > 0) {
      currentMappingIndex -= 1;
      refreshBank();
    }
  } else if (event.control == config.bankRight) {
    if (currentMappingIndex < mappings.size() - 1) {
      currentMappingIndex += 1;
      refreshBank();
    }
  } else {
    std::optional<ControlOutput> control = currentMapping().outputFromString(event.control);
    if (control) {
      config.outputs.at(control->output)->send(control->path, control->inverted ? 1.0 - event.value : event.value);
    }
    config.gui->send("moved:" + event.control + ":" + std::to_string(event.value) + ":" + currentMapping().encodedMappingOf(event.control));
  }
}

void Mappings::midiMockCallback(std::string led, bool value) {
  config.gui->send("mockSetLed:" + led + ":" + (value ? "true" : "false"));
}

void Mappings::outputCallback(const std::string& path, float v) {
  std::optional<std::pair<std::string, bool>> control = currentMapping().feedbackFor(path);
  if (control) {
    config.midi->feedback(control->first, control->second ? 1.0 - v : v);
  }
}

void Mappings::guiOpenCallback() {
  refreshBank();

  std::string devices = "devices";
  for (auto& output : config.outputs) {
    devices += ":" + output.first;
  }
  config.gui->send(devices);
  if (config.midi->isMock) {
    config.gui->send("mock");
  }
}

void Mappings::guiRecvCallback(const std::string& str) {
  std::size_t commandStart = 0;
  std::size_t commandEnd = str.find(':', commandStart);
  std::string command = str.substr(commandStart, commandEnd - commandStart);
  if (command == "setControl") {
    std::size_t controlStart = commandEnd + 1;
    std::size_t controlEnd = str.find(':', controlStart);
    std::string control = str.substr(controlStart, controlEnd - controlStart);
    currentMapping().addControl(control, ControlOutput(str, controlEnd + 1));
    currentMapping().addFeedback(control);
  } else if (command == "clearControl") {
    std::size_t controlStart = commandEnd + 1;
    std::size_t controlEnd = str.find(':', controlStart);
    std::string control = str.substr(controlStart, controlEnd - controlStart);
    currentMapping().removeControl(control);
    currentMapping().addFeedback(control);
  } else if (command == "setChannel") {
    std::size_t channelStart = commandEnd + 1;
    std::size_t channelEnd = str.find(':', channelStart);
    std::string channel = str.substr(channelStart, channelEnd - channelStart);
    currentMapping().addChannel(channel, ChannelOutput(str, channelEnd + 1));
    currentMapping().addFeedbackForChannel(channel);
  } else if (command == "clearChannel") {
    std::size_t channelStart = commandEnd + 1;
    std::size_t channelEnd = str.find(':', channelStart);
    std::string channel = str.substr(channelStart, channelEnd - channelStart);
    currentMapping().removeChannel(channel);
    currentMapping().addFeedbackForChannel(channel);
  } else if (command == "setAction") {
    std::size_t actionStart = commandEnd + 1;
    std::size_t actionEnd = str.find(':', actionStart);
    std::string action = str.substr(actionStart, actionEnd - actionStart);
    currentMapping().addAction(action, ActionOutput(str, actionEnd + 1));
    currentMapping().addFeedbackForAction(action);
  } else if (command == "clearAction") {
    std::size_t actionStart = commandEnd + 1;
    std::size_t actionEnd = str.find(':', actionStart);
    std::string action = str.substr(actionStart, actionEnd - actionStart);
    currentMapping().removeAction(action);
    currentMapping().addFeedbackForAction(action);
  } else if (command == "echo") {
    config.gui->send(str);
  } else if (command == "bankChange") {
    std::size_t directionStart = commandEnd + 1;
    std::size_t directionEnd = str.find(':', directionStart);
    std::string direction = str.substr(directionStart, directionEnd - directionStart);

    std::size_t controlStart = directionEnd + 1;
    std::size_t controlEnd = str.find(':', controlStart);
    std::string controlName = str.substr(controlStart, controlEnd - controlStart);

    if (direction=="left") {
      if (currentMappingIndex > 0) {
        currentMappingIndex -= 1;
      }
    } else if (direction=="right") {
      if (currentMappingIndex < mappings.size() - 1) {
        currentMappingIndex += 1;
      }
    }
    refreshBank();
    if (!controlName.empty()) {
      //send the named control data again after the bank switch has been made
      config.gui->send("moved:" + controlName + ":unchanged:" + currentMapping().encodedMappingOf(controlName));
    }
  } else if (command == "mockMoved") {
    std::size_t controlStart = commandEnd + 1;
    std::size_t controlEnd = str.find(':', controlStart);
    std::string control = str.substr(controlStart, controlEnd - controlStart);
    std::size_t valueStart = controlEnd + 1;
    std::size_t valueEnd = str.find(':', valueStart);
    float value = std::stof(str.substr(valueStart, valueEnd - valueStart));
    config.midi->recvMockMoved({control, value});
  } else {
    std::cerr << "Unknown gui command line: " << str << std::endl;
  }
  save();
}

Mappings::Mappings(Config& config)
  : config(config)
{
  refreshBank();
  config.midi->config = &config;
  config.midi->setCallback(bindMember(&Mappings::midiCallback, this));
  config.midi->setMockCallback(bindMember(&Mappings::midiMockCallback, this));
  for (auto& output : config.outputs) {
    output.second->setCallback(bindMember(&Mappings::outputCallback, this));
  }
  config.gui->setOpenCallback(bindMember(&Mappings::guiOpenCallback, this));
  config.gui->setRecvCallback(bindMember(&Mappings::guiRecvCallback, this));
  load();
}

