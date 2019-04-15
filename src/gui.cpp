#include "gui.hpp"

using namespace std::placeholders;

void GUI::openHandler(const Connection& connection) {
  connections.insert(connection);
  openCallback();
}

void GUI::closeHandler(const Connection& connection) {
  connections.erase(connection);
}

void GUI::recvHandler(const Connection& connection, const Message& message) {
  (void)connection;
  recvCallback(message->get_payload());
}

GUI::GUI(asio::io_context& io_context) {
  try {
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.init_asio(&io_context);
    server.set_reuse_addr(true);
    server.set_open_handler(bindMember(&GUI::openHandler, this));
    server.set_close_handler(bindMember(&GUI::closeHandler, this));
    server.set_message_handler(bindMember(&GUI::recvHandler, this));
    server.listen(8080);
    server.start_accept();
  } catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }
}

GUI::~GUI() {
  for (const Connection& connection : connections) {
    server.close(connection, websocketpp::close::status::going_away, "Server closing");
  }
  server.stop_listening();
}

void GUI::send(const std::string& str) {
  for (const Connection& connection : connections) {
    server.send(connection, str.c_str(), str.size(), websocketpp::frame::opcode::text);
  }
}

