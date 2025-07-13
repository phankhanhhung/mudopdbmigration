#include "app/app.hpp"

#include <iostream>
#include <memory>
#include <string>

#include "helper/utility.hpp"
#include "helper/query_update.hpp"
#include "api/driver.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"

int run_app(const AppConfig& cfg, std::istream& in, std::ostream& out) {
  std::string s;

  // Connection string: from cfg if provided, otherwise prompt and read.
  if (!cfg.connection_string.empty()) {
    s = cfg.connection_string;
  } else {
    out << "Connect> ";
    std::getline(in, s);
  }
  trim_end(s);

  // Determine which driver to use based on the connection string.
  bool is_embedded = (s.find("//") == std::string::npos);
  std::unique_ptr<Driver> driver;
  if (is_embedded) {
    driver = std::make_unique<EmbeddedDriver>();
  } else {
    driver = std::make_unique<NetworkDriver>();
  }

  // Establish the connection
  std::unique_ptr<Connection> conn = driver->connect(s);
  s.clear();

  // Create a statement from the connection.
  std::unique_ptr<Statement> stmt = conn->create_statement();

  // Enter SQL command loop.
  out << "\nSQL> " << std::flush;
  while (std::getline(in, s)) {
    trim_end(s);
    if (s == "exit" || s == "quit") {
      break;
    }
    if (s.rfind("select", 0) == 0) {
      do_query(stmt.get(), s);
    } else {
      do_update(stmt.get(), s);
    }
    s.clear();
    out << "\nSQL> " << std::flush;
  }
  conn->close();

  return 0;
}
