#include <gtest/gtest.h>
#include "record/tablescan.hpp"
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
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);

        fm = std::make_shared<FileMgr>(test_dir, blocksize);
        lm = std::make_shared<LogMgr>(fm, logfile);
        bm = std::make_shared<BufferMgr>(fm, lm, 8);

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
// TableScan Tests
// ============================================================================

TEST_F(TableScanTest, CreateEmptyTable) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.before_first().value();
    EXPECT_FALSE(scan.next().value());

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, InsertSingleRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();
    scan.set_string("name", "Alice").value();
    scan.set_int("age", 25).value();

    scan.before_first().value();
    ASSERT_TRUE(scan.next().value());

    EXPECT_EQ(scan.get_int("id").value(), 1);
    EXPECT_EQ(scan.get_string("name").value(), "Alice");
    EXPECT_EQ(scan.get_int("age").value(), 25);

    EXPECT_FALSE(scan.next().value());

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, InsertMultipleRecords) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();
    scan.set_string("name", "Alice").value();
    scan.set_int("age", 25).value();

    scan.insert().value();
    scan.set_int("id", 2).value();
    scan.set_string("name", "Bob").value();
    scan.set_int("age", 30).value();

    scan.insert().value();
    scan.set_int("id", 3).value();
    scan.set_string("name", "Charlie").value();
    scan.set_int("age", 35).value();

    scan.before_first().value();

    ASSERT_TRUE(scan.next().value());
    EXPECT_EQ(scan.get_int("id").value(), 1);

    ASSERT_TRUE(scan.next().value());
    EXPECT_EQ(scan.get_int("id").value(), 2);

    ASSERT_TRUE(scan.next().value());
    EXPECT_EQ(scan.get_int("id").value(), 3);

    EXPECT_FALSE(scan.next().value());

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, ScanAllRecords) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    for (int i = 1; i <= 5; i++) {
        scan.insert().value();
        scan.set_int("id", i).value();
        scan.set_string("name", "Person" + std::to_string(i)).value();
        scan.set_int("age", 20 + i).value();
    }

    scan.before_first().value();
    int count = 0;
    while (scan.next().value()) {
        count++;
    }

    EXPECT_EQ(count, 5);

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, DeleteRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();

    scan.insert().value();
    scan.set_int("id", 2).value();

    scan.insert().value();
    scan.set_int("id", 3).value();

    scan.before_first().value();
    while (scan.next().value()) {
        if (scan.get_int("id").value() == 2) {
            scan.delete_record().value();
            break;
        }
    }

    scan.before_first().value();
    std::vector<int> ids;
    while (scan.next().value()) {
        ids.push_back(scan.get_int("id").value());
    }

    EXPECT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 1);
    EXPECT_EQ(ids[1], 3);

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, UpdateRecord) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();
    scan.set_string("name", "Alice").value();
    scan.set_int("age", 25).value();

    scan.before_first().value();
    ASSERT_TRUE(scan.next().value());

    scan.set_string("name", "Alicia").value();
    scan.set_int("age", 26).value();

    scan.before_first().value();
    ASSERT_TRUE(scan.next().value());

    EXPECT_EQ(scan.get_int("id").value(), 1);
    EXPECT_EQ(scan.get_string("name").value(), "Alicia");
    EXPECT_EQ(scan.get_int("age").value(), 26);

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, GetRID) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();

    scan.before_first().value();
    ASSERT_TRUE(scan.next().value());

    std::optional<RID> rid = scan.get_rid();
    ASSERT_TRUE(rid.has_value());

    EXPECT_EQ(rid.value().block_number(), 0);
    EXPECT_EQ(rid.value().slot(), 0);

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, MoveToRID) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    scan.insert().value();
    scan.set_int("id", 1).value();

    scan.insert().value();
    scan.set_int("id", 2).value();
    scan.set_string("name", "Target").value();

    scan.before_first().value();
    scan.next().value();
    scan.next().value();  // On second record
    RID target_rid = scan.get_rid().value();

    scan.insert().value();
    scan.set_int("id", 3).value();

    scan.move_to_rid(target_rid).value();

    EXPECT_EQ(scan.get_int("id").value(), 2);
    EXPECT_EQ(scan.get_string("name").value(), "Target");

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, MultipleBlocks) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    int num_records = 30;

    for (int i = 1; i <= num_records; i++) {
        scan.insert().value();
        scan.set_int("id", i).value();
        scan.set_string("name", "Person" + std::to_string(i)).value();
        scan.set_int("age", 20 + i).value();
    }

    scan.before_first().value();
    int count = 0;
    while (scan.next().value()) {
        count++;
        EXPECT_EQ(scan.get_int("id").value(), count);
    }

    EXPECT_EQ(count, num_records);
    EXPECT_GT(fm->length("students.tbl"), 2u);

    scan.close().value();
    tx->commit();
}

TEST_F(TableScanTest, InsertIntoFullPage) {
    auto tx = std::make_shared<tx::Transaction>(fm, lm, bm);
    TableScan scan(tx, "students", *layout);

    for (int i = 1; i <= 11; i++) {
        scan.insert().value();
        scan.set_int("id", i).value();
    }

    EXPECT_EQ(fm->length("students.tbl"), 1u);

    scan.insert().value();
    scan.set_int("id", 12).value();

    EXPECT_EQ(fm->length("students.tbl"), 2u);

    scan.before_first().value();
    int count = 0;
    while (scan.next().value()) {
        count++;
    }
    EXPECT_EQ(count, 12);

    scan.close().value();
    tx->commit();
}

// main() is provided by gtest_main
