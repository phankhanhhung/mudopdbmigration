#include "app/app.hpp"

#include <iostream>
#include <string>

#include "helper/utility.hpp"
#include "helper/query_update.hpp"
#include "api/driver.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"

int run_app(const std::string& connection_string) {
  std::string s = connection_string;

  bool is_embedded = (s.find("//") == std::string::npos);
  std::unique_ptr<Driver> driver;
  if (is_embedded) {
    driver = std::make_unique<EmbeddedDriver>();
  } else {
    driver = std::make_unique<NetworkDriver>();
  }

  std::unique_ptr<Connection> conn = driver->connect(s);
  std::unique_ptr<Statement> stmt = conn->create_statement();

  std::cout << "\nSQL> " << std::flush;
  while (std::getline(std::cin, s)) {
    trim_end(s);
    if (s == "exit" || s == "quit") break;
    if (s.rfind("select", 0) == 0) {
      do_query(stmt.get(), s);
    } else {
      do_update(stmt.get(), s);
    }
    s.clear();
    std::cout << "\nSQL> " << std::flush;
  }
  conn->close();
  return 0;
}
