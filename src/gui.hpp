#ifndef GUI_h
#define GUI_h

#define ASIO_STANDALONE
#include <functional>                              // for function
#include <memory>                                  // for owner_less, allocator
#include <set>                                     // for set
#include <string>                                  // for string
#include <utility>                                 // for move
#include <websocketpp/config/asio_no_tls.hpp>      // for asio
#include <websocketpp/close.hpp>                   // for going_away
#include <websocketpp/common/connection_hdl.hpp>   // for connection_hdl
#include <websocketpp/frame.hpp>                   // for text
#include <websocketpp/endpoint.hpp>      // for endpoint::close
#include <websocketpp/message_buffer/message.hpp>  // for message
#include <websocketpp/roles/server_endpoint.hpp>   // for server
namespace asio { class io_context; }  // lines 13-13

class GUI {
  private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using Message = Server::message_ptr;
    using Connection = websocketpp::connection_hdl;

    Server server;
    std::set<Connection,std::owner_less<Connection>> connections;

    std::function<void()> openCallback;
    std::function<void(const std::string&)> recvCallback;

    void openHandler(const Connection& connection) {
      connections.insert(connection);
      openCallback();
    }
    void closeHandler(const Connection& connection) {
      connections.erase(connection);
    }
    void recvHandler(const Connection& connection, const Message& message) {
      (void)connection;
      recvCallback(message->get_payload());
    }
  public:
    GUI(asio::io_context& io_context);
    GUI(const GUI&) = delete;
    GUI& operator =(const GUI&) = delete;
    GUI(GUI&& other) = default;
    GUI& operator =(GUI&&) = delete;

    ~GUI() {
      for (const Connection& connection : connections) {
        server.close(connection, websocketpp::close::status::going_away, "Server closing");
      }
      server.stop_listening();
    }


    void send(const std::string& str) {
      for (const Connection& connection : connections) {
        server.send(connection, str.c_str(), str.size(), websocketpp::frame::opcode::text);
      }
    }

    void setOpenCallback(std::function<void()> f) { openCallback = std::move(f); }
    void setRecvCallback(std::function<void(const std::string&)> f) { recvCallback = std::move(f); }
};

#endif
