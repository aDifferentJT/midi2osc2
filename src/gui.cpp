#include "gui.hpp"

using namespace std::placeholders;

void GUI::openHandler(Connection connection) {
  connections.insert(connection);
  openCallback();
}

void GUI::closeHandler(Connection connection) {
  connections.erase(connection);
}

void GUI::recvHandler(Connection connection, Message message) {
  (void)connection;
  recvCallback(message->get_payload());
}

GUI::GUI(asio::io_context& io_context) {
  try {
    server.clear_access_channels(websocketpp::log::alevel::all);
    server.init_asio(&io_context);
    server.set_open_handler(std::bind(&GUI::openHandler, this, _1));
    server.set_close_handler(std::bind(&GUI::closeHandler, this, _1));
    server.set_message_handler(std::bind(&GUI::recvHandler, this, _1, _2));
    server.listen(8080);
    server.start_accept();
  } catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "other exception" << std::endl;
  }
}

GUI::~GUI() {
  for (Connection connection : connections) {
    server.close(connection, websocketpp::close::status::going_away, "Server closing");
  }
  server.stop_listening();
}

void GUI::send(std::string str) {
  for (Connection connection : connections) {
    server.send(connection, str.c_str(), str.size(), websocketpp::frame::opcode::text);
  }
}

