#include <gtest/gtest.h>
#include "record/tablescan.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include "buffer/buffermgr.hpp"
#include "file/filemgr.hpp"
#include "log/logmgr.hpp"
#include <filesystem>
#include <memory>

using namespace record;
using namespace buffer;
using namespace file;
using namespace log;
namespace fs = std::filesystem;

// ============================================================================
// Test Fixture
// ============================================================================

class TableScanTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_tablescan_test";
    std::string logfile = "test.log";
    size_t blocksize = 400;

    std::shared_ptr<FileMgr> fm;
    std::shared_ptr<LogMgr> lm;
    std::shared_ptr<BufferMgr> bm;
    std::shared_ptr<Schema> schema;
    std::unique_ptr<Layout> layout;

    void SetUp() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);

        // Create managers
        fm = std::make_shared<FileMgr>(test_dir, blocksize);
        lm = std::make_shared<LogMgr>(fm, logfile);
        bm = std::make_shared<BufferMgr>(fm, lm, 8);

        // Create schema: {id: INT, name: VARCHAR(20), age: INT}
        schema = std::make_shared<Schema>();
        schema->add_int_field("id");
        schema->add_string_field("name", 20);
        schema->add_int_field("age");

        layout = std::make_unique<Layout>(schema);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

// ============================================================================
// TableScan Tests
// ============================================================================

TEST_F(TableScanTest, CreateEmptyTable) {
    TableScan scan(bm, "students", *layout);

    // New table should have no records
    scan.before_first();
    EXPECT_FALSE(scan.next());

    scan.close();
}

TEST_F(TableScanTest, InsertSingleRecord) {
    TableScan scan(bm, "students", *layout);

    scan.insert();
    scan.set_int("id", 1);
    scan.set_string("name", "Alice");
    scan.set_int("age", 25);

    // Scan the record back
    scan.before_first();
    ASSERT_TRUE(scan.next());

    EXPECT_EQ(scan.get_int("id"), 1);
    EXPECT_EQ(scan.get_string("name"), "Alice");
    EXPECT_EQ(scan.get_int("age"), 25);

    EXPECT_FALSE(scan.next());  // Only one record

    scan.close();
}

TEST_F(TableScanTest, InsertMultipleRecords) {
    TableScan scan(bm, "students", *layout);

    // Insert three records
    scan.insert();
    scan.set_int("id", 1);
    scan.set_string("name", "Alice");
    scan.set_int("age", 25);

    scan.insert();
    scan.set_int("id", 2);
    scan.set_string("name", "Bob");
    scan.set_int("age", 30);

    scan.insert();
    scan.set_int("id", 3);
    scan.set_string("name", "Charlie");
    scan.set_int("age", 35);

    // Scan all records
    scan.before_first();

    ASSERT_TRUE(scan.next());
    EXPECT_EQ(scan.get_int("id"), 1);

    ASSERT_TRUE(scan.next());
    EXPECT_EQ(scan.get_int("id"), 2);

    ASSERT_TRUE(scan.next());
    EXPECT_EQ(scan.get_int("id"), 3);

    EXPECT_FALSE(scan.next());

    scan.close();
}

TEST_F(TableScanTest, ScanAllRecords) {
    TableScan scan(bm, "students", *layout);

    // Insert 5 records
    for (int i = 1; i <= 5; i++) {
        scan.insert();
        scan.set_int("id", i);
        scan.set_string("name", "Person" + std::to_string(i));
        scan.set_int("age", 20 + i);
    }

    // Count records
    scan.before_first();
    int count = 0;
    while (scan.next()) {
        count++;
    }

    EXPECT_EQ(count, 5);

    scan.close();
}

