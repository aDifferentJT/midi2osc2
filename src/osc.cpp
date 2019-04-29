// IWYU pragma: no_include <asio/buffer.hpp>
// IWYU pragma: no_include <asio/ip/address_v4.hpp>
// IWYU pragma: no_include <asio/ip/impl/address.ipp>

// IWYU pragma: no_include <bits/stdint-intn.h>
#include <cstdint> // IWYU pragma: keep

#include "osc.hpp"
#include <chrono>        // for duration, steady_clock, operator+, seconds
#include <iostream>      // for operator<<, endl, basic_ostream, cerr
#include <iterator>      // for back_insert_iterator, back_inserter
#include <new>           // for operator new
#include <numeric>       // for accumulate
#include <ratio>         // for ratio
#include <system_error>  // for error_code
#include <thread>        // for sleep_until, thread
#include "utils.hpp"     // for bindMember

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::vector<char> OSC::Message::makeTypeTagString(std::vector<Type> types) {
  std::string str = ",";
  for (Type t : types) {
    switch (t) {
      case Type::i: str += "i"; break;
      case Type::f: str += "f"; break;
      case Type::s: str += "s"; break;
      case Type::b: str += "b"; break;
    }
  }
  return makeOSCstring(str);
}

template <typename It>
std::vector<char> concatIt(It data) {
  std::size_t n = std::accumulate(data.begin(), data.end(), 0, [](std::size_t a, std::vector<char> b){ return a + b.size(); });
  std::vector<char> v(n);
  auto it = v.begin();
  for (std::vector<char> d : data) {
    std::copy(d.begin(), d.end(), it);
    it += d.size();
  }
  return v;
}

template <typename... Types>
std::size_t lengthOfDatas() { return 0; }
template <typename... Types>
std::size_t lengthOfDatas(const std::vector<char>& data, Types... datas) { return (data.size() + lengthOfDatas(datas...)); }

template <typename It, typename... Types>
void copyDatas(It it) { (void)it; }
template <typename It, typename... Types>
void copyDatas(It it, std::vector<char> data, Types... datas) {
  std::copy(data.begin(), data.end(), it);
  copyDatas(it + data.size(), datas...);
}

template <typename... Types>
std::vector<char> concat(Types... datas) {
  std::size_t n = lengthOfDatas(datas...);
  std::vector<char> v(n);
  copyDatas(v.begin(), datas...);
  return v;
}

std::vector<char> OSC::Message::makeArgument(Argument arg) {
  return std::visit(overloaded
      { [](int32_t v)           -> std::vector<char> { return makeOSCnum(v); }
      , [](float v)             -> std::vector<char> { return makeOSCnum(v); }
      , [](std::string v)       -> std::vector<char> { return makeOSCstring(v); }
      , [](std::vector<char> v) -> std::vector<char> { return makeOSCstring(concat(makeOSCnum(static_cast<int32_t>(v.size())), v)); }
      }, std::move(arg));
}

std::vector<char> OSC::Message::makeArguments(std::vector<Argument> arg) {
  std::vector<std::vector<char>> data;
  std::transform(arg.begin(), arg.end(), std::back_inserter(data), makeArgument);
  return concatIt(data);
}

OSC::Message::Message(std::vector<char> packet) {
  auto addressPatternEnd = std::find(packet.begin(), packet.end(), '\0');
  addressPattern = std::string(packet.begin(), addressPatternEnd);
  std::size_t addressPatternPad = 4 - ((addressPatternEnd - packet.begin()) % 4);
  auto typeTagStringBegin = addressPatternEnd + addressPatternPad;
  if (*typeTagStringBegin != ',') {
    std::cerr << "Missing Type Tag String" << std::endl;
    return;
  }
  auto typeTagStringEnd = std::find(typeTagStringBegin, packet.end(), '\0');
  types.resize(typeTagStringEnd - (typeTagStringBegin + 1));
  std::transform(typeTagStringBegin + 1, typeTagStringEnd, types.begin(), [](char t){ switch (t) {
      case 'i': return Type::i;
      case 'f': return Type::f;
      case 's': return Type::s;
      case 'b': return Type::b;
      default: throw;
      }});
  std::size_t typeTagStringPad = 4 - ((typeTagStringEnd - typeTagStringBegin) % 4);
  char* argumentsIt = &*(typeTagStringEnd + typeTagStringPad);
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
}

std::vector<char> OSC::Message::toPacket() {
  return concat( makeOSCstring(addressPattern)
      , makeTypeTagString(types)
      , makeArguments(arguments)
      );
}

float OSC::Message::toFloat() {
  if (arguments.size() != 1) {
    throw;
  }
  return std::visit(overloaded
      { [](int32_t v)           -> float { return static_cast<float>(v); }
      , [](float v)             -> float { return v; }
      , [](std::string v)       -> float { (void)v; throw; }
      , [](std::vector<char> v) -> float { (void)v; throw; }
      }, arguments[0]);
}

void OSC::recvHandler(const asio::error_code& error, std::size_t count_recv) {
  (void)error;
  (void)count_recv;
  callback(Message(recvBuffer));
  socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, bindMember(&OSC::recvHandler, this));
}

OSC::OSC(asio::io_context& io_context, const std::string& ip, unsigned short sendPort, unsigned short recvPort)
  : io_context(io_context)
  , sendPort(sendPort)
  , address(asio::ip::make_address(ip.c_str()))
  , socket(io_context, udp::endpoint(asio::ip::address(asio::ip::address_v4::any()), recvPort))
    , recvBuffer(1024)
{
  socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, bindMember(&OSC::recvHandler, this));
}

void OSC::send(Message message) {
  socket.send_to(asio::buffer(message.toPacket()), udp::endpoint(address, sendPort));
}

void OSC::sendPeriodically(const Message& message, std::chrono::seconds period) {
  std::thread([this, message, period]() {
      for (;;) {
      auto start = std::chrono::steady_clock::now();
      send(message);
      std::this_thread::sleep_until(start + period);
      }
      }).detach();
}

std::pair<std::string, bool> OSC::merge(const std::string& channel, const std::string& action) const {
  std::size_t channelTypeEnd = channel.find('$');
  std::string channelType = channel.substr(0, channelTypeEnd);
  std::string channelArg = channel.substr(channelTypeEnd + 1);
  std::string path;
  bool inverted = false;
  if (channelType == "input") {
    int num = std::stoi(channelArg);
    if (action == "gain") {
      path += "/headamp/";
    } else {
      path += "/ch/";
    }
    if (num < 10) {
      path += '0';
    }
    path += std::to_string(num);
  } else if (channelType == "aux") {
    if (action == "gain") {
      path += "/headamp/17";
    } else {
      path += "/rtn/aux";
    }
  } else if (channelType == "lr") {
    path += "/lr";
  } else if (channelType == "dca") {
    path += "/dca/";
    path += std::to_string(std::stoi(channelArg));
  }
  if (action == "fader") {
    if (channelType == "dca") {
      path += "/fader";
    } else {
      path += "/mix/fader";
    }
  } else if (action == "mute") {
    if (channelType == "dca") {
      path += "/on";
    } else {
      path += "/mix/on";
    }
    inverted = true;
  } else if (action == "on") {
    if (channelType == "dca") {
      path += "/on";
    } else {
      path += "/mix/on";
    }
  } else if (action == "gain") {
    path += "/gain";
  }
  return std::make_pair(path, inverted);
}

