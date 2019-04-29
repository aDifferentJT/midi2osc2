#ifndef OSC_h
#define OSC_h

#define ASIO_STANDALONE
#include <asio.hpp> // IWYU pragma: keep
// IWYU pragma: no_include <asio/error_code.hpp>
// IWYU pragma: no_include <asio/io_context.hpp>
// IWYU pragma: no_include <asio/ip/address.hpp>
// IWYU pragma: no_include <asio/ip/udp.hpp>
// IWYU pragma: no_forward_declare asio::io_context

// IWYU pragma: no_include <bits/stdint-intn.h>
#include <cstdint> // IWYU pragma: keep

#include <algorithm>            // for reverse
#include <chrono>               // for seconds
#include <functional>           // for function
#include <iosfwd>               // for size_t
#include <string>               // for string
#include <type_traits>          // for endian, endian::little, endian::native
#include <utility>              // for move, pair
#include <variant>              // for variant
#include <vector>               // for vector, vector<>::iterator
#include "output.hpp"           // for Output

using asio::ip::udp;

class OSC final : public Output {
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
            std::size_t n = sizeof(T);
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
            std::size_t n = sizeof(T);
            if (std::endian::native == std::endian::little) {
              std::reverse(v, v + n);
            }
            return *(T*)v;
          }
        template <typename T>
          static std::vector<char> makeOSCstring(T str) {
            std::size_t pad = 4 - (str.size() % 4);
            std::vector<char> v(str.begin(), str.end());
            for(std::size_t i = 0; i < pad; i++) {
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
    void recvHandler(const asio::error_code& error, std::size_t count_recv);
  public:
    OSC(asio::io_context& io_context, const std::string& ip, unsigned short sendPort, unsigned short recvPort);
    void send(Message message);
    void send(const std::string& addressPattern) override { send(Message(addressPattern)); }
    void send(const std::string& addressPattern, float arg) override { send(Message(addressPattern, arg)); }
    template <typename... Args>
      void send(const std::string& addressPattern, Args... args) { send(Message(addressPattern, args...)); }
    void sendPeriodically(const Message& message, std::chrono::seconds period = std::chrono::seconds(9));
    void sendPeriodically(const std::string& addressPattern, std::chrono::seconds period = std::chrono::seconds(9)) override { sendPeriodically(Message(addressPattern), period); }
    void sendPeriodically(const std::string& addressPattern, float arg, std::chrono::seconds period = std::chrono::seconds(9)) override { sendPeriodically(Message(addressPattern, arg), period); }
    template <typename... Args>
      void sendPeriodically(const std::string& addressPattern, Args... args, std::chrono::seconds period = std::chrono::seconds(9)) { sendPeriodically(Message(addressPattern, args...), period); }
    void setCallback(std::function<void(Message)> f) { callback = std::move(f); }
    void setCallback(std::function<void(const std::string&, float)> f) override { callback = [f](Message msg) { f(msg.addressPattern, msg.toFloat()); }; }
    std::pair<std::string, bool> merge(const std::string& channel, const std::string& action) const override;
};

#endif

