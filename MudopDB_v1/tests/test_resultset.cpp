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
  EXPECT_THROW(rs->get_int("id"), std::runtime_error);
}

TEST(EmbeddedResultSet, GetStringWithNoScan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);
  EXPECT_THROW(rs->get_string("name"), std::runtime_error);
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

TEST(EmbeddedResultSet, FieldNameLowercaseThrowsWithNoScan) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  // With no scan, get_int/get_string now throw instead of returning defaults
  EXPECT_THROW(rs->get_int("ID"), std::runtime_error);
  EXPECT_THROW(rs->get_string("NAME"), std::runtime_error);
}

// ============================================================================
// Test ResultSet lifecycle
// ============================================================================

TEST(EmbeddedResultSet, FullLifecycle) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  EXPECT_FALSE(rs->next());
  // get_int/get_string now throw with no scan
  EXPECT_THROW(rs->get_int("id"), std::runtime_error);
  EXPECT_THROW(rs->get_string("name"), std::runtime_error);

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
// Test error handling with exceptions
// ============================================================================

TEST(EmbeddedResultSet, ThrowsOnErrorInsteadOfSwallowing) {
  auto conn = std::make_shared<EmbeddedConnection>(nullptr);
  auto rs = std::make_unique<EmbeddedResultSet>(nullptr, conn);

  // next() returns false with no scan (not an error)
  EXPECT_NO_THROW(rs->next());
  // get_int/get_string throw with no scan (was silently returning defaults before)
  EXPECT_THROW(rs->get_int("field"), std::runtime_error);
  EXPECT_THROW(rs->get_string("field"), std::runtime_error);
  EXPECT_NO_THROW(rs->close());
}