TEST_F(TableScanTest, DeleteRecord) {
    TableScan scan(bm, "students", *layout);

    // Insert three records
    scan.insert();
    scan.set_int("id", 1);

    scan.insert();
    scan.set_int("id", 2);

    scan.insert();
    scan.set_int("id", 3);

    // Delete record with id=2
    scan.before_first();
    while (scan.next()) {
        if (scan.get_int("id") == 2) {
            scan.delete_record();
            break;
        }
    }

    // Verify only 2 records remain
    scan.before_first();
    std::vector<int> ids;
    while (scan.next()) {
        ids.push_back(scan.get_int("id"));
    }

    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 3);

    scan.close();
}

TEST_F(TableScanTest, UpdateRecord) {
    TableScan scan(bm, "students", *layout);

    // Insert a record
    scan.insert();
    scan.set_int("id", 1);
    scan.set_string("name", "Alice");
    scan.set_int("age", 25);

    // Update the record
    scan.before_first();
    ASSERT_TRUE(scan.next());

    scan.set_string("name", "Alicia");
    scan.set_int("age", 26);

    // Verify update
    scan.before_first();
    ASSERT_TRUE(scan.next());

    EXPECT_EQ(scan.get_int("id"), 1);
    EXPECT_EQ(scan.get_string("name"), "Alicia");
    EXPECT_EQ(scan.get_int("age"), 26);

    scan.close();
}

TEST_F(TableScanTest, GetRID) {
    TableScan scan(bm, "students", *layout);

    // Insert a record
    scan.insert();
    scan.set_int("id", 1);

    // Get RID
    scan.before_first();
    ASSERT_TRUE(scan.next());

    std::optional<RID> rid = scan.get_rid();
    ASSERT_TRUE(rid.has_value());

    EXPECT_EQ(rid.value().block_number(), 0);
    EXPECT_EQ(rid.value().slot(), 0);

    scan.close();
}

TEST_F(TableScanTest, MoveToRID) {
    TableScan scan(bm, "students", *layout);

    // Insert three records and save second RID
    scan.insert();
    scan.set_int("id", 1);

    scan.insert();
    scan.set_int("id", 2);
    scan.set_string("name", "Target");

    scan.before_first();
    scan.next();
    scan.next();  // On second record
    RID target_rid = scan.get_rid().value();

    scan.insert();
    scan.set_int("id", 3);

    // Move to specific RID
    scan.move_to_rid(target_rid);

    EXPECT_EQ(scan.get_int("id"), 2);
    EXPECT_EQ(scan.get_string("name"), "Target");

    scan.close();
}

TEST_F(TableScanTest, MultipleBlocks) {
    TableScan scan(bm, "students", *layout);

    // Insert many records to span multiple blocks
    // Slot size = 36, block size = 400, so ~11 records per block
    int num_records = 30;

    for (int i = 1; i <= num_records; i++) {
        scan.insert();
        scan.set_int("id", i);
        scan.set_string("name", "Person" + std::to_string(i));
        scan.set_int("age", 20 + i);
    }

    // Verify all records
    scan.before_first();
    int count = 0;
    while (scan.next()) {
        count++;
        EXPECT_EQ(scan.get_int("id"), count);
    }

    EXPECT_EQ(count, num_records);

    // Verify table spans multiple blocks
    EXPECT_GT(fm->length("students.tbl"), 2);

    scan.close();
}

TEST_F(TableScanTest, InsertIntoFullPage) {
    TableScan scan(bm, "students", *layout);

    // Fill first page (11 records with slot size 36)
    for (int i = 1; i <= 11; i++) {
        scan.insert();
        scan.set_int("id", i);
    }

    // Verify first page is full - table has 1 block
    EXPECT_EQ(fm->length("students.tbl"), 1);

    // Insert one more - should create new block
    scan.insert();
    scan.set_int("id", 12);

    // Verify new block was created
    EXPECT_EQ(fm->length("students.tbl"), 2);

    // Verify all 12 records accessible
    scan.before_first();
    int count = 0;
    while (scan.next()) {
        count++;
    }
    EXPECT_EQ(count, 12);

    scan.close();
}

// main() is provided by gtest_main
