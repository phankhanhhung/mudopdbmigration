#include <gtest/gtest.h>
#include "record/schema.hpp"

using namespace record;

// ============================================================================
// Schema Tests
// ============================================================================

TEST(SchemaTest, CreateEmpty) {
    Schema schema;

    EXPECT_EQ(schema.fields().size(), 0);
}

TEST(SchemaTest, AddIntField) {
    Schema schema;

    schema.add_int_field("id");
    schema.add_int_field("age");

    EXPECT_TRUE(schema.has_field("id"));
    EXPECT_TRUE(schema.has_field("age"));
    EXPECT_FALSE(schema.has_field("name"));

    EXPECT_EQ(schema.type("id"), Type::INTEGER);
    EXPECT_EQ(schema.type("age"), Type::INTEGER);

    EXPECT_EQ(schema.length("id"), 0);
    EXPECT_EQ(schema.length("age"), 0);
}

TEST(SchemaTest, AddStringField) {
    Schema schema;

    schema.add_string_field("name", 20);
    schema.add_string_field("email", 50);

    EXPECT_TRUE(schema.has_field("name"));
    EXPECT_TRUE(schema.has_field("email"));

    EXPECT_EQ(schema.type("name"), Type::VARCHAR);
    EXPECT_EQ(schema.type("email"), Type::VARCHAR);

    EXPECT_EQ(schema.length("name"), 20);
    EXPECT_EQ(schema.length("email"), 50);
}

TEST(SchemaTest, AddFromOtherSchema) {
    Schema schema1;
    schema1.add_int_field("id");
    schema1.add_string_field("name", 20);

    Schema schema2;
    schema2.add("id", schema1);
    schema2.add("name", schema1);

    EXPECT_TRUE(schema2.has_field("id"));
    EXPECT_TRUE(schema2.has_field("name"));

    EXPECT_EQ(schema2.type("id"), Type::INTEGER);
    EXPECT_EQ(schema2.type("name"), Type::VARCHAR);
    EXPECT_EQ(schema2.length("name"), 20);
}

TEST(SchemaTest, AddAllFields) {
    Schema schema1;
    schema1.add_int_field("id");
    schema1.add_string_field("name", 20);
    schema1.add_int_field("age");

    Schema schema2;
    schema2.add_string_field("email", 50);
    schema2.add_all(schema1);

    EXPECT_TRUE(schema2.has_field("email"));
    EXPECT_TRUE(schema2.has_field("id"));
    EXPECT_TRUE(schema2.has_field("name"));
    EXPECT_TRUE(schema2.has_field("age"));

    EXPECT_EQ(schema2.fields().size(), 4);
}

TEST(SchemaTest, FieldOrder) {
    Schema schema;

    schema.add_int_field("id");
    schema.add_string_field("name", 20);
    schema.add_int_field("age");
    schema.add_string_field("email", 50);

    const auto& fields = schema.fields();

    EXPECT_EQ(fields.size(), 4);
    EXPECT_EQ(fields[0], "id");
    EXPECT_EQ(fields[1], "name");
    EXPECT_EQ(fields[2], "age");
    EXPECT_EQ(fields[3], "email");
}

// main() is provided by gtest_main
