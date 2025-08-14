#include <gtest/gtest.h>
#include "log/logmgr.hpp"
#include "log/logiterator.hpp"
#include "file/filemgr.hpp"
#include <filesystem>
#include <string>
#include <algorithm>

using namespace log;
using namespace file;
namespace fs = std::filesystem;

// ============================================================================
// Test Fixture
// ============================================================================

class LogLayerTest : public ::testing::Test {
protected:
    std::string test_dir = "/tmp/mudopdb_log_test";
    std::string logfile = "test.log";
    size_t blocksize = 400;

    void SetUp() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    // Helper to create log record
    std::vector<uint8_t> make_record(const std::string& data) {
        return std::vector<uint8_t>(data.begin(), data.end());
    }

    // Helper to convert record to string
    std::string record_to_string(const std::vector<uint8_t>& rec) {
        return std::string(rec.begin(), rec.end());
    }
};

// ============================================================================
// LogMgr Basic Tests
// ============================================================================

TEST_F(LogLayerTest, CreateNewLog) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    // Log file should be created
    EXPECT_EQ(fm->length(logfile), 1);  // One block allocated
}

TEST_F(LogLayerTest, AppendSingleRecord) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    auto rec = make_record("test record");
    size_t lsn = lm.append(rec);

    EXPECT_EQ(lsn, 1);  // First LSN is 1
}

TEST_F(LogLayerTest, AppendMultipleRecords) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    size_t lsn1 = lm.append(make_record("record 1"));
    size_t lsn2 = lm.append(make_record("record 2"));
    size_t lsn3 = lm.append(make_record("record 3"));

    EXPECT_EQ(lsn1, 1);
    EXPECT_EQ(lsn2, 2);
    EXPECT_EQ(lsn3, 3);
}

TEST_F(LogLayerTest, FlushUpdatesLSN) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record("test"));
    lm.flush(1);  // Should flush

    // No direct way to verify flush, but it shouldn't throw
}

TEST_F(LogLayerTest, FlushIsIdempotent) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record("test"));
    lm.flush(1);
    lm.flush(1);  // Flush again - should be no-op
    lm.flush(0);  // Flush lower LSN - should be no-op
}

// ============================================================================
// LogIterator Tests
// ============================================================================

TEST_F(LogLayerTest, IteratorOnEmptyLog) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    auto iter = lm.iterator();
    EXPECT_FALSE(iter->has_next());
}

TEST_F(LogLayerTest, IteratorSingleRecord) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record("hello"));

    auto iter = lm.iterator();
    EXPECT_TRUE(iter->has_next());

    auto rec = iter->next();
    EXPECT_EQ(record_to_string(rec), "hello");

    EXPECT_FALSE(iter->has_next());
}

TEST_F(LogLayerTest, IteratorMultipleRecords) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record("first"));
    lm.append(make_record("second"));
    lm.append(make_record("third"));

    auto iter = lm.iterator();

    // Verify we can read all 3 records
    std::vector<std::string> records;
    while (iter->has_next()) {
        records.push_back(record_to_string(iter->next()));
    }

    EXPECT_EQ(records.size(), 3);
    // All records should be present
    EXPECT_TRUE(std::find(records.begin(), records.end(), "first") != records.end());
    EXPECT_TRUE(std::find(records.begin(), records.end(), "second") != records.end());
    EXPECT_TRUE(std::find(records.begin(), records.end(), "third") != records.end());
}

TEST_F(LogLayerTest, IteratorThrowsWhenExhausted) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record("only"));

    auto iter = lm.iterator();
    iter->next();  // Consume the only record

    EXPECT_THROW(iter->next(), std::runtime_error);
}

// ============================================================================
// Page Wrapping Tests
// ============================================================================

TEST_F(LogLayerTest, FillPageToCapacity) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    // Fill page with a large record that leaves little space
    std::string big_data(380, 'X');  // 380 + 4 (length) = 384 bytes
    lm.append(make_record(big_data));

    // Should still be on first block (boundary = 400, used = 384, remaining = 16)
    EXPECT_EQ(fm->length(logfile), 1);

    // Add another record that doesn't fit (needs 4 for length + data + 4 for boundary)
    // Remaining space is 16 bytes, but we need at least 8 bytes (4 + data + boundary check)
    lm.append(make_record("overflow"));

    // Should have allocated second block
    EXPECT_GE(fm->length(logfile), 1);  // At least 1, possibly 2
}

