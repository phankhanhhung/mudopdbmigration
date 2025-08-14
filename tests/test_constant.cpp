#include <gtest/gtest.h>
#include "query/constant.hpp"
#include <unordered_set>

// ============================================================================
// Test Constant creation with integers
// ============================================================================

TEST(Constant, CreateWithInt) {
  auto c = Constant::with_int(42);
  EXPECT_TRUE(c.as_int().has_value());
  EXPECT_EQ(c.as_int().value(), 42);
  EXPECT_FALSE(c.as_string().has_value());
}

TEST(Constant, CreateWithNegativeInt) {
  auto c = Constant::with_int(-100);
  EXPECT_TRUE(c.as_int().has_value());
  EXPECT_EQ(c.as_int().value(), -100);
}

TEST(Constant, CreateWithZero) {
  auto c = Constant::with_int(0);
  EXPECT_TRUE(c.as_int().has_value());
  EXPECT_EQ(c.as_int().value(), 0);
}

// ============================================================================
// Test Constant creation with strings
// ============================================================================

TEST(Constant, CreateWithString) {
  auto c = Constant::with_string("hello");
  EXPECT_TRUE(c.as_string().has_value());
  EXPECT_EQ(c.as_string().value(), "hello");
  EXPECT_FALSE(c.as_int().has_value());
}

TEST(Constant, CreateWithEmptyString) {
  auto c = Constant::with_string("");
  EXPECT_TRUE(c.as_string().has_value());
  EXPECT_EQ(c.as_string().value(), "");
}

// ============================================================================
// Test to_string conversion
// ============================================================================

TEST(Constant, ToStringWithInt) {
  auto c = Constant::with_int(123);
  EXPECT_EQ(c.to_string(), "123");
}

TEST(Constant, ToStringWithString) {
  auto c = Constant::with_string("test");
  EXPECT_EQ(c.to_string(), "test");
}

// ============================================================================
// Test equality comparison
// ============================================================================

TEST(Constant, EqualityInts) {
  auto c1 = Constant::with_int(42);
  auto c2 = Constant::with_int(42);
  auto c3 = Constant::with_int(43);

  EXPECT_TRUE(c1 == c2);
  EXPECT_FALSE(c1 == c3);
  EXPECT_TRUE(c1 != c3);
}

TEST(Constant, EqualityStrings) {
  auto c1 = Constant::with_string("abc");
  auto c2 = Constant::with_string("abc");
  auto c3 = Constant::with_string("xyz");

  EXPECT_TRUE(c1 == c2);
  EXPECT_FALSE(c1 == c3);
  EXPECT_TRUE(c1 != c3);
}

TEST(Constant, EqualityMixedTypes) {
  auto c_int = Constant::with_int(42);
  auto c_str = Constant::with_string("42");

  EXPECT_FALSE(c_int == c_str);
  EXPECT_TRUE(c_int != c_str);
}

// ============================================================================
// Test ordering comparison
// ============================================================================

TEST(Constant, OrderingInts) {
  auto c1 = Constant::with_int(10);
  auto c2 = Constant::with_int(20);

  EXPECT_TRUE(c1 < c2);
  EXPECT_TRUE(c1 <= c2);
  EXPECT_FALSE(c1 > c2);
  EXPECT_FALSE(c1 >= c2);
  EXPECT_TRUE(c2 > c1);
}

TEST(Constant, OrderingStrings) {
  auto c1 = Constant::with_string("apple");
  auto c2 = Constant::with_string("banana");

  EXPECT_TRUE(c1 < c2);
  EXPECT_TRUE(c1 <= c2);
  EXPECT_FALSE(c1 > c2);
  EXPECT_TRUE(c2 > c1);
}

TEST(Constant, OrderingMixedTypes) {
  auto c_int = Constant::with_int(100);
  auto c_str = Constant::with_string("100");

  // By convention, int < string (based on variant index)
  EXPECT_TRUE(c_int < c_str);
  EXPECT_FALSE(c_str < c_int);
}

// ============================================================================
// Test hash functionality
// ============================================================================

TEST(Constant, HashConsistency) {
  auto c1 = Constant::with_int(42);
  auto c2 = Constant::with_int(42);

  EXPECT_EQ(c1.hash(), c2.hash());
}

TEST(Constant, HashInUnorderedSet) {
  std::unordered_set<Constant> set;

  auto c1 = Constant::with_int(1);
  auto c2 = Constant::with_int(2);
  auto c3 = Constant::with_string("test");

  set.insert(c1);
  set.insert(c2);
  set.insert(c3);
  set.insert(c1); // Duplicate

  EXPECT_EQ(set.size(), 3); // Should have 3 unique elements
  EXPECT_TRUE(set.find(c1) != set.end());
  EXPECT_TRUE(set.find(c2) != set.end());
  EXPECT_TRUE(set.find(c3) != set.end());
}

// ============================================================================
// Test copy and move semantics
// ============================================================================

TEST(Constant, CopyConstructor) {
  auto c1 = Constant::with_int(42);
  auto c2 = c1;

  EXPECT_EQ(c1, c2);
  EXPECT_EQ(c2.as_int().value(), 42);
}

TEST(Constant, AssignmentOperator) {
  auto c1 = Constant::with_int(42);
  auto c2 = Constant::with_string("test");

  c2 = c1;
  EXPECT_EQ(c2.as_int().value(), 42);
  EXPECT_FALSE(c2.as_string().has_value());
}
