#include <gtest/gtest.h>
#include <string>
#include <memory>
#include "api/driver.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"

TEST(Drivers, EmbeddedConnectCreatesStatementAndCloses) {
  EmbeddedDriver d;
  // Pass a simple database name (not a URL with //) to use embedded driver
  auto conn = d.connect("testdb");
  ASSERT_NE(conn, nullptr);
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
  EXPECT_NO_THROW(conn->close());
}

TEST(Drivers, NetworkConnectFailsWhenNoServer) {
  NetworkDriver d;
  // No server running - connection should fail
  EXPECT_THROW(d.connect("127.0.0.1:19999"), std::runtime_error);
}
