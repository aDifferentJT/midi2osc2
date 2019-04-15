#include "mappings.hpp"
#include "midi.hpp"
#include "osc.hpp"
#include "output.hpp"
#include "config.hpp"

#include <cassert>
#include <csignal>
#include <memory>

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

