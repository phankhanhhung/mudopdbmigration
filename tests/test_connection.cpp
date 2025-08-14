#include <gtest/gtest.h>
#include "api/connection.hpp"
#include "api/driver.hpp"
#include "api/statement.hpp"
#include <memory>
#include <sstream>

// Helper to capture stdout
static std::string capture(std::function<void()> fn) {
  std::ostringstream oss;
  auto* old = std::cout.rdbuf(oss.rdbuf());
  fn();
  std::cout.rdbuf(old);
  return oss.str();
}

// ============================================================================
// Test EmbeddedConnection creation
// ============================================================================

TEST(EmbeddedConnection, CreateWithNullDB) {
  // Test that connection can be created with null SimpleDB
  // (since SimpleDB is not yet migrated)
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  EXPECT_NE(conn, nullptr);
}

TEST(EmbeddedConnection, CreateViaDriver) {
  EmbeddedDriver driver;
  auto conn = driver.connect("mem://testdb");
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
  auto out = capture([&]{ conn->close(); });
  EXPECT_NE(out.find("Connection closed."), std::string::npos);
}

TEST(EmbeddedConnection, CommitNoTransaction) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  // Should not throw when transaction is null
  EXPECT_NO_THROW(conn->commit());
}

TEST(EmbeddedConnection, RollbackNoTransaction) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  // Should not throw when transaction is null
  EXPECT_NO_THROW(conn->rollback());
}

TEST(EmbeddedConnection, GetTransactionReturnsNull) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  // Since SimpleDB is null, transaction should be null
  auto tx = conn->get_transaction();
  EXPECT_EQ(tx, nullptr);
}

TEST(EmbeddedConnection, PlannerReturnsNull) {
  auto conn = std::make_unique<EmbeddedConnection>(nullptr);
  // Since SimpleDB is null, planner should be null
  auto planner = conn->planner();
  EXPECT_EQ(planner, nullptr);
}

// ============================================================================
// Test EmbeddedConnection lifecycle
// ============================================================================

TEST(EmbeddedConnection, FullLifecycle) {
  EmbeddedDriver driver;
  auto conn = driver.connect("mem://testdb");
  ASSERT_NE(conn, nullptr);

  // Create statement
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);

  // Commit (should not throw)
  EXPECT_NO_THROW(conn->commit());

  // Rollback (should not throw)
  EXPECT_NO_THROW(conn->rollback());

  // Close
  auto out = capture([&]{ conn->close(); });
  EXPECT_NE(out.find("Connection closed."), std::string::npos);
}

// ============================================================================
// Test connection sharing
// ============================================================================

TEST(EmbeddedConnection, SharedPointerUsage) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto stmt = conn->create_statement();

  // Statement creation should not affect connection reference count
  // since it uses a non-owning shared_ptr
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
  EXPECT_NE(stmt1.get(), stmt2.get()); // Different statement objects
}

// ============================================================================
// Test NetworkConnection (basic functionality)
// ============================================================================

TEST(NetworkConnection, CreateStatement) {
  auto conn = std::make_unique<NetworkConnection>();
  auto stmt = conn->create_statement();
  EXPECT_NE(stmt, nullptr);
}

TEST(NetworkConnection, Close) {
  auto conn = std::make_unique<NetworkConnection>();
  auto out = capture([&]{ conn->close(); });
  EXPECT_NE(out.find("Connection closed."), std::string::npos);
}

TEST(NetworkConnection, CommitNoThrow) {
  auto conn = std::make_unique<NetworkConnection>();
  EXPECT_NO_THROW(conn->commit());
}

TEST(NetworkConnection, RollbackNoThrow) {
  auto conn = std::make_unique<NetworkConnection>();
  EXPECT_NO_THROW(conn->rollback());
}
