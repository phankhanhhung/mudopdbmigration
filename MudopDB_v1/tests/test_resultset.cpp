#include <gtest/gtest.h>
#include "api/result_set.hpp"
#include "api/connection.hpp"
#include "api/statement.hpp"
#include "api/metadata.hpp"
#include "plan/plan.hpp"
#include "query/scan.hpp"
#include <memory>

// ============================================================================
// Test EmbeddedResultSet creation
// ============================================================================

TEST(EmbeddedResultSet, CreateWithNullPlan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_NE(rs, nullptr);
}

// ============================================================================
// Test EmbeddedResultSet methods with no scan
// ============================================================================

TEST(EmbeddedResultSet, NextWithNoScan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_FALSE(rs->next());
}

TEST(EmbeddedResultSet, GetIntWithNoScan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_EQ(rs->get_int("id"), 0);
}

TEST(EmbeddedResultSet, GetStringWithNoScan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_EQ(rs->get_string("name"), "");
}

TEST(EmbeddedResultSet, GetMetaData) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  auto metadata = rs->get_meta_data();
  EXPECT_NE(metadata, nullptr);
}

TEST(EmbeddedResultSet, Close) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_NO_THROW(rs->close());
}

// ============================================================================
// Test field name case conversion
// ============================================================================

TEST(EmbeddedResultSet, FieldNameLowercase) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  EXPECT_NO_THROW(rs->get_int("ID"));
  EXPECT_NO_THROW(rs->get_int("Id"));
  EXPECT_NO_THROW(rs->get_int("id"));

  EXPECT_NO_THROW(rs->get_string("NAME"));
  EXPECT_NO_THROW(rs->get_string("Name"));
  EXPECT_NO_THROW(rs->get_string("name"));
}

// ============================================================================
// Test ResultSet lifecycle
// ============================================================================

TEST(EmbeddedResultSet, FullLifecycle) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  EXPECT_FALSE(rs->next());
  EXPECT_EQ(rs->get_int("id"), 0);
  EXPECT_EQ(rs->get_string("name"), "");

  auto metadata = rs->get_meta_data();
  EXPECT_NE(metadata, nullptr);

  rs->close();
}

// ============================================================================
// Test polymorphic ResultSet usage
// ============================================================================

TEST(ResultSet, PolymorphicUsage) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  std::unique_ptr<ResultSet> rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  EXPECT_NE(rs, nullptr);
  EXPECT_FALSE(rs->next());
}

// ============================================================================
// Test NetworkResultSet (basic functionality)
// ============================================================================

TEST(NetworkResultSet, Create) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  EXPECT_NE(rs, nullptr);
}

TEST(NetworkResultSet, NextReturnsFalse) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  EXPECT_FALSE(rs->next());
}

TEST(NetworkResultSet, GetIntReturnsZero) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  EXPECT_EQ(rs->get_int("id"), 0);
}

TEST(NetworkResultSet, GetStringReturnsEmpty) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  EXPECT_EQ(rs->get_string("name"), "");
}

TEST(NetworkResultSet, GetMetaData) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  auto metadata = rs->get_meta_data();
  EXPECT_NE(metadata, nullptr);
}

TEST(NetworkResultSet, Close) {
  auto conn = std::make_shared<NetworkConnection>();
  auto rs = std::make_unique<NetworkResultSet>(conn, 123);
  EXPECT_NO_THROW(rs->close());
}

// ============================================================================
// Test error handling with exceptions
// ============================================================================

TEST(EmbeddedResultSet, HandlesExceptionsGracefully) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  EXPECT_NO_THROW({
    rs->next();
    rs->get_int("field");
    rs->get_string("field");
    rs->close();
  });
}
