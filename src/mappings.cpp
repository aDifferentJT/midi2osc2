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

Mappings::Control::Control(std::string str, size_t start) {
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

Mappings::Channel::Channel(std::string str, size_t start) {
  size_t outputStart = start;
  size_t outputEnd = str.find(':', outputStart);
  output = str.substr(outputStart, outputEnd - outputStart);
  size_t channelStart = outputEnd + 1;
  size_t channelEnd = str.find(':', channelStart);
  channel = str.substr(channelStart, channelEnd - channelStart);
}

Mappings::Action::Action(std::string str, size_t start) {
  size_t actionStart = start;
  size_t actionEnd = str.find(':', actionStart);
  action = str.substr(actionStart, actionEnd - actionStart);
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
      try {
        Control control = currentMapping().controls.at(event.control);
        this->config.outputs.at(control.output)->send(control.path, control.inverted ? 1.0 - event.value : event.value);
      } catch (std::out_of_range&) {
        try {
          Channel channel = currentMapping().channels.at(this->config.channelForControl(event.control));
          Action action = currentMapping().actions.at(this->config.actionForControl(event.control));
          Output* output = this->config.outputs.at(channel.output);
          std::pair<std::string, bool> merged = output->merge(channel.channel, action.action);
          output->send(merged.first, merged.second ? 1.0 - event.value : event.value);
        } catch (std::out_of_range&) {
          std::cerr << "Control not mapped: " << event.control << std::endl;
        }
      }
      {
        std::optional<Control> control = getOpt(currentMapping().controls, event.control);
        std::optional<Channel> channel = getOpt(currentMapping().channels, this->config.channelForControl(event.control));
        std::optional<Action> action = getOpt(currentMapping().actions, this->config.actionForControl(event.control));
        this->gui->send
          ( "moved:" + event.control
           + ":" + std::to_string(event.value)
           + ":" + bindOptional<std::string, Control>(control, &Control::encode).value_or("::")
           + ":" + this->config.channelForControl(event.control)
           + ":" + bindOptional<std::string, Channel>(channel, &Channel::encode).value_or(":")
           + ":" + this->config.actionForControl(event.control)
           + ":" + bindOptional<std::string, Action>(action, &Action::encode).value_or("")
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
      this->currentMapping().controls[control] = Control(str, controlEnd + 1);
      this->currentMapping().feedbacks[this->currentMapping().controls[control].path] = control;
    } else if (command == "clearControl") {
      size_t controlStart = commandEnd + 1;
      size_t controlEnd = str.find(':', controlStart);
      std::string control = str.substr(controlStart, controlEnd - controlStart);
      try {
        this->currentMapping().feedbacks.erase(this->currentMapping().controls.at(control).path);
      } catch (std::out_of_range&) {}
      this->currentMapping().controls.erase(control);
    } else if (command == "setChannel") {
      size_t channelStart = commandEnd + 1;
      size_t channelEnd = str.find(':', channelStart);
      std::string channel = str.substr(channelStart, channelEnd - channelStart);
      this->currentMapping().channels[channel] = Channel(str, channelEnd + 1);
      // TODO: Add feedback
    } else if (command == "clearChannel") {
      size_t channelStart = commandEnd + 1;
      size_t channelEnd = str.find(':', channelStart);
      std::string channel = str.substr(channelStart, channelEnd - channelStart);
      // TODO: Clear feedback
      this->currentMapping().channels.erase(channel);
    } else if (command == "setAction") {
      size_t actionStart = commandEnd + 1;
      size_t actionEnd = str.find(':', actionStart);
      std::string action = str.substr(actionStart, actionEnd - actionStart);
      this->currentMapping().actions[action] = Action(str, actionEnd + 1);
      // TODO: Add feedback
    } else if (command == "clearAction") {
      size_t actionStart = commandEnd + 1;
      size_t actionEnd = str.find(':', actionStart);
      std::string action = str.substr(actionStart, actionEnd - actionStart);
      // TODO: Clear feedback
      this->currentMapping().actions.erase(action);
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
        std::optional<Control> control = getOpt(currentMapping().controls, controlName);
        std::optional<Channel> channel = getOpt(currentMapping().channels, this->config.channelForControl(controlName));
        std::optional<Action> action = getOpt(currentMapping().actions, this->config.actionForControl(controlName));
        this->gui->send
          ( "moved:" + controlName
           + ":unchanged"
           + ":" + bindOptional<std::string, Control>(control, &Control::encode).value_or("::")
           + ":" + this->config.channelForControl(controlName)
           + ":" + bindOptional<std::string, Channel>(channel, &Channel::encode).value_or(":")
           + ":" + this->config.actionForControl(controlName)
           + ":" + bindOptional<std::string, Action>(action, &Action::encode).value_or("")
          );
      }
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
                     size_t typeStart = 0;
                     size_t typeEnd = line.find(':', typeStart);
                     std::string type = line.substr(typeStart, typeEnd - typeStart);
                     if (type == "control") {
                       size_t controlStart = typeEnd + 1;
                       size_t controlEnd = line.find(':', controlStart);
                       std::string control = line.substr(controlStart, controlEnd - controlStart);
                       mapping.controls[control] = Control(line, controlEnd + 1);
                       mapping.feedbacks[mapping.controls[control].path] = control;
                     } else if (type == "channel") {
                       size_t channelStart = typeEnd + 1;
                       size_t channelEnd = line.find(':', channelStart);
                       std::string channel = line.substr(channelStart, channelEnd - channelStart);
                       mapping.channels[channel] = Channel(line, channelEnd + 1);
                       // TODO: Add feedback
                     } else if (type == "action") {
                       size_t actionStart = typeEnd + 1;
                       size_t actionEnd = line.find(':', actionStart);
                       std::string action = line.substr(actionStart, actionEnd - actionStart);
                       mapping.actions[action] = Action(line, actionEnd + 1);
                       // TODO: Add feedback
                     }
                   }
                   return mapping;
                 });
}

void Mappings::write() {
  for (const Mapping& mapping : mappings) {
    std::ofstream f(mapping.filename);
    for (std::pair<std::string, Control> control : mapping.controls) {
      f << "control:" << control.first << ":" << control.second.output << ":" << control.second.path << "\n";
    }
    for (std::pair<std::string, Channel> channel : mapping.channels) {
      f << "channel:" << channel.first << ":" << channel.second.output << ":" << channel.second.channel << "\n";
    }
    for (std::pair<std::string, Action> action : mapping.actions) {
      f << "action:" << action.first << ":" << action.second.action << "\n";
    }
    f.close();
  }
}
