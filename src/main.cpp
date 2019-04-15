#define ASIO_STANDALONE
#include <asio.hpp> // IWYU pragma: keep
// IWYU pragma: no_include <asio/executor_work_guard.hpp>
// IWYU pragma: no_include <asio/impl/io_context.hpp>
// IWYU pragma: no_include <asio/impl/io_context.ipp>
// IWYU pragma: no_include <asio/io_context.hpp>

#include <csignal>                       // for signal, SIGINT
#include <iostream>                      // for operator<<, endl, basic_ostream
#include <memory>                        // for allocator, make_shared
#include "config.hpp"                    // for Config
#include "gui.hpp"                       // for GUI
#include "mappings.hpp"                  // for Mappings

asio::io_context io_context;

extern "C" void stop(int sig) {
  (void)sig;
  io_context.stop();
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Missing command line argument" << std::endl;
    throw;
  }

  Config config(io_context, argv[1]);

  auto gui = std::make_shared<GUI>(io_context);

  Mappings mappings(config, gui);

  std::signal(SIGINT, &stop);

  asio::executor_work_guard<asio::io_context::executor_type> work
    = asio::make_work_guard(io_context);
  io_context.run();
}

