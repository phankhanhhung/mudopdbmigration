#include <gtest/gtest.h>
#include "record/rid.hpp"

using namespace record;

// ============================================================================
// RID Tests
// ============================================================================

TEST(RIDTest, Construction) {
    RID rid(3, 5);

    EXPECT_EQ(rid.block_number(), 3);
    EXPECT_EQ(rid.slot(), 5);
}

TEST(RIDTest, Equality) {
    RID rid1(3, 5);
    RID rid2(3, 5);
    RID rid3(3, 6);
    RID rid4(4, 5);

    EXPECT_TRUE(rid1 == rid2);
    EXPECT_FALSE(rid1 != rid2);

    EXPECT_FALSE(rid1 == rid3);
    EXPECT_TRUE(rid1 != rid3);

    EXPECT_FALSE(rid1 == rid4);
    EXPECT_TRUE(rid1 != rid4);
}

TEST(RIDTest, ToString) {
    RID rid(10, 25);

    std::string str = rid.to_string();

    EXPECT_NE(str.find("10"), std::string::npos);
    EXPECT_NE(str.find("25"), std::string::npos);
}

// main() is provided by gtest_main
