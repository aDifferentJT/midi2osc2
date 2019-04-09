#include "osc.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

asio::io_context OSC::io_context;

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

template <size_t I = 0, typename V>
std::vector<char> data_from_variant(V var) {
  size_t i = var.index();
  if constexpr (I >= std::variant_size_v<V>) {
    return std::vector<char>();
  } else if (i == I) {
    std::variant_alternative_t<I, V> v = std::get<I>(var);
    size_t n = sizeof(std::variant_alternative_t<I, V>);
    return std::vector<char>(&v, &v + n);
  } else {
    return data_from_variant<I+1>(var);
  }
}

template <typename It>
std::vector<char> concatIt(It data) {
  size_t n = std::accumulate(data.begin(), data.end(), 0, [](size_t a, std::vector<char> b){ return a + b.size(); });
  std::vector<char> v(n);
  auto it = v.begin();
  for (std::vector<char> d : data) {
    std::copy(d.begin(), d.end(), it);
    it += d.size();
  }
  return v;
}

template <typename... Types>
size_t lengthOfDatas() { return 0; }
template <typename... Types>
size_t lengthOfDatas(std::vector<char> data, Types... datas) { return data.size() + lengthOfDatas(datas...); }

template <typename It, typename... Types>
void copyDatas(It it) {}
template <typename It, typename... Types>
void copyDatas(It it, std::vector<char> data, Types... datas) {
  std::copy(data.begin(), data.end(), it);
  copyDatas(it + data.size(), datas...);
}

template <typename... Types>
std::vector<char> concat(Types... datas) {
  size_t n = lengthOfDatas(datas...);
  std::vector<char> v(n);
  copyDatas(v.begin(), datas...);
  return v;
}

std::vector<char> OSC::Message::makeArgument(Argument arg) {
  //return data_from_variant(arg);
  return std::visit(overloaded
      { [](int32_t v) { return makeOSCnum(v); }
      , [](float v) { return makeOSCnum(v); }
      , [](std::string v) { return makeOSCstring(v); }
      , [](std::vector<char> v) { return makeOSCstring(concat(makeOSCnum((int32_t)v.size()), v)); }
      }, arg);
}

std::vector<char> OSC::Message::makeArguments(std::vector<Argument> arg) {
  std::vector<std::vector<char>> data;
  std::transform(arg.begin(), arg.end(), std::back_inserter(data), makeArgument);
  return concatIt(data);
}

std::vector<char> OSC::Message::toPacket() {
  return concat( makeOSCstring(addressPattern)
      , makeTypeTagString(types)
      , makeArguments(arguments)
      );
}

OSC::Message::Message(std::vector<char> packet) {
  auto addressPatternEnd = std::find(packet.begin(), packet.end(), '\0');
  addressPattern = std::string(packet.begin(), addressPatternEnd);
  size_t addressPatternPad = 3 - ((addressPatternEnd - packet.begin() + 3) % 4);
  auto typeTagStringBegin = addressPatternEnd + addressPatternPad;
  if (*typeTagStringBegin != ',') {
    std::cerr << "Missing Type Tag String" << std::endl;
    return;
  } else {
    auto typeTagStringEnd = std::find(typeTagStringBegin, packet.end(), '\0');
    types.resize(typeTagStringEnd - (typeTagStringBegin + 1));
    std::transform(typeTagStringBegin + 1, typeTagStringEnd, types.begin(), [](char t){ switch (t) {
        case 'i': return Type::i;
        case 'f': return Type::f;
        case 's': return Type::s;
        case 'b': return Type::b;
        default: throw;
        }});
    size_t typeTagStringPad = 3 - ((typeTagStringEnd - typeTagStringBegin + 3) % 4);
    char* argumentsIt = &*(typeTagStringEnd + typeTagStringPad);
    for (Type t : types) {
      switch (t) {
        case Type::i:
          arguments.push_back(*(int32_t*)argumentsIt);
          argumentsIt += sizeof(int32_t);
          break;
        case Type::f:
          arguments.push_back(*(float*)argumentsIt);
          argumentsIt += sizeof(float);
          break;
        case Type::s:
          {
            std::string str(argumentsIt);
            arguments.push_back(str);
            size_t n = str.size();
            size_t pad = 3 - ((n + 3) % 4);
            argumentsIt += n + pad;
          }
          break;
        case Type::b:
          {
            int32_t n = *(int32_t*)argumentsIt;
            argumentsIt += sizeof(int32_t);
            std::vector<char> v(argumentsIt, argumentsIt + n);
            arguments.push_back(v);
            size_t pad = 3 - ((n + 3) % 4);
            argumentsIt += n + pad;
          }
          break;
      }
    }
  }
}

void OSC::init() {
  asio::executor_work_guard<asio::io_context::executor_type> work
    = asio::make_work_guard(io_context);
  std::thread t([](){ io_context.run(); });
  t.detach();
}

using namespace std::placeholders;

void OSC::recvHandler(const asio::error_code& error, size_t count_recv) {
  recvCallback(Message(recvBuffer));
  socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, std::bind(&OSC::recvHandler, this, _1, _2));
}

OSC::OSC(std::string ip, unsigned short sendPort, unsigned short recvPort, std::function<void(Message)> recvCallback)
  : sendPort(sendPort)
  , address(asio::ip::make_address(ip.c_str()))
  , socket(io_context, udp::endpoint(asio::ip::address(asio::ip::address_v4::any()), recvPort))
  , recvCallback(recvCallback)
    , recvBuffer(1024)
{
  socket.async_receive_from(asio::buffer(recvBuffer), recvEndpoint, std::bind(&OSC::recvHandler, this, _1, _2));
}

void OSC::send(Message message) {
  socket.send_to(asio::buffer(message.toPacket()), udp::endpoint(address, sendPort));
}

