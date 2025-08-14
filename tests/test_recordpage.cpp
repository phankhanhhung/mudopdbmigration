#include <gtest/gtest.h>
#include "record/recordpage.hpp"
#include "record/layout.hpp"
#include "record/schema.hpp"
#include "tx/transaction.hpp"
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
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);

        fm = std::make_shared<FileMgr>(test_dir, blocksize);
        lm = std::make_shared<LogMgr>(fm, logfile);
        bm = std::make_shared<BufferMgr>(fm, lm, 3);

        schema = std::make_shared<Schema>();
        schema->add_int_field("id");
        schema->add_string_field("name", 20);
        schema->add_int_field("age");

        layout = std::make_unique<Layout>(schema);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
};

// ============================================================================
// RecordPage Tests
// ============================================================================

TEST_F(RecordPageTest, Format) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    std::optional<size_t> slot = rp.next_after(std::nullopt);
    EXPECT_FALSE(slot.has_value());

    tx->commit();
}

TEST_F(RecordPageTest, SetGetInt) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    std::optional<size_t> slot = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot.has_value());

    rp.set_int(slot.value(), "id", 123);
    rp.set_int(slot.value(), "age", 25);

    EXPECT_EQ(rp.get_int(slot.value(), "id"), 123);
    EXPECT_EQ(rp.get_int(slot.value(), "age"), 25);

    tx->commit();
}

TEST_F(RecordPageTest, SetGetString) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    std::optional<size_t> slot = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot.has_value());

    rp.set_string(slot.value(), "name", "Alice");
    EXPECT_EQ(rp.get_string(slot.value(), "name"), "Alice");

    tx->commit();
}

TEST_F(RecordPageTest, InsertAfter) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    std::optional<size_t> slot1 = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot1.has_value());
    EXPECT_EQ(slot1.value(), 0);

    std::optional<size_t> slot2 = rp.insert_after(slot1);
    ASSERT_TRUE(slot2.has_value());
    EXPECT_EQ(slot2.value(), 1);

    std::optional<size_t> slot3 = rp.insert_after(slot2);
    ASSERT_TRUE(slot3.has_value());
    EXPECT_EQ(slot3.value(), 2);

    tx->commit();
}

TEST_F(RecordPageTest, NextAfter) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    auto s0 = rp.insert_after(std::nullopt);
    auto s1 = rp.insert_after(s0);
    auto s2 = rp.insert_after(s1);

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
    EXPECT_FALSE(current.has_value());

    tx->commit();
}

TEST_F(RecordPageTest, DeleteRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    auto slot1 = rp.insert_after(std::nullopt);
    auto slot2 = rp.insert_after(slot1);
    auto slot3 = rp.insert_after(slot2);

    rp.delete_record(slot2.value());

    std::optional<size_t> current = rp.next_after(std::nullopt);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 0);

    current = rp.next_after(current);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current.value(), 2);

    current = rp.next_after(current);
    EXPECT_FALSE(current.has_value());

    tx->commit();
}

TEST_F(RecordPageTest, PageFull) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    std::optional<size_t> slot = std::nullopt;
    int count = 0;

    while (true) {
        std::optional<size_t> newslot = rp.insert_after(slot);
        if (!newslot.has_value()) {
            break;
        }
        slot = newslot;
        count++;
    }

    EXPECT_GT(count, 0);
    EXPECT_EQ(count, 11);

    tx->commit();
}

TEST_F(RecordPageTest, MultipleRecords) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    auto slot1 = rp.insert_after(std::nullopt);
    rp.set_int(slot1.value(), "id", 1);
    rp.set_string(slot1.value(), "name", "Alice");
    rp.set_int(slot1.value(), "age", 25);

    auto slot2 = rp.insert_after(slot1);
    rp.set_int(slot2.value(), "id", 2);
    rp.set_string(slot2.value(), "name", "Bob");
    rp.set_int(slot2.value(), "age", 30);

    auto slot3 = rp.insert_after(slot2);
    rp.set_int(slot3.value(), "id", 3);
    rp.set_string(slot3.value(), "name", "Charlie");
    rp.set_int(slot3.value(), "age", 35);

    EXPECT_EQ(rp.get_int(slot1.value(), "id"), 1);
    EXPECT_EQ(rp.get_string(slot1.value(), "name"), "Alice");
    EXPECT_EQ(rp.get_int(slot1.value(), "age"), 25);

    EXPECT_EQ(rp.get_int(slot2.value(), "id"), 2);
    EXPECT_EQ(rp.get_string(slot2.value(), "name"), "Bob");
    EXPECT_EQ(rp.get_int(slot2.value(), "age"), 30);

    EXPECT_EQ(rp.get_int(slot3.value(), "id"), 3);
    EXPECT_EQ(rp.get_string(slot3.value(), "name"), "Charlie");
    EXPECT_EQ(rp.get_int(slot3.value(), "age"), 35);

    tx->commit();
}

TEST_F(RecordPageTest, SlotReuse) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    auto slot1 = rp.insert_after(std::nullopt);
    auto slot2 = rp.insert_after(slot1);

    rp.delete_record(slot1.value());

    auto slot3 = rp.insert_after(std::nullopt);
    ASSERT_TRUE(slot3.has_value());
    EXPECT_EQ(slot3.value(), 0);

    tx->commit();
}

TEST_F(RecordPageTest, FieldBoundaries) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    BlockId blk = tx->append("test.dat");

    RecordPage rp(tx, blk, *layout);
    rp.format();

    auto slot1 = rp.insert_after(std::nullopt);
    rp.set_int(slot1.value(), "id", 111);
    rp.set_string(slot1.value(), "name", "AAA");
    rp.set_int(slot1.value(), "age", 11);

    auto slot2 = rp.insert_after(slot1);
    rp.set_int(slot2.value(), "id", 222);
    rp.set_string(slot2.value(), "name", "BBB");
    rp.set_int(slot2.value(), "age", 22);

    EXPECT_EQ(rp.get_int(slot1.value(), "id"), 111);
    EXPECT_EQ(rp.get_string(slot1.value(), "name"), "AAA");
    EXPECT_EQ(rp.get_int(slot1.value(), "age"), 11);

    EXPECT_EQ(rp.get_int(slot2.value(), "id"), 222);
    EXPECT_EQ(rp.get_string(slot2.value(), "name"), "BBB");
    EXPECT_EQ(rp.get_int(slot2.value(), "age"), 22);

    tx->commit();
}

// main() is provided by gtest_main
