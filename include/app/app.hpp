#pragma once
#include <iosfwd>
#include <string>

struct AppConfig {
  std::string connection_string; // optional preset; empty -> read from input
};

// Run the application logic that main() used to hold.
// Returns 0 on success. Uses provided streams for I/O to enable testing.
int run_app(const AppConfig& cfg, std::istream& in, std::ostream& out);
