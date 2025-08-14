#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <functional>
#include "api/driver.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"

static std::string capture(std::function<void()> fn) {
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());
  fn();
  std::cout.rdbuf(old);
  return oss.str();
}

TEST(Drivers, EmbeddedConnectCreatesStatementAndCloses) {
  EmbeddedDriver d;
  auto conn = d.connect("mem://test");
  ASSERT_NE(conn, nullptr);
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
  auto out = capture([&]{ conn->close(); });
  EXPECT_NE(out.find("Connection closed."), std::string::npos);
}

TEST(Drivers, NetworkConnectCreatesStatementAndCloses) {
  NetworkDriver d;
  auto conn = d.connect("tcp://localhost:9999");
  ASSERT_NE(conn, nullptr);
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
  auto out = capture([&]{ conn->close(); });
  EXPECT_NE(out.find("Connection closed."), std::string::npos);
}
