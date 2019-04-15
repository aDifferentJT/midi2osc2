#include "mappings.hpp"

template <typename T, typename U>
std::optional<T> bindOptional(std::optional<U> x, std::function<T(U*)> f) { return x ? std::optional{f(&*x)} : std::nullopt; }

template <typename T, typename U>
std::optional<U> getOpt(std::unordered_map<T,U> xs, T x) {
  try {
    return xs.at(x);
  } catch (std::out_of_range&) {
    return std::nullopt;
  }
}

Mappings::ControlOutput::ControlOutput(std::string str, size_t start) {
  size_t outputStart = start;
  size_t outputEnd = str.find(':', outputStart);
  output = str.substr(outputStart, outputEnd - outputStart);
  size_t pathStart = outputEnd + 1;
  size_t pathEnd = str.find(':', pathStart);
  path = str.substr(pathStart, pathEnd - pathStart);
  size_t invertedStart = pathEnd + 1;
  size_t invertedEnd = str.find(':', invertedStart);
  inverted = str.substr(invertedStart, invertedEnd - invertedStart) == "true";
}

Mappings::ChannelOutput::ChannelOutput(std::string str, size_t start) {
  size_t outputStart = start;
  size_t outputEnd = str.find(':', outputStart);
  output = str.substr(outputStart, outputEnd - outputStart);
  size_t channelStart = outputEnd + 1;
  size_t channelEnd = str.find(':', channelStart);
  channel = str.substr(channelStart, channelEnd - channelStart);
}

Mappings::ActionOutput::ActionOutput(std::string str, size_t start) {
  size_t actionStart = start;
  size_t actionEnd = str.find(':', actionStart);
  action = str.substr(actionStart, actionEnd - actionStart);
}

void Mappings::Mapping::write() const {
    std::ofstream f(filename);
    for (std::pair<std::string, ControlOutput> control : controls) {
      f << "control:" << control.first << ":" << control.second.output << ":" << control.second.path << "\n";
    }
    for (std::pair<std::string, ChannelOutput> channel : channels) {
      f << "channel:" << channel.first << ":" << channel.second.output << ":" << channel.second.channel << "\n";
    }
    for (std::pair<std::string, ActionOutput> action : actions) {
      f << "action:" << action.first << ":" << action.second.action << "\n";
    }
    f.close();
}

std::optional<Mappings::ControlOutput> Mappings::Mapping::outputFromString(std::string str) {
      try {
        return controls.at(str);
      } catch (std::out_of_range&) {
        try {
          ChannelOutput channel = channels.at(config.channelForControl(str));
          ActionOutput action = actions.at(config.actionForControl(str));
          Output* output = config.outputs.at(channel.output);
          std::pair<std::string, bool> merged = output->merge(channel.channel, action.action);
          return ControlOutput(channel.output, merged.first, merged.second);
        } catch (std::out_of_range&) {
          std::cerr << "Control not mapped: " << str << std::endl;
          return std::nullopt;
        }
      }
}

void Mappings::refreshBank() {
  this->config.midi->setLed(this->config.bankLeft, currentMappingIndex > 0);
  this->config.midi->setLed(this->config.bankRight, currentMappingIndex < mappings.size() - 1);
  this->gui->send("bank:" + std::to_string(currentMappingIndex));
  if (currentMappingIndex > 0) {
    this->gui->send("enableBank:left");
  } else {
    this->gui->send("disableBank:left");
  }
  if (currentMappingIndex < mappings.size() - 1) {
    this->gui->send("enableBank:right");
  } else {
    this->gui->send("disableBank:right");
  }
}

