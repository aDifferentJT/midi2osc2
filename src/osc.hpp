#ifndef OSC_h
#define OSC_h

#include "output.hpp"

#define ASIO_STANDALONE
#include <asio.hpp>

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using asio::ip::udp;

class OSC : public Output {
  public:
    struct Message {
      public:
        enum class Type {
          i, f, s, b
        };
        static_assert(sizeof(float) == 4, "Float is not 32 bits");
        using Argument = std::variant<int32_t, float, std::string, std::vector<char>>;
        std::string addressPattern;
        std::vector<Type> types;
        std::vector<Argument> arguments;
        Message(std::string  addressPattern, std::vector<Type>  types, std::vector<Argument>  arguments)
          : addressPattern(std::move(std::move(addressPattern))), types(std::move(std::move(types))), arguments(std::move(std::move(arguments))) {}
      private:
        template <typename T>
          static std::vector<char> makeOSCnum(T v) {
            char* p = (char*)&v;
            size_t n = sizeof(T);
            std::vector<char> res(p, p + n);
            if (std::endian::native == std::endian::little) {
              std::reverse(res.begin(), res.end());
            }
            return res;
          }
        template <typename T>
          static T unmakeOSCnum(std::vector<char> v) {
            if (std::endian::native == std::endian::little) {
              std::reverse(v.begin(), v.end());
            }
            return *(T*)v.begin();
          }
        template <typename T>
          static T unmakeOSCnum(char* v) {
            size_t n = sizeof(T);
            if (std::endian::native == std::endian::little) {
              std::reverse(v, v + n);
            }
            return *(T*)v;
          }
        template <typename T>
          static std::vector<char> makeOSCstring(T str) {
            size_t pad = 3 - ((str.size() + 3) % 4);
            std::vector<char> v(str.begin(), str.end());
            for(size_t i = 0; i < pad; i++) {
              v.push_back('\0');
            }
            return v;
          }
        static std::vector<char> makeTypeTagString(std::vector<Type> types);
        static std::vector<char> makeArgument(Argument arg);
        static std::vector<char> makeArguments(std::vector<Argument> arg);
      public:
        explicit Message(std::string addressPattern) : addressPattern(std::move(std::move(addressPattern))) {}
        template <typename... Args>
          Message(std::string addressPattern, int arg, Args... args) : Message(std::move(addressPattern), args...) {
            types.insert(types.begin(), Type::i);
            arguments.insert(arguments.begin(), arg);
          }
        template <typename... Args>
          Message(std::string addressPattern, float arg, Args... args) : Message(addressPattern, args...) {
            types.insert(types.begin(), Type::f);
            arguments.insert(arguments.begin(), arg);
          }
        template <typename... Args>
          Message(std::string addressPattern, std::string arg, Args... args) : Message(addressPattern, args...) {
            types.insert(types.begin(), Type::s);
            arguments.insert(arguments.begin(), arg);
          }
        template <typename... Args>
          Message(std::string addressPattern, std::vector<char> arg, Args... args) : Message(addressPattern, args...) {
            types.insert(types.begin(), Type::b);
            arguments.insert(arguments.begin(), arg);
          }
        std::vector<char> toPacket();
        explicit Message(std::vector<char> packet);
        float toFloat();
    };
  private:
    asio::io_context& io_context;

    const unsigned short sendPort;
    const asio::ip::address address;
    udp::socket socket;

    std::function<void(Message)> callback = [](Message m){ (void)m; };
    std::vector<char> recvBuffer;
    udp::socket::endpoint_type recvEndpoint;
    void recvHandler(const asio::error_code& error, size_t count_recv);
  public:
    OSC(asio::io_context& io_context, const std::string& ip, unsigned short sendPort, unsigned short recvPort);
    void send(Message message);
    void send(std::string addressPattern, float arg) override { send(Message(addressPattern, arg)); }
    template <typename... Args>
      void send(std::string addressPattern, Args... args) { send(Message(addressPattern, args...)); }
    void setCallback(std::function<void(Message)> f) { callback = std::move(f); }
    void setCallback(std::function<void(std::string, float)> f) override { callback = [f](Message msg) { f(msg.addressPattern, msg.toFloat()); }; }
};

#endif

