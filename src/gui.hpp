#ifndef GUI_h
#define GUI_h

#define ASIO_STANDALONE
#include <functional>                             // for function
#include <memory>                                 // for owner_less
#include <set>                                    // for set
#include <string>                                 // for string
#include <utility>                                // for move
#include <websocketpp/config/asio_no_tls.hpp>     // for asio
#include "websocketpp/common/connection_hdl.hpp"  // for connection_hdl
#include "websocketpp/roles/server_endpoint.hpp"  // for server
namespace asio { class io_context; }

class GUI {
  private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using Message = Server::message_ptr;
    using Connection = websocketpp::connection_hdl;

    Server server;
    std::set<Connection,std::owner_less<Connection>> connections;

    std::function<void()> openCallback;
    std::function<void(const std::string&)> recvCallback;

    void openHandler(const Connection& connection);
    void closeHandler(const Connection& connection);
    void recvHandler(const Connection& connection, const Message& message);
  public:
    GUI(asio::io_context& io_context);
    GUI(const GUI&) = delete;
    GUI& operator =(const GUI&) = delete;
    GUI(GUI&& other) = default;
    GUI& operator =(GUI&&) = delete;
    ~GUI();

    void send(const std::string& str);

    void setOpenCallback(std::function<void()> f) { openCallback = std::move(f); }
    void setRecvCallback(std::function<void(const std::string&)> f) { recvCallback = std::move(f); }
};

#endif