Mappings::Mappings(const Config& config, GUI* gui)
  : config(config)
    , gui(gui)
{
  refreshBank();
  config.midi->setCallback([this](Midi::Event event) {
    if (event.control == this->config.bankLeft) {
      if (currentMappingIndex > 0) {
        currentMappingIndex -= 1;
        refreshBank();
      }
    } else if (event.control == this->config.bankRight) {
      if (currentMappingIndex < mappings.size() - 1) {
        currentMappingIndex += 1;
        refreshBank();
      }
    } else {
      std::optional<ControlOutput> control = currentMapping().outputFromString(event.control);
      if (control) {
        this->config.outputs.at(control->output)->send(control->path, control->inverted ? 1.0 - event.value : event.value);
      }
      {
        std::optional<ControlOutput> control = getOpt(currentMapping().controls, event.control);
        std::optional<ChannelOutput> channel = getOpt(currentMapping().channels, this->config.channelForControl(event.control));
        std::optional<ActionOutput> action = getOpt(currentMapping().actions, this->config.actionForControl(event.control));
        this->gui->send
          ( "moved:" + event.control
           + ":" + std::to_string(event.value)
           + ":" + bindOptional<std::string, ControlOutput>(control, &ControlOutput::encode).value_or("::")
           + ":" + this->config.channelForControl(event.control)
           + ":" + bindOptional<std::string, ChannelOutput>(channel, &ChannelOutput::encode).value_or(":")
           + ":" + this->config.actionForControl(event.control)
           + ":" + bindOptional<std::string, ActionOutput>(action, &ActionOutput::encode).value_or("")
          );
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
    refreshBank();

    std::string devices = "devices";
    for (std::pair<std::string, Output*> output : this->config.outputs) {
      devices += ":" + output.first;
    }
    this->gui->send(devices);
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
      this->currentMapping().controls[control] = ControlOutput(str, controlEnd + 1);
      currentMapping().addFeedback(control);
    } else if (command == "clearControl") {
      size_t controlStart = commandEnd + 1;
      size_t controlEnd = str.find(':', controlStart);
      std::string control = str.substr(controlStart, controlEnd - controlStart);
      this->currentMapping().controls.erase(control);
      currentMapping().addFeedback(control);
    } else if (command == "setChannel") {
      size_t channelStart = commandEnd + 1;
      size_t channelEnd = str.find(':', channelStart);
      std::string channel = str.substr(channelStart, channelEnd - channelStart);
      this->currentMapping().channels[channel] = ChannelOutput(str, channelEnd + 1);
      currentMapping().addFeedbackForChannel(channel);
    } else if (command == "clearChannel") {
      size_t channelStart = commandEnd + 1;
      size_t channelEnd = str.find(':', channelStart);
      std::string channel = str.substr(channelStart, channelEnd - channelStart);
      this->currentMapping().channels.erase(channel);
      currentMapping().addFeedbackForChannel(channel);
    } else if (command == "setAction") {
      size_t actionStart = commandEnd + 1;
      size_t actionEnd = str.find(':', actionStart);
      std::string action = str.substr(actionStart, actionEnd - actionStart);
      this->currentMapping().actions[action] = ActionOutput(str, actionEnd + 1);
      currentMapping().addFeedbackForAction(action);
    } else if (command == "clearAction") {
      size_t actionStart = commandEnd + 1;
      size_t actionEnd = str.find(':', actionStart);
      std::string action = str.substr(actionStart, actionEnd - actionStart);
      this->currentMapping().actions.erase(action);
      currentMapping().addFeedbackForAction(action);
    } else if (command == "echo") {
      this->gui->send(str);
    } else if (command == "bankChange") {
      size_t directionStart = commandEnd + 1;
      size_t directionEnd = str.find(':', directionStart);
      std::string direction = str.substr(directionStart, directionEnd - directionStart);

      size_t controlStart = directionEnd + 1;
      size_t controlEnd = str.find(':', controlStart);
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
      if (controlName != "") {
        //send the named control data again after the bank switch has been made
        std::optional<ControlOutput> control = getOpt(currentMapping().controls, controlName);
        std::optional<ChannelOutput> channel = getOpt(currentMapping().channels, this->config.channelForControl(controlName));
        std::optional<ActionOutput> action = getOpt(currentMapping().actions, this->config.actionForControl(controlName));
        this->gui->send
          ( "moved:" + controlName
           + ":unchanged"
           + ":" + bindOptional<std::string, ControlOutput>(control, &ControlOutput::encode).value_or("::")
           + ":" + this->config.channelForControl(controlName)
           + ":" + bindOptional<std::string, ChannelOutput>(channel, &ChannelOutput::encode).value_or(":")
           + ":" + this->config.actionForControl(controlName)
           + ":" + bindOptional<std::string, ActionOutput>(action, &ActionOutput::encode).value_or("")
          );
      }
    }
    this->write();
  });
  std::transform(config.banks.begin(), config.banks.end(), std::back_inserter(mappings),
                 [this](std::string filename) -> Mapping {
                   Mapping mapping(this->config, filename);
                   std::ifstream f(filename);
                   while (!f.eof() && f.peek() != std::char_traits<char>::eof()) {
                     std::string line;
                     std::getline(f, line);
                     size_t typeStart = 0;
                     size_t typeEnd = line.find(':', typeStart);
                     std::string type = line.substr(typeStart, typeEnd - typeStart);
                     if (type == "control") {
                       size_t controlStart = typeEnd + 1;
                       size_t controlEnd = line.find(':', controlStart);
                       std::string control = line.substr(controlStart, controlEnd - controlStart);
                       mapping.controls[control] = ControlOutput(line, controlEnd + 1);
                       mapping.addFeedback(control);
                     } else if (type == "channel") {
                       size_t channelStart = typeEnd + 1;
                       size_t channelEnd = line.find(':', channelStart);
                       std::string channel = line.substr(channelStart, channelEnd - channelStart);
                       mapping.channels[channel] = ChannelOutput(line, channelEnd + 1);
                       mapping.addFeedbackForChannel(channel);
                     } else if (type == "action") {
                       size_t actionStart = typeEnd + 1;
                       size_t actionEnd = line.find(':', actionStart);
                       std::string action = line.substr(actionStart, actionEnd - actionStart);
                       mapping.actions[action] = ActionOutput(line, actionEnd + 1);
                       mapping.addFeedbackForAction(action);
                     }
                   }
                   return mapping;
                 });
}