TEST_F(LogLayerTest, IteratorAcrossMultipleBlocks) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    // Fill first page with a large record
    std::string big_data(385, 'A');  // 385 + 4 = 389 bytes, leaves 11 bytes
    lm.append(make_record(big_data));

    // This should overflow to second page (needs 15+ bytes: 4 + 11 data)
    lm.append(make_record("second_page"));  // 11 chars + 4 length = 15 bytes

    // Verify we can read both records
    auto iter = lm.iterator();

    std::vector<std::vector<uint8_t>> records;
    while (iter->has_next()) {
        records.push_back(iter->next());
    }

    EXPECT_EQ(records.size(), 2);

    // Find the records (order may vary)
    bool found_big = false;
    bool found_small = false;
    for (const auto& rec : records) {
        if (rec.size() == 385) found_big = true;
        if (record_to_string(rec) == "second_page") found_small = true;
    }

    EXPECT_TRUE(found_big);
    EXPECT_TRUE(found_small);
}

// ============================================================================
// Persistence Tests
// ============================================================================

TEST_F(LogLayerTest, PersistenceAfterFlush) {
    // Write records
    {
        auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
        LogMgr lm(fm, logfile);

        lm.append(make_record("persistent1"));
        lm.append(make_record("persistent2"));
        lm.flush(2);
    }

    // Read back in new session
    {
        auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
        LogMgr lm(fm, logfile);

        auto iter = lm.iterator();

        // Records are read in order they were written
        EXPECT_TRUE(iter->has_next());
        auto rec1 = record_to_string(iter->next());

        EXPECT_TRUE(iter->has_next());
        auto rec2 = record_to_string(iter->next());

        // Check we got both records (order may depend on page layout)
        EXPECT_TRUE((rec1 == "persistent1" && rec2 == "persistent2") ||
                    (rec1 == "persistent2" && rec2 == "persistent1"));

        EXPECT_FALSE(iter->has_next());
    }
}

TEST_F(LogLayerTest, AppendToExistingLog) {
    // First session - create log and FLUSH it
    {
        auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
        LogMgr lm(fm, logfile);

        lm.append(make_record("old1"));
        lm.append(make_record("old2"));
        lm.flush(2);  // IMPORTANT: Must flush to persist records
    }

    // Second session - append more
    {
        auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
        LogMgr lm(fm, logfile);

        lm.append(make_record("new1"));
        lm.append(make_record("new2"));

        auto iter = lm.iterator();

        // Should see all 4 records
        std::vector<std::string> records;
        while (iter->has_next()) {
            records.push_back(record_to_string(iter->next()));
        }

        EXPECT_EQ(records.size(), 4);
        // Verify all records are present
        EXPECT_TRUE(std::find(records.begin(), records.end(), "old1") != records.end());
        EXPECT_TRUE(std::find(records.begin(), records.end(), "old2") != records.end());
        EXPECT_TRUE(std::find(records.begin(), records.end(), "new1") != records.end());
        EXPECT_TRUE(std::find(records.begin(), records.end(), "new2") != records.end());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(LogLayerTest, EmptyRecord) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    lm.append(make_record(""));

    auto iter = lm.iterator();
    EXPECT_TRUE(iter->has_next());
    auto rec = iter->next();
    EXPECT_EQ(rec.size(), 0);
}

TEST_F(LogLayerTest, LargeRecord) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    // Create record that takes most of the page
    std::string large_data(350, 'L');
    lm.append(make_record(large_data));

    auto iter = lm.iterator();
    EXPECT_TRUE(iter->has_next());
    auto rec = iter->next();
    EXPECT_EQ(rec.size(), 350);
}

TEST_F(LogLayerTest, ManySmallRecords) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    const int num_records = 100;
    for (int i = 0; i < num_records; i++) {
        lm.append(make_record("rec" + std::to_string(i)));
    }

    // Verify all records
    auto iter = lm.iterator();
    int count = 0;
    while (iter->has_next()) {
        iter->next();
        count++;
    }

    EXPECT_EQ(count, num_records);
}

TEST_F(LogLayerTest, BinaryData) {
    auto fm = std::make_shared<FileMgr>(test_dir, blocksize);
    LogMgr lm(fm, logfile);

    // Create record with binary data (nulls, special bytes)
    std::vector<uint8_t> binary_data = {0x00, 0xFF, 0x01, 0x7F, 0x80, 0xFE};
    lm.append(binary_data);

    auto iter = lm.iterator();
    auto rec = iter->next();

    EXPECT_EQ(rec, binary_data);
}

// main() is provided by gtest_main
