// IWYU pragma: no_include <asio/buffer.hpp>
// IWYU pragma: no_include <asio/ip/address_v4.hpp>
// IWYU pragma: no_include <asio/ip/impl/address.ipp>

#include "osc.hpp"
#include <chrono>        // for duration, steady_clock, operator+, seconds
#include <iterator>      // for back_insert_iterator, back_inserter
#include <new>           // for operator new
#include <numeric>       // for accumulate
#include <ratio>         // for ratio
#include <system_error>  // for error_code
#include <thread>        // for thread, sleep_until

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

template <typename... Vs>
std::size_t lengthOfDatas() { return 0; }
template <typename V, typename... Vs>
std::size_t lengthOfDatas(V v, Vs... vs) { return (v.size() + lengthOfDatas(vs...)); }

template <typename It, typename... Vs>
void copyDatas(It it) { (void)it; }
template <typename It, typename V, typename... Vs>
void copyDatas(It it, V v, Vs... vs) {
  std::copy(v.begin(), v.end(), it);
  copyDatas(it + v.size(), vs...);
}

template <typename... Vs>
std::vector<char> concat(Vs... vs) {
  std::size_t n = lengthOfDatas(vs...);
  std::vector<char> v(n);
  copyDatas(v.begin(), vs...);
  return v;
}

std::vector<char> OSC::Message::makeArgument(Argument arg) {
  return visit<std::vector<char>>(
    std::move(arg),
    std::function(&makeOSCnum<int32_t>),
    std::function(&makeOSCnum<float>),
    std::function(&makeOSCstring<std::string>),
    [](std::vector<char> v) -> std::vector<char> { return makeOSCstring(concat(makeOSCnum<int32_t>(v.size()), v)); }
    );
}

std::vector<char> OSC::Message::makeArguments(std::vector<Argument> arg) {
  std::vector<std::vector<char>> data;
  std::transform(arg.begin(), arg.end(), std::back_inserter(data), makeArgument);
  return concatIt(data);
}

std::vector<char> OSC::Message::toPacket() {
  return concat(
    makeOSCstring(addressPattern),
    makeTypeTagString(types),
    makeArguments(arguments)
    );
}

std::optional<float> OSC::Message::toFloat() {
  if (arguments.size() != 1) {
    std::cerr << "Wrong number of arguments in packet" << std::endl;
    return std::nullopt;
  }
  return visit<std::optional<float>>(
    arguments[0],
    just<float> ^ static_cast_f<float, int32_t>,
    just<float>,
    ConstNullopt<float, std::string>,
    ConstNullopt<float, std::vector<char>>
    );
}

void OSC::recvHandler(const asio::error_code& error, std::size_t count_recv) {
  (void)error;
  (void)count_recv;
  for (const Message& message : Message::getMessages(recvBuffer.begin(), recvBuffer.end())) {
    callback(message);
  }
  socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, bindMember(&OSC::recvHandler, this));
}

OSC::OSC(asio::io_context& io_context, const std::string& ip, unsigned short sendPort, unsigned short recvPort) :
  io_context(io_context),
  sendPort(sendPort),
  address(asio::ip::make_address(ip.c_str())),
  socket(io_context, udp::endpoint(asio::ip::address(asio::ip::address_v4::any()), recvPort)),
  recvBuffer(1024) {
    socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, bindMember(&OSC::recvHandler, this));
  }

void OSC::send(Message message) {
  socket.send_to(asio::buffer(message.toPacket()), udp::endpoint(address, sendPort));
}

void OSC::sendPeriodically(const Message& message, std::chrono::seconds period) {
  periodicSends.emplace_back([this, message, period]() {
    for (;;) {
      auto start = std::chrono::steady_clock::now();
      send(message);
      std::this_thread::sleep_until(start + period);
    }
  });
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

