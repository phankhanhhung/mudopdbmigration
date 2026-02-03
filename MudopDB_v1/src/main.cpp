#include <iostream>
#include "app/app.hpp"

int main(int argc, char** argv) {
  AppConfig cfg{};
  // Optional: allow passing connection string as first argument
  if (argc > 1) {
    cfg.connection_string = argv[1];
  }
  return run_app(cfg, std::cin, std::cout);
}