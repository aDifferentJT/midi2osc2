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
// IWYU pragma: no_include <bits/stdint-uintn.h>
#include <cstdint> // IWYU pragma: keep

#include <algorithm>    // for reverse, find, transform
#include <chrono>       // for seconds
#include <functional>   // for function
#include <iostream>     // for size_t, operator<<, endl, basic_ostream, cerr
#include <string>       // for string, allocator, operator==, basic_string
#include <thread>       // for thread
#include <type_traits>  // for endian, endian::little, endian::native, remov...
#include <utility>      // for move, pair
#include <variant>      // for variant
#include <vector>       // for vector, vector<>::iterator
#include "output.hpp"   // for Output
#include "utils.hpp"    // for bindMember, visit

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
        Message(std::string addressPattern, std::vector<Type> types, std::vector<Argument> arguments)
          : addressPattern(std::move(addressPattern)), types(std::move(types)), arguments(std::move(arguments)) {}
      private:
        template <typename T> static constexpr Type forType();
        template <> constexpr Type forType<int>              () { return Type::i; }
        template <> constexpr Type forType<float>            () { return Type::f; }
        template <> constexpr Type forType<std::string>      () { return Type::s; }
        template <> constexpr Type forType<std::vector<char>>() { return Type::b; }

        template <typename T>
          static std::vector<char> makeOSCnum(T v) {
            char* p = (char*)&v;
            std::size_t n = sizeof(T);
            std::vector<char> res(p, p + n);
            if (endian::native == endian::little) {
              std::reverse(res.begin(), res.end());
            }
            return res;
          }
        template <typename T>
          static T unmakeOSCnum(std::vector<char> v) {
            if (endian::native == endian::little) {
              std::reverse(v.begin(), v.end());
            }
            return *(T*)v.begin();
          }
        template <typename T>
          static T unmakeOSCnum(char* v) {
            std::size_t n = sizeof(T);
            if (endian::native == endian::little) {
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
        template <typename T, typename... Args>
          Message(std::string addressPattern, T arg, Args... args) : Message(std::move(addressPattern), args...) {
            types.insert(types.begin(), forType<T>());
            arguments.insert(arguments.begin(), arg);
          }
      private:
        template <typename It>
          static std::optional<Message> parse(It begin, It end) {
            auto addressPatternEnd = std::find(begin, end, '\0');
            std::string addressPattern(begin, addressPatternEnd);
            std::size_t addressPatternPad = 4 - ((addressPatternEnd - begin) % 4);
            auto typeTagStringBegin = addressPatternEnd + addressPatternPad;
            if (*typeTagStringBegin != ',') {
              std::cerr << "Missing Type Tag String" << std::endl;
              return std::nullopt;
            }
            auto typeTagStringEnd = std::find(typeTagStringBegin, end, '\0');
            std::vector<Type> types(typeTagStringEnd - (typeTagStringBegin + 1));
            try {
              std::transform(typeTagStringBegin + 1, typeTagStringEnd, types.begin(), [](char t){ switch (t) {
                case 'i': return Type::i;
                case 'f': return Type::f;
                case 's': return Type::s;
                case 'b': return Type::b;
                default: throw 0;
              }});
            } catch (int x) {
              return std::nullopt;
            }
            std::size_t typeTagStringPad = 4 - ((typeTagStringEnd - typeTagStringBegin) % 4);
            char* argumentsIt = &*(typeTagStringEnd + typeTagStringPad);
            std::vector<Argument> arguments;
            for (Type t : types) {
              switch (t) {
                case Type::i:
                  arguments.emplace_back(unmakeOSCnum<int32_t>(argumentsIt));
                  argumentsIt += sizeof(int32_t);
                  break;
                case Type::f:
                  arguments.emplace_back(unmakeOSCnum<float>(argumentsIt));
                  argumentsIt += sizeof(float);
                  break;
                case Type::s:
                  {
                    std::string str(argumentsIt);
                    arguments.emplace_back(str);
                    std::size_t n = str.size();
                    std::size_t pad = 4 - (n % 4);
                    argumentsIt += n + pad;
                  }
                  break;
                case Type::b:
                  {
                    int32_t n = *reinterpret_cast<int32_t*>(argumentsIt);
                    argumentsIt += sizeof(int32_t);
                    std::vector<char> v(argumentsIt, argumentsIt + n);
                    arguments.emplace_back(v);
                    std::size_t pad = 4 - (n % 4);
                    argumentsIt += n + pad;
                  }
                  break;
              }
            }
            return Message(std::move(addressPattern), std::move(types), std::move(arguments));
          }

      public:
        template <typename It>
          static std::vector<Message> getMessages(It begin, It end) {
            if (std::string(begin, begin + 8) == "#bundle") {
              auto it = begin + 16;
              std::vector<OSC::Message> messages;
              while (it < end) {
                auto size = unmakeOSCnum<uint32_t>(&*it);
                auto newMessages = getMessages(it, it + size);
                messages.insert(messages.end(), newMessages.begin(), newMessages.end());
              }
              return messages;
            }
            if (auto m = Message::parse(begin, end)) {
              return std::vector({*m});
            }
            return std::vector<Message>();
          }

        static std::vector<Message> getMessages(std::vector<char> packet);
        std::vector<char> toPacket();
        std::optional<float> toFloat();
    };
  private:
    asio::io_context& io_context;

    const unsigned short sendPort;
    const asio::ip::address address;
    udp::socket socket;

    std::function<void(Message)> callback = Const<void>();
    std::vector<char> recvBuffer;
    udp::socket::endpoint_type recvEndpoint;
    void recvHandler(const asio::error_code& error, std::size_t count_recv);

    std::vector<std::thread> periodicSends;
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
    void setCallback(std::function<void(const std::string&, float)> f) override {
      callback = [f](Message msg) {
        if (auto flt = msg.toFloat()) {
          f(msg.addressPattern, *flt);
        }
      };
    }
    std::pair<std::string, bool> merge(const std::string& channel, const std::string& action) const override;
};

#endif

