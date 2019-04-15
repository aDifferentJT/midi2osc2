#ifndef Output_h
#define Output_h

#include <functional>
#include <string>

class Output {
  public:
    Output() = default;
    Output(const Output&) = delete;
    Output& operator=(const Output&) = delete;
    Output(Output&&) = delete;
    Output& operator=(Output&&) = delete;
    virtual ~Output() = default;
    virtual void send(std::string addressPattern) = 0;
    virtual void send(std::string addressPattern, float v) = 0;
    virtual void setCallback(std::function<void(std::string, float)> f) = 0;
    virtual std::pair<std::string, bool> merge(std::string channel, std::string action) const = 0;
};

#endif

