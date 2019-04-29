#ifndef Output_h
#define Output_h

#include <chrono>
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
    virtual void send(const std::string& addressPattern) = 0;
    virtual void send(const std::string& addressPattern, float v) = 0;
    virtual void sendPeriodically(const std::string& addressPattern, std::chrono::seconds period = std::chrono::seconds(9)) = 0;
    virtual void sendPeriodically(const std::string& addressPattern, float v, std::chrono::seconds period = std::chrono::seconds(9)) = 0;
    virtual void setCallback(std::function<void(const std::string&, float)> f) = 0;
    virtual std::pair<std::string, bool> merge(const std::string& channel, const std::string& action) const = 0;
};

#endif

