#include <gtest/gtest.h>
#include "api/connection.hpp"
#include "api/driver.hpp"
#include "api/statement.hpp"
#include <memory>

// ============================================================================
// Test EmbeddedConnection creation
// ============================================================================

TEST(EmbeddedConnection, CreateWithNullDB) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  EXPECT_NE(conn, nullptr);
}

// ============================================================================
// Test EmbeddedConnection methods
// ============================================================================

TEST(EmbeddedConnection, CreateStatement) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
}

TEST(EmbeddedConnection, Close) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  EXPECT_NO_THROW(conn->close());
}

TEST(EmbeddedConnection, CommitNoTransaction) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  EXPECT_NO_THROW(conn->commit());
}

TEST(EmbeddedConnection, RollbackNoTransaction) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  EXPECT_NO_THROW(conn->rollback());
}

TEST(EmbeddedConnection, GetTransactionReturnsNull) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  auto tx = conn->get_transaction();
  EXPECT_EQ(tx, nullptr);
}

TEST(EmbeddedConnection, PlannerReturnsNull) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  auto planner = conn->planner();
  EXPECT_EQ(planner, nullptr);
}

// ============================================================================
// Test EmbeddedConnection lifecycle
// ============================================================================

TEST(EmbeddedConnection, FullLifecycle) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  ASSERT_NE(conn, nullptr);

  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);

  EXPECT_NO_THROW(conn->commit());
  EXPECT_NO_THROW(conn->rollback());
  EXPECT_NO_THROW(conn->close());
}

// ============================================================================
// Test connection sharing
// ============================================================================

TEST(EmbeddedConnection, SharedPointerUsage) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
}

// ============================================================================
// Test multiple statements from same connection
// ============================================================================

TEST(EmbeddedConnection, MultipleStatements) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);

  auto stmt1 = conn->create_statement();
  auto stmt2 = conn->create_statement();

  EXPECT_NE(stmt1, nullptr);
  EXPECT_NE(stmt2, nullptr);
  EXPECT_NE(stmt1.get(), stmt2.get());
}

// ============================================================================
// Test NetworkConnection (requires server - connection failure expected)
// ============================================================================

TEST(NetworkConnection, ConnectFailsWhenNoServer) {
  EXPECT_THROW(
    NetworkConnection("127.0.0.1", 19999),
    std::runtime_error
  );
}
