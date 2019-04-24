#ifndef Mapping_h
#define Mapping_h

#include <fstream>        // for size_t
#include <optional>       // for optional, nullopt
#include <set>            // for set
#include <stdexcept>      // for out_of_range
#include <string>         // for string, allocator, operator+, char_traits
#include <unordered_map>  // for unordered_map, unordered_map<>::mapped_type
#include <utility>        // for move
#include <vector>         // for vector
#include "config.hpp"     // for Config
#include "midi.hpp"       // for Midi::Event, Midi

class Mappings {
  private:
    struct ControlOutput {
      const std::string output;
      const std::string path;
      bool inverted = false;
      ControlOutput(const std::string& str, std::size_t start = 0);
      ControlOutput() = default;
      ControlOutput(std::string output, std::string path, bool inverted) : output(std::move(output)), path(std::move(path)), inverted(inverted) {}
      std::string encode() const { return output + ":" + path + ":" + (inverted ? "true" : "false"); }
    };
    struct ChannelOutput {
      const std::string output;
      const std::string channel;
      ChannelOutput(const std::string& str, std::size_t start = 0);
      ChannelOutput() = default;
      ChannelOutput(std::string output, std::string channel) : output(std::move(output)), channel(std::move(channel)) {}
      std::string encode() const { return output + ":" + channel; }
    };
    struct ActionOutput {
      const std::string action;
      ActionOutput(const std::string& str, std::size_t start = 0);
      ActionOutput() = default;
      ActionOutput(std::string action) : action(std::move(action)) {}
      std::string encode() const { return action; }
    };
    struct Mapping {
      private:
        const Config& config;
      public:
        const std::string filename;
      private:
        std::unordered_map<std::string, ControlOutput> controls;
        std::unordered_map<std::string, ChannelOutput> channels;
        std::unordered_map<std::string, ActionOutput> actions;
        std::unordered_map<std::string, std::pair<std::string, bool>> feedbacks;
      public:
        Mapping(const Config& config, std::string filename) : config(config), filename(std::move(filename)) {}

        void write() const;

        std::optional<ControlOutput> outputFromString(const std::string& str);

        std::string encodedMappingOf(const std::string& controlName);

        std::optional<std::pair<std::string, bool>> feedbackFor(const std::string& str) {
          try {
            return feedbacks.at(str);
          } catch (std::out_of_range&) {
            return std::nullopt;
          }
        }

        void addControl(const std::string& control, ControlOutput&& output) {
          controls.erase(control);
          controls.emplace(control, output);
        }
        void removeControl(const std::string& control) {
          controls.erase(control);
        }
        void addChannel(const std::string& channel, ChannelOutput&& output) {
          channels.erase(channel);
          channels.emplace(channel, output);
        }
        void removeChannel(const std::string& channel) {
          channels.erase(channel);
        }
        void addAction(const std::string& action, ActionOutput&& output) {
          actions.erase(action);
          actions.emplace(action, output);
        }
        void removeAction(const std::string& action) {
          actions.erase(action);
        }
        void addFeedback(const std::string& control) {
          std::optional<ControlOutput> output = outputFromString(control);
          if (output) {
            feedbacks[output->path] = std::make_pair(control, output->inverted);
          }
        }
      private:
        template <typename T>
          void addFeedback(T controls) {
            for (const std::string& control : controls) {
              addFeedback(control);
            }
          }
      public:
        void addFeedbackForChannel(const std::string& channel) {
          addFeedback(config.controlsForChannel(channel));
        }
        void addFeedbackForAction(const std::string& action) {
          addFeedback(config.controlsForAction(action));
        }
    };
    Config& config;
    std::vector<Mapping> mappings;
    std::size_t currentMappingIndex = 0;
    Mapping& currentMapping() { return mappings[currentMappingIndex]; }

    void refreshBank();

    void midiCallback(Midi::Event event);
    void outputCallback(const std::string& path, float v);
    void guiOpenCallback();
    void guiRecvCallback(const std::string& str);
  public:
    Mappings(Config& config);
    void write() {
      for (const Mapping& mapping : mappings) {
        mapping.write();
      }
    }
};

#endif

