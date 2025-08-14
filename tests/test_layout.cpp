#include <gtest/gtest.h>
#include "record/layout.hpp"
#include "record/schema.hpp"
#include "file/page.hpp"
#include <memory>

using namespace record;

// ============================================================================
// Layout Tests
// ============================================================================

TEST(LayoutTest, CreateFromSchema) {
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    schema->add_int_field("age");

    Layout layout(schema);

    EXPECT_EQ(layout.schema(), schema);
    EXPECT_GT(layout.slot_size(), 0);
}

TEST(LayoutTest, FieldOffsets) {
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    schema->add_int_field("age");

    Layout layout(schema);

    // Offsets should be:
    // Flag at 0 (4 bytes)
    // id at 4 (4 bytes)
    // name at 8 (4 + 20 = 24 bytes)
    // age at 32 (4 bytes)

    EXPECT_EQ(layout.offset("id"), 4);
    EXPECT_EQ(layout.offset("name"), 8);
    EXPECT_EQ(layout.offset("age"), 32);
}

TEST(LayoutTest, SlotSize) {
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("id");
    schema->add_string_field("name", 20);
    schema->add_int_field("age");

    Layout layout(schema);

    // Slot size = 4 (flag) + 4 (id) + 24 (name) + 4 (age) = 36
    EXPECT_EQ(layout.slot_size(), 36);
}

TEST(LayoutTest, IntegerFieldSize) {
    auto schema = std::make_shared<Schema>();
    schema->add_int_field("field1");
    schema->add_int_field("field2");
    schema->add_int_field("field3");

    Layout layout(schema);

    // Slot size = 4 (flag) + 4 + 4 + 4 = 16
    EXPECT_EQ(layout.slot_size(), 16);
}

TEST(LayoutTest, VarcharFieldSize) {
    auto schema = std::make_shared<Schema>();
    schema->add_string_field("short", 10);
    schema->add_string_field("long", 100);

    Layout layout(schema);

    // VARCHAR storage = 4-byte length + max chars
    // short: 4 + 10 = 14
    // long: 4 + 100 = 104
    // Slot size = 4 (flag) + 14 + 104 = 122

    EXPECT_EQ(layout.slot_size(), 122);
    EXPECT_EQ(layout.offset("short"), 4);
    EXPECT_EQ(layout.offset("long"), 4 + file::Page::max_length(10));
}

// main() is provided by gtest_main
