#ifndef GUI_h
#define GUI_h

#include "utils.hpp"

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>
#include <iostream>
#include <set>
#include <string>

class GUI {
  private:
    using Server = websocketpp::server<websocketpp::config::asio>;
    using Message = Server::message_ptr;
    using Connection = websocketpp::connection_hdl;

    Server server;
    std::set<Connection,std::owner_less<Connection>> connections;

    std::function<void()> openCallback;
    std::function<void(std::string)> recvCallback;

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
    void setRecvCallback(std::function<void(std::string)> f) { recvCallback = std::move(f); }
};

#endif
