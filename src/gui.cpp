#include "gui.hpp"
#include <iostream>                       // for operator<<, endl, basic_ost...
#include "utils.hpp"                      // for bindMember
#include "websocketpp/error.hpp"          // for exception
#include "websocketpp/logger/levels.hpp"  // for alevel, alevel::all
namespace asio { class io_context; }  // lines 10-10

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
    std::cerr << e.what() << std::endl;
  } catch (...) {
    std::cerr << "other exception" << std::endl;
  }
}

