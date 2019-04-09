#ifndef Output_h
#define Output_h

#include <string>

class Output {
  public:
    virtual void send(std::string addressPattern, float v) = 0;
};

#endif

