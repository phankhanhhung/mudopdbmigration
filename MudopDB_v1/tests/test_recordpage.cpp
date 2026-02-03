#include <gtest/gtest.h>
#include "record/recordpage.hpp"
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

class RecordPageTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_recordpage_test";
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
        bm = std::make_shared<BufferMgr>(fm, lm, 3);

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
// RecordPage Tests
// ============================================================================

TEST_F(RecordPageTest, Format) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // All slots should be EMPTY
    std::optional<size_t> slot = rp.next_after(std::nullopt);
    EXPECT_FALSE(slot.has_value());  // No used slots

    bm->unpin(idx);
}

TEST_F(RecordPageTest, SetGetInt) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    std::optional<size_t> slot = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot.has_value());

    rp.set_int(slot.value(), "id", 123);
    rp.set_int(slot.value(), "age", 25);

    EXPECT_EQ(rp.get_int(slot.value(), "id"), 123);
    EXPECT_EQ(rp.get_int(slot.value(), "age"), 25);

    bm->unpin(idx);
}

TEST_F(RecordPageTest, SetGetString) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    std::optional<size_t> slot = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot.has_value());

    rp.set_string(slot.value(), "name", "Alice");

    EXPECT_EQ(rp.get_string(slot.value(), "name"), "Alice");

    bm->unpin(idx);
}

TEST_F(RecordPageTest, InsertAfter) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert first record
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot1.has_value());
    EXPECT_EQ(slot1.value(), 0);

    // Insert second record
    std::optional<size_t> slot2 = rp.insert_after(slot1);
    ASSERT_TRUE(slot2.has_value());
    EXPECT_EQ(slot2.value(), 1);

    // Insert third record
    std::optional<size_t> slot3 = rp.insert_after(slot2);
    ASSERT_TRUE(slot3.has_value());
    EXPECT_EQ(slot3.value(), 2);

    bm->unpin(idx);
}

TEST_F(RecordPageTest, NextAfter) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert three records
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    std::optional<size_t> slot2 = rp.insert_after(slot1);
    std::optional<size_t> slot3 = rp.insert_after(slot2);

    // Iterate through used slots
    std::optional<size_t> current = rp.next_after(std::nullopt);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 0);

    current = rp.next_after(current);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 1);

    current = rp.next_after(current);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 2);

    current = rp.next_after(current);
    EXPECT_FALSE(current.has_value());  // No more used slots

    bm->unpin(idx);
}

TEST_F(RecordPageTest, DeleteRecord) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert three records
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    std::optional<size_t> slot2 = rp.insert_after(slot1);
    std::optional<size_t> slot3 = rp.insert_after(slot2);

    // Delete middle record
    rp.delete_record(slot2.value());

    // Verify slot 1 is still used
    std::optional<size_t> current = rp.next_after(std::nullopt);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 0);

    // Slot 2 should be skipped
    current = rp.next_after(current);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 2);  // Skip slot 1, go to slot 2

    current = rp.next_after(current);
    EXPECT_FALSE(current.has_value());

    bm->unpin(idx);
}

TEST_F(RecordPageTest, PageFull) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert until page is full
    std::optional<size_t> slot = std::nullopt;
    int count = 0;

    while (true) {
        std::optional<size_t> newslot = rp.insert_after(slot);
        if (!newslot.has_value()) {
            break;  // Page full
        }
        slot = newslot;
        count++;
    }

    // Verify we inserted some records
    EXPECT_GT(count, 0);

    // Slot size = 4 + 4 + 24 + 4 = 36
    // Page size = 400
    // Expected slots = 400 / 36 = 11
    EXPECT_EQ(count, 11);

    bm->unpin(idx);
}

TEST_F(RecordPageTest, MultipleRecords) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert three records with data
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    rp.set_int(slot1.value(), "id", 1);
    rp.set_string(slot1.value(), "name", "Alice");
    rp.set_int(slot1.value(), "age", 25);

    std::optional<size_t> slot2 = rp.insert_after(slot1);
    rp.set_int(slot2.value(), "id", 2);
    rp.set_string(slot2.value(), "name", "Bob");
    rp.set_int(slot2.value(), "age", 30);

    std::optional<size_t> slot3 = rp.insert_after(slot2);
    rp.set_int(slot3.value(), "id", 3);
    rp.set_string(slot3.value(), "name", "Charlie");
    rp.set_int(slot3.value(), "age", 35);

    // Verify all records
    EXPECT_EQ(rp.get_int(slot1.value(), "id"), 1);
    EXPECT_EQ(rp.get_string(slot1.value(), "name"), "Alice");
    EXPECT_EQ(rp.get_int(slot1.value(), "age"), 25);

    EXPECT_EQ(rp.get_int(slot2.value(), "id"), 2);
    EXPECT_EQ(rp.get_string(slot2.value(), "name"), "Bob");
    EXPECT_EQ(rp.get_int(slot2.value(), "age"), 30);

    EXPECT_EQ(rp.get_int(slot3.value(), "id"), 3);
    EXPECT_EQ(rp.get_string(slot3.value(), "name"), "Charlie");
    EXPECT_EQ(rp.get_int(slot3.value(), "age"), 35);

    bm->unpin(idx);
}

TEST_F(RecordPageTest, SlotReuse) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert two records
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    std::optional<size_t> slot2 = rp.insert_after(slot1);

    // Delete first record
    rp.delete_record(slot1.value());

    // Insert new record - should reuse slot 0
    std::optional<size_t> slot3 = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot3.has_value());
    EXPECT_EQ(slot3.value(), 0);  // Reused deleted slot

    bm->unpin(idx);
}

TEST_F(RecordPageTest, FieldBoundaries) {
    BlockId blk = fm->append("test.dat");
    size_t idx = bm->pin(blk);
    Buffer& buff = bm->buffer(idx);

    RecordPage rp(buff, *layout);
    rp.format();

    // Insert two records with different data
    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    rp.set_int(slot1.value(), "id", 111);
    rp.set_string(slot1.value(), "name", "AAA");
    rp.set_int(slot1.value(), "age", 11);

    std::optional<size_t> slot2 = rp.insert_after(slot1);
    rp.set_int(slot2.value(), "id", 222);
    rp.set_string(slot2.value(), "name", "BBB");
    rp.set_int(slot2.value(), "age", 22);

    // Verify fields don't overlap
    EXPECT_EQ(rp.get_int(slot1.value(), "id"), 111);
    EXPECT_EQ(rp.get_string(slot1.value(), "name"), "AAA");
    EXPECT_EQ(rp.get_int(slot1.value(), "age"), 11);

    EXPECT_EQ(rp.get_int(slot2.value(), "id"), 222);
    EXPECT_EQ(rp.get_string(slot2.value(), "name"), "BBB");
    EXPECT_EQ(rp.get_int(slot2.value(), "age"), 22);

    bm->unpin(idx);
}

// main() is provided by gtest_main
