#include "mappings.hpp"
#include <iostream>    // for cerr
#include <memory>      // for unique_ptr
#include "gui.hpp"     // for GUI
#include "output.hpp"  // for Output
#include "utils.hpp"   // for parseUnit, bindMember, bindOptional, getOpt

Mappings::ControlOutput Mappings::ControlOutput::parse(const std::string& str, std::size_t start) {
  parseUnit(output, str, start);
  parseUnit(path, str, outputEnd + 1);
  parseUnit(inverted, str, pathEnd + 1);
  return ControlOutput(output, path, inverted == "true");
}

Mappings::ChannelOutput Mappings::ChannelOutput::parse(const std::string& str, std::size_t start) {
  parseUnit(output, str, start);
  parseUnit(channel, str, outputEnd + 1);
  return ChannelOutput(output, channel);
}

Mappings::ActionOutput Mappings::ActionOutput::parse(const std::string& str, std::size_t start) {
  parseUnit(action, str, start);
  return ActionOutput(action);
}

Mappings::Mapping::Mapping(const Config& config, const std::string& filename) : config(config), filename(filename) {
  std::ifstream f(filename);
  while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
    std::string line;
    std::getline(f, line);
    parseUnit(type, line, 0);
    parseUnit(name, line, typeEnd + 1);
    if (type == "control") {
      addControl(name, ControlOutput::parse(line, nameEnd + 1));
      addFeedback(name);
    } else if (type == "channel") {
      addChannel(name, ChannelOutput::parse(line, nameEnd + 1));
      addFeedbackForChannel(name);
    } else if (type == "action") {
      addAction(name, ActionOutput::parse(line, nameEnd + 1));
      addFeedbackForAction(name);
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
      //std::cerr << "Control not mapped: " << str << std::endl;
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

void Mappings::midiCallback(const Midi::Event& event) {
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

void Mappings::midiMockCallback(const std::string& led, bool value) {
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
  parseUnit(command, str, 0);
  parseUnit(name, str, commandEnd + 1);
  if (command == "setControl") {
    currentMapping().addControl(name, ControlOutput::parse(str, nameEnd + 1));
    currentMapping().addFeedback(name);
  } else if (command == "clearControl") {
    currentMapping().removeControl(name);
    currentMapping().addFeedback(name);
  } else if (command == "setChannel") {
    currentMapping().addChannel(name, ChannelOutput::parse(str, nameEnd + 1));
    currentMapping().addFeedbackForChannel(name);
  } else if (command == "clearChannel") {
    currentMapping().removeChannel(name);
    currentMapping().addFeedbackForChannel(name);
  } else if (command == "setAction") {
    currentMapping().addAction(name, ActionOutput::parse(str, nameEnd + 1));
    currentMapping().addFeedbackForAction(name);
  } else if (command == "clearAction") {
    currentMapping().removeAction(name);
    currentMapping().addFeedbackForAction(name);
  } else if (command == "echo") {
    config.gui->send(str);
  } else if (command == "bankChange") {
    parseUnit(direction, str, commandEnd + 1);
    parseUnit(control, str, directionEnd + 1);

    if (direction == "left") {
      if (currentMappingIndex > 0) {
        currentMappingIndex -= 1;
      }
    } else if (direction == "right") {
      if (currentMappingIndex < mappings.size() - 1) {
        currentMappingIndex += 1;
      }
    }
    refreshBank();
    if (!control.empty()) {
      //send the named control data again after the bank switch has been made
      config.gui->send("moved:" + control + ":unchanged:" + currentMapping().encodedMappingOf(control));
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

