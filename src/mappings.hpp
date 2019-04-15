#ifndef Mapping_h
#define Mapping_h

#include "config.hpp"
#include "gui.hpp"
#include "midi.hpp"
#include "output.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>

class Mappings {
  private:
    struct ControlOutput {
      std::string output;
      std::string path;
      bool inverted;
      ControlOutput(std::string str, size_t start = 0);
      ControlOutput() = default;
      ControlOutput(std::string output, std::string path, bool inverted) : output(output), path(path), inverted(inverted) {}
      std::string encode() const { return output + ":" + path + ":" + (inverted ? "true" : "false"); }
    };
    struct ChannelOutput {
      std::string output;
      std::string channel;
      ChannelOutput(std::string str, size_t start = 0);
      ChannelOutput() = default;
      ChannelOutput(std::string output, std::string channel) : output(output), channel(channel) {}
      std::string encode() const { return output + ":" + channel; }
    };
    struct ActionOutput {
      std::string action;
      ActionOutput(std::string str, size_t start = 0);
      ActionOutput() = default;
      ActionOutput(std::string action) : action(action) {}
      std::string encode() const { return action; }
    };
    struct Mapping {
      private:
        const Config& config;
      public:
        const std::string filename;
        std::unordered_map<std::string, ControlOutput> controls;
        std::unordered_map<std::string, ChannelOutput> channels;
        std::unordered_map<std::string, ActionOutput> actions;
        std::unordered_map<std::string, std::string> feedbacks;
        Mapping(const Config& config, std::string filename) : config(config), filename(filename) {}

        void write() const;

        std::optional<ControlOutput> outputFromString(std::string str);

        void addFeedback(std::string control) {
          std::optional<ControlOutput> output = outputFromString(control);
          if (output) {
          feedbacks[output->path] = control;
          }
        }
      private:
        template <typename T>
          void addFeedback(T controls) {
            for (std::string control : controls) {
              addFeedback(control);
            }
          }
      public:
        void addFeedbackForChannel(std::string channel) {
          addFeedback(config.controlsForChannel(channel));
        }
        void addFeedbackForAction(std::string action) {
          addFeedback(config.controlsForAction(action));
        }
    };
    const Config& config;
    GUI* gui;
    std::vector<Mapping> mappings;
    size_t currentMappingIndex = 0;
    Mapping& currentMapping() { return mappings[currentMappingIndex]; }

    void refreshBank();
  public:
    Mappings(const Config& config, GUI* gui);
    void write() {
      for (const Mapping& mapping : mappings) {
        mapping.write();
      }
    }
};

#endif

