#include "helper/utility.hpp"
#include <gtest/gtest.h>

TEST(UtilityTrim, RemovesTrailingSpaces) {
  std::string s = "hello   ";
  trim_end(s);
  EXPECT_EQ(s, "hello");
}